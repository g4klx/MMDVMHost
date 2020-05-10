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

#define EMPHASIS_GAIN_DB 0 //Gain needs to be the same for pre an deeemphasis

CFMControl::CFMControl(CFMNetwork* network) :
m_network(network),
m_enabled(false),
m_preemphasis(0.3889703087993727F, -0.3290005228984741F, 0.0F, 1.0F, 0.282029168302153F, 0.0F, EMPHASIS_GAIN_DB),
m_deemphasis(1.0F, 0.282029168302153F, 0.0F, 0.3889703087993727F, -0.3290005228984741F, 0.0F, EMPHASIS_GAIN_DB)
{
}

CFMControl::~CFMControl()
{
}

bool CFMControl::writeModem(const unsigned char* data, unsigned int length)
{
    assert(data != NULL);
    assert(length > 0U);

    if(m_network == NULL)
        return false;

    float samples[170U];
    unsigned int nSamples = 0U;

    // Unpack the serial data into float values.
    for (unsigned int i = 0U; i < length; i += 3U) {
        unsigned short sample1 = 0U;
        unsigned short sample2 = 0U;
        unsigned int MASK = 0x00000FFFU;

        unsigned int pack = 0U;
        unsigned char* packPointer = (unsigned char*)&pack;

        packPointer[1U] = data[i];
        packPointer[2U] = data[i + 1U];
        packPointer[3U] = data[i + 2U];

        sample2 = (short)(pack & MASK);
        sample1 = (short)(pack >> 12);

        // Convert from unsigned short (0 - +4095) to float (-1.0 - +1.0)
        samples[nSamples++] = (float(sample1) - 2048.0F) / 2048.0F;
        samples[nSamples++] = (float(sample2) - 2048.0F) / 2048.0F;
    }

    // De-emphasise the data and any other processing needed (maybe a low-pass filter to remove the CTCSS)
    for (unsigned int i = 0U; i < nSamples; i++)
        samples[i] = m_deemphasis.filter(samples[i]);

    unsigned char out[350U];
    unsigned int nOut = 0U;

    // Repack the data (8-bit unsigned values containing unsigned 16-bit data)
    for (unsigned int i = 0U; i < nSamples; i++) {
        unsigned short sample = (unsigned short)((samples[i] + 1.0F) * 32767.0F + 0.5F);
        out[nOut++] = (sample >> 8) & 0xFFU;
        out[nOut++] = (sample >> 0) & 0xFFU;
    }

    return m_network->write(out, nOut);
}

unsigned int CFMControl::readModem(unsigned char* data, unsigned int space)
{
    assert(data != NULL);
    assert(space > 0U);
    if(m_network == NULL)
        return 0;

    unsigned char netData[300U];
    unsigned int length = m_network->read(netData, 270U);
    if (length == 0U)
        return 0U;

    float samples[170U];
    unsigned int nSamples = 0U;

    // Convert the unsigned 16-bit data (+65535 - 0) to float (+1.0 - -1.0)
    for (unsigned int i = 0U; i < length; i += 2U) {
        unsigned short sample = (netData[i + 0U] << 8) | netData[i + 1U];
        samples[nSamples++] = (float(sample) / 32767.0F) - 1.0F;
    }

    // Pre-emphasise the data and other stuff.
    for (unsigned int i = 0U; i < nSamples; i++)
        samples[i] = m_preemphasis.filter(samples[i]);

    // Pack the floating point data (+1.0 to -1.0) to packed 12-bit samples (+2047 - -2048)
    unsigned int pack = 0U;
    unsigned char* packPointer = (unsigned char*)&pack;
    unsigned int j = 0U;
    unsigned int i = 0U;
    for (; i < nSamples && j < space; i += 2U, j += 3U) {
        unsigned short sample1 = (unsigned short)((samples[i] + 1.0F) * 2048.0F + 0.5F);
        unsigned short sample2 = (unsigned short)((samples[i + 1] + 1.0F) * 2048.0F + 0.5F);

        pack = 0;
        pack = ((unsigned int)sample1) << 12;
        pack |= sample2;

        data[j] = packPointer[1U];
        data[j + 1U] = packPointer[2U];
        data[j + 2U] = packPointer[3U];
    }

    return j;//return the number of bytes written
}

void CFMControl::clock(unsigned int ms)
{
    // May not be needed
}

void CFMControl::enable(bool enabled)
{
    // May not be needed
}
