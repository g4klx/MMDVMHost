/*
 *   Copyright (C) 2020,2021,2024 by Jonathan Naylor G4KLX
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
#include <cstdio>
#endif

#include <cassert>

#define SWAP_BYTES_16(a) (((a >> 8) & 0x00FFU) | ((a << 8) & 0xFF00U))

const float        DEEMPHASIS_GAIN_DB  = 8.0F;  // Audio gain  adjustment
const float        PREEMPHASIS_GAIN_DB = 0.0F;  // Audio gain  adjustment
const unsigned int FM_MASK             = 0x00000FFFU;

// Created using http://t-filter.engineerjs.com/
const float FILTER_TAPS[] = {
    -0.05171164345032154F,     0.004577697825204152F,   0.0841254955090718F,     0.021290266836279378F,  -0.004089760781267529F,
     0.017543929045823792F,   -0.019757885035823444F,  -0.007257327157430058F,  -0.004449379743389856F,  -0.02385756312026885F,
     0.0008185571872412343F,  -0.013864145248825219F,  -0.011210498318183482F,   0.005254565026658612F,  -0.015043483504571332F,
     0.005461884596609861F,    0.002026322985797473F,  -0.007542135237135551F,   0.015720810214077726F,  -0.004651704877827266F,
     0.005558418419098179F,    0.013941762626730268F,  -0.007509473824811953F,   0.016618272581132985F,   0.002487274459992476F,
    -0.0024802940122075103F,   0.017683063350944713F,  -0.009994442394814056F,   0.007156447064599394F,   0.006433822485904772F
    -0.014292696730698431F,    0.013319129319180239F,  -0.010408978940031273F,  -0.007266470155557534F,   0.009230635462251885F,
    -0.021463578074814135F,    0.005194836976513505F,  -0.004126873023205119F,  -0.01808260377380932F,    0.01304137129976236F,
    -0.017532323849990875F,   -0.001929317250481216F,   0.009395238582798674F,  -0.01975948679774396F,    0.01622855958724977F,
    -0.0029621037585558234F,  -0.006899065995755296F,   0.02321184132395859F,   -0.013467585741796682F,   0.01371204158005827F,
     0.013692208003629535F,   -0.012367114737680571F,   0.02722969001859293F,   -0.005345784554487899F,   0.0010074007729973767F,
     0.022305163683623532F,   -0.019636621576653458F,   0.016349411295640864F,   0.0003626724003974509F, -0.018539298657843845F,
     0.01994785867615676F,    -0.024001641760916962F,  -0.0034795731393629766F,  0.0060060611058407164F, -0.033081075064776075F,
     0.012754552578116456F,   -0.01694452885708811F,   -0.019221979726946815F,   0.016490911013530663F,  -0.03092454436058619F,
     0.00861201477103357F,     0.004613884756593608F,  -0.021987826895264795F,   0.030752871497932738F,  -0.011775928674227762F,
     0.007473527853524001F,    0.03126561745903647F,   -0.016067027177553667F,   0.0378218068192532F,     0.010312992900560435F,
    -0.0006238863579307288F,   0.04477891903198119F,   -0.015623491538694835F,   0.02384438222815608F,    0.01750864066332353F,
    -0.026325530246751806F,    0.03302218423753518F,   -0.03126149618245644F,   -0.014849557775866738F,   0.005783253669755862F,
    -0.07047543753434074F,     0.004291583771423271F,  -0.05850933604118403F,   -0.06807719431962597F,    0.00009772317389628164F,
    -0.13523267568572162F,     0.0004875192337944561F, -0.07994164839657204F,   -0.18719066454627675F,    0.5305904386356379F,
     0.5305904386356379F,     -0.18719066454627675F,   -0.07994164839657204F,    0.0004875192337944561F, -0.13523267568572162F,
     0.00009772317389628164F, -0.06807719431962597F,   -0.05850933604118403F,    0.004291583771423271F,  -0.07047543753434074F,
     0.005783253669755862F,   -0.014849557775866738F,  -0.03126149618245644F,    0.03302218423753518F,   -0.026325530246751806F,
     0.01750864066332353F,     0.02384438222815608F,   -0.015623491538694835F,   0.04477891903198119F,   -0.0006238863579307288F,
     0.010312992900560435F,    0.0378218068192532F,    -0.016067027177553667F,   0.03126561745903647F,    0.007473527853524001F,
    -0.011775928674227762F,    0.030752871497932738F,  -0.021987826895264795F,   0.004613884756593608F,   0.00861201477103357F,
    -0.03092454436058619F,     0.016490911013530663F,  -0.019221979726946815F,  -0.01694452885708811F,    0.012754552578116456F,
    -0.033081075064776075F,    0.0060060611058407164F, -0.0034795731393629766F, -0.024001641760916962F,   0.01994785867615676F,
    -0.018539298657843845F,    0.0003626724003974509F,  0.016349411295640864F,  -0.019636621576653458F,   0.022305163683623532F,
     0.0010074007729973767F,  -0.005345784554487899F,   0.02722969001859293F,   -0.012367114737680571F,   0.013692208003629535F,
     0.01371204158005827F,    -0.013467585741796682F,   0.02321184132395859F,   -0.006899065995755296F,  -0.0029621037585558234F,
     0.01622855958724977F,    -0.01975948679774396F,    0.009395238582798674F,  -0.001929317250481216F,  -0.017532323849990875F,
     0.01304137129976236F,    -0.01808260377380932F,   -0.004126873023205119F,   0.005194836976513505F,  -0.021463578074814135F,
     0.009230635462251885F,   -0.007266470155557534F,  -0.010408978940031273F,   0.013319129319180239F,  -0.014292696730698431F,
     0.006433822485904772F,    0.007156447064599394F,  -0.009994442394814056F,   0.017683063350944713F,  -0.0024802940122075103F,
     0.002487274459992476F,    0.016618272581132985F,  -0.007509473824811953F,   0.013941762626730268F,   0.005558418419098179F,
    -0.004651704877827266F,    0.015720810214077726F,  -0.007542135237135551F,   0.002026322985797473F,   0.005461884596609861F,
    -0.015043483504571332F,    0.005254565026658612F,  -0.011210498318183482F,  -0.013864145248825219F,   0.0008185571872412343F,
    -0.02385756312026885F,    -0.004449379743389856F,  -0.007257327157430058F,  -0.019757885035823444F,   0.017543929045823792F,
    -0.004089760781267529F,    0.021290266836279378F,   0.0841254955090718F,     0.004577697825204152F,  -0.05171164345032154F
};

const unsigned int FILTER_TAPS_COUNT = 200U;

CFMControl::CFMControl(IFMNetwork* network, float txAudioGain, float rxAudioGain, bool preEmphasisOn, bool deEmphasisOn) :
m_network(network),
m_txAudioGain(txAudioGain),
m_rxAudioGain(rxAudioGain),
m_preEmphasisOn(preEmphasisOn),
m_deEmphasisOn(deEmphasisOn),
m_enabled(false),
m_incomingRFAudio(1600U, "Incoming RF FM Audio"),
m_preEmphasis(8.315375384336983F, -7.03334621603483F,    0.0F, 1.0F,  0.282029168302153F,  0.0F, PREEMPHASIS_GAIN_DB),
m_deEmphasis(0.07708787090460224F, 0.07708787090460224F, 0.0F, 1.0F, -0.8458242581907955F, 0.0F, DEEMPHASIS_GAIN_DB),
m_filter(FILTER_TAPS, FILTER_TAPS_COUNT)
{
    assert(txAudioGain > 0.0F);
    assert(rxAudioGain > 0.0F);
}

CFMControl::~CFMControl()
{
}

bool CFMControl::writeModem(const unsigned char* data, unsigned int length)
{
    assert(data != NULL);
    assert(length > 0U);

    if (m_network == NULL)
        return true;

    if (data[0U] == TAG_HEADER)
        return m_network->writeStart();

    if (data[0U] == TAG_EOT)
        return m_network->writeEnd();

    if (data[0U] != TAG_DATA)
        return false;

    m_incomingRFAudio.addData(data + 1U, length - 1U);
    unsigned int bufferLength = m_incomingRFAudio.dataSize();
    if (bufferLength > 240U)    // 160 samples 12-bit
        bufferLength = 240U;    // 160 samples 12-bit

    if (bufferLength >= 3U) {
        bufferLength = bufferLength - bufferLength % 3U;    // Round down to nearest multiple of 3
        unsigned char bufferData[240U];                     // 160 samples 12-bit       
        m_incomingRFAudio.getData(bufferData, bufferLength);

        unsigned int pack = 0U;
        unsigned char* packPointer = (unsigned char*)&pack;
        float out[160U];                                    // 160 samples 12-bit
        unsigned int nOut = 0U;
        short unpackedSamples[2U];

        for (unsigned int i = 0U; i < bufferLength; i += 3U) {
            // Extract unsigned 12 bit unsigned sample pairs packed into 3 bytes to 16 bit signed
            packPointer[0U] = bufferData[i + 0U];
            packPointer[1U] = bufferData[i + 1U];
            packPointer[2U] = bufferData[i + 2U];

            unpackedSamples[1U] = short(int(pack & FM_MASK) - 2048);
            unpackedSamples[0U] = short(int(pack >> 12 & FM_MASK) - 2048);

            // Process unpacked sample pair
            for (unsigned char j = 0U; j < 2U; j++) {
                // Convert to float (-1.0 to +1.0)
                float sampleFloat = (float(unpackedSamples[j]) * m_rxAudioGain) / 2048.0F;

                // De-emphasise and remove CTCSS
                if (m_deEmphasisOn)
                    sampleFloat = m_deEmphasis.filter(sampleFloat);

                out[nOut++] = m_filter.filter(sampleFloat);
            }
        }

#if defined(DUMP_RF_AUDIO)
		FILE* audiofile = ::fopen("./audiodump.bin", "ab");
		if (audiofile != NULL) {
			::fwrite(out, sizeof(float), nOut, audiofile);
			::fclose(audiofile);
		}
#endif
		return m_network->writeData(out, nOut);
	}

	return true;
}

unsigned int CFMControl::readModem(unsigned char* data, unsigned int space)
{
	assert(data != NULL);
	assert(space > 0U);

	if (m_network == NULL)
		return 0U;

	if (space > 240U)		// 160 samples 12-bit
		space = 240U;		// 160 samples 12-bit

	float netData[160U]; // Modem can handle up to 160 samples at a time
	unsigned int length = m_network->readData(netData, 160U);  // 160 samples 12-bit
	if (length == 0U)
		return 0U;

	unsigned int pack = 0U;
	unsigned char* packPointer = (unsigned char*)&pack;
	unsigned int nData = 0U;

	for (unsigned int i = 0; i < length; i++) {
		float sampleFloat = netData[i] * m_txAudioGain;

        // Pre-emphasis
        if (m_preEmphasisOn)
            sampleFloat = m_preEmphasis.filter(sampleFloat);

		// Convert float to 12-bit samples (0 to 4095)
		unsigned int sample12bit = (unsigned int)((sampleFloat + 1.0F) * 2048.0F + 0.5F);

		// Pack 2 samples into 3 bytes
		if ((i & 1U) == 0) {
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
}

void CFMControl::clock(unsigned int ms)
{
	// May not be needed
}

void CFMControl::enable(bool enabled)
{
	// May not be needed
}
