/*
 *   Copyright (C) 2020 by Jonathan Naylor G4KLX
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "FMControl.h"

#include <string>

#if defined(DUMP_RF_AUDIO)
#include <stdio.h>
#endif

#define SWAP_BYTES_16(a) (((a >> 8) & 0x00FFU) | ((a << 8) & 0xFF00U))

const float        DEEMPHASIS_GAIN_DB  = 0.0F;
const float        PREEMPHASIS_GAIN_DB = 0.0F;
const float        FILTER_GAIN_DB      = 0.0F;
const unsigned int FM_MASK             = 0x00000FFFU;

CFMControl::CFMControl(CFMNetwork* network) :
m_network(network),
m_enabled(false),
m_incomingRFAudio(1600U, "Incoming RF FM Audio"),
m_preemphasis (NULL),
m_deemphasis  (NULL),
m_filterStage1(NULL),
m_filterStage2(NULL),
m_filterStage3(NULL)
{
    m_preemphasis  = new CIIRDirectForm1Filter(8.315375384336983F,-7.03334621603483F,0.0F,1.0F,0.282029168302153F,0.0F, PREEMPHASIS_GAIN_DB);
    m_deemphasis   = new CIIRDirectForm1Filter(0.07708787090460224F,0.07708787090460224F,0.0F,1.0F,-0.8458242581907955F,0.0F, DEEMPHASIS_GAIN_DB);

    //cheby type 1 0.2dB cheby type 1 3rd order 300-2700Hz fs=8000
    m_filterStage1 = new CIIRDirectForm1Filter(0.29495028f, 0.0f, -0.29495028f, 1.0f, -0.61384624f, -0.057158668f, FILTER_GAIN_DB);
    m_filterStage2 = new CIIRDirectForm1Filter(1.0f, 2.0f, 1.0f, 1.0f, 0.9946123f, 0.6050482f, FILTER_GAIN_DB);
    m_filterStage3 = new CIIRDirectForm1Filter(1.0f, -2.0f, 1.0f, 1.0f, -1.8414584f, 0.8804949f, FILTER_GAIN_DB);
}

CFMControl::~CFMControl()
{
    delete m_preemphasis ;
    delete m_deemphasis  ;

    delete m_filterStage1;
    delete m_filterStage2;
    delete m_filterStage3;
}

bool CFMControl::writeModem(const unsigned char* data, unsigned int length)
{
    assert(data != NULL);
    assert(length > 0U);

    if (m_network == NULL)
        return true;
        
    if (data[0U] == TAG_HEADER)
        return true;

    if (data[0U] == TAG_EOT)
        return m_network->writeEOT();

    if (data[0U] != TAG_DATA)
        return false;

    m_incomingRFAudio.addData(data + 1U, length - 1U);
    unsigned int bufferLength = m_incomingRFAudio.dataSize();
    if (bufferLength > 255U)
        bufferLength = 255U;

    if (bufferLength >= 3U) {
#if defined(DUMP_RF_AUDIO)
    FILE * audiofile = fopen("./audiodump.bin", "ab");
#endif

        bufferLength = bufferLength - bufferLength % 3U; //round down to nearest multiple of 3
        unsigned char bufferData[255U];        
        m_incomingRFAudio.getData(bufferData, bufferLength);
    
        unsigned int nSamples = 0;
        float samples[85U]; // 255 / 3;

        // Unpack the serial data into float values.
        for (unsigned int i = 0U; i < bufferLength; i += 3U) {
            short sample1 = 0U;
            short sample2 = 0U;

            unsigned int pack = 0U;
            unsigned char* packPointer = (unsigned char*)&pack;

            packPointer[0U] = bufferData[i];
            packPointer[1U] = bufferData[i + 1U];
            packPointer[2U] = bufferData[i + 2U];

            //extract unsigned 12 bit samples to 16 bit signed
            sample2 = short(int(pack & FM_MASK) - 2048);
            sample1 = short(int(pack >> 12) - 2048);

            // Convert from unsigned short (0 - +4095) to float (-1.0 - +1.0)
            samples[nSamples++] = float(sample1) / 2048.0F;
            samples[nSamples++] = float(sample2) / 2048.0F;
        }

        //De-emphasise the data and remove CTCSS
        for (unsigned int i = 0U; i < nSamples; i++) {
            samples[i] = m_deemphasis->filter(samples[i]);
            samples[i] = m_filterStage3->filter(m_filterStage2->filter(m_filterStage1->filter(samples[i])));
        }

        unsigned short out[170U]; // 85 * 2
        unsigned int nOut = 0U;

        // Repack the data (8-bit unsigned values containing unsigned 16-bit data)
        for (unsigned int i = 0U; i < nSamples; i++) {
            unsigned short sample = (unsigned short)((samples[i] + 1.0F) * 32767.0F + 0.5F);
            out[nOut++] = SWAP_BYTES_16(sample);//change endianess to network order, transmit MSB first
        }

#if defined(DUMP_RF_AUDIO) 
        if(audiofile != NULL)
            fwrite(out, sizeof(unsigned short), nOut, audiofile);
#endif

#if defined(DUMP_RF_AUDIO)
    if(audiofile != NULL)
        fclose(audiofile);
#endif

        return m_network->writeData((unsigned char*)out, nOut * sizeof(unsigned short));
    }

    return true;
}

unsigned int CFMControl::readModem(unsigned char* data, unsigned int space)
{
    assert(data != NULL);
    assert(space > 0U);

    if (m_network == NULL)
        return 0U;

    if(space > 252U)
        space = 252U;

    unsigned short netData[84U];//modem can handle up to 84 samples (252 bytes) at a time
    unsigned int length = m_network->read((unsigned char*)netData, 84U * sizeof(unsigned short));
    length /= sizeof(unsigned short);
    if (length == 0U)
        return 0U;

    unsigned int pack = 0U;
    unsigned char* packPointer = (unsigned char*)&pack;
    unsigned int nData = 0U;

    for(unsigned int i = 0; i < length; i++) {
        unsigned short netSample = SWAP_BYTES_16(netData[i]);//((netData[i] << 8) & 0xFF00U)| ((netData[i] >> 8) & 0x00FFU);
        // Convert the unsigned 16-bit data (+65535 - 0) to float (+1.0 - -1.0)
        float sampleFloat = (float(netSample) / 32768.0F) - 1.0F;

        //preemphasis
        sampleFloat = m_preemphasis->filter(sampleFloat);

        // Convert float to 12-bit samples (0 to 4095)
        unsigned int sample12bit = (unsigned int)((sampleFloat + 1.0F) * 2048.0F + 0.5F);

        // pack 2 samples onto 3 bytes
        if((i & 1U) == 0) {
            pack = 0U;
            pack = sample12bit << 12;
        } else {
            pack |= sample12bit;

            data[nData++] = packPointer[0U];
            data[nData++] = packPointer[1U];
            data[nData++] = packPointer[2U];
        }
    }

    return nData;


    // float samples[84U];
    // unsigned int nSamples = 0U;
    // // Convert the unsigned 16-bit data (+65535 - 0) to float (+1.0 - -1.0)
    // for (unsigned int i = 0U; i < length; i += 2U) {
    //     unsigned short sample = (netData[i + 0U] << 8) | netData[i + 1U];
    //     samples[nSamples++] = (float(sample) / 32768.0F) - 1.0F;
    // }

    // //Pre-emphasise the data and other stuff.
    // for (unsigned int i = 0U; i < nSamples; i++)
    //    samples[i] = m_preemphasis->filter(samples[i]);

    // // Pack the floating point data (+1.0 to -1.0) to packed 12-bit samples (+2047 - -2048)
    // unsigned int pack = 0U;
    // unsigned char* packPointer = (unsigned char*)&pack;
    // unsigned int j = 0U;
    // unsigned int i = 0U;
    // for (; i < nSamples && j < space; i += 2U, j += 3U) {
    //     unsigned short sample1 = (unsigned short)((samples[i] + 1.0F) * 2048.0F + 0.5F);
    //     unsigned short sample2 = (unsigned short)((samples[i + 1] + 1.0F) * 2048.0F + 0.5F);

    //     pack = 0U;
    //     pack = ((unsigned int)sample1) << 12;
    //     pack |= sample2;

    //     data[j] = packPointer[0U];
    //     data[j + 1U] = packPointer[1U];
    //     data[j + 2U] = packPointer[2U];
    // }

    // return j;//return the number of bytes written
}

void CFMControl::clock(unsigned int ms)
{
    // May not be needed
}

void CFMControl::enable(bool enabled)
{
    // May not be needed
}
