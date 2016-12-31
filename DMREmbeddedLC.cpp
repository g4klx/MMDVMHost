/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
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

#include "DMREmbeddedLC.h"

#include "Hamming.h"
#include "Utils.h"
#include "CRC.h"

#include <cstdio>
#include <cassert>
#include <cstring>

CDMREmbeddedLC::CDMREmbeddedLC() :
m_rawLC(NULL),
m_state(LCS_NONE)
{
	// Allow for multi-block non embedded LC data
	m_rawLC = new bool[300U];
}

CDMREmbeddedLC::~CDMREmbeddedLC()
{
	delete[] m_rawLC;
}

// Add LC data (which may consist of 4 blocks) to the data store
CDMRLC* CDMREmbeddedLC::addData(const unsigned char* data, unsigned char lcss)
{
	assert(data != NULL);

	bool rawData[40U];
	CUtils::byteToBitsBE(data[14U], rawData + 0U);
	CUtils::byteToBitsBE(data[15U], rawData + 8U);
	CUtils::byteToBitsBE(data[16U], rawData + 16U);
	CUtils::byteToBitsBE(data[17U], rawData + 24U);
	CUtils::byteToBitsBE(data[18U], rawData + 32U);

	// Is this the first block of a 4 block embedded LC ?
	if (lcss == 1U) {
		for (unsigned int a = 0U; a < 32U; a++)
			m_rawLC[a] = rawData[a + 4U];

		// Show we are ready for the next LC block
		m_state = LCS_FIRST;
		return NULL;
	}

	// Is this the 2nd block of a 4 block embedded LC ?
	if (lcss == 3U && m_state == LCS_FIRST) {
		for (unsigned int a = 0U; a < 32U; a++)
			m_rawLC[a + 32U] = rawData[a + 4U];

		// Show we are ready for the next LC block
		m_state = LCS_SECOND;
		return NULL;
	}

	// Is this the 3rd block of a 4 block embedded LC ?
	if (lcss == 3U && m_state == LCS_SECOND) {
		for (unsigned int a = 0U; a < 32U; a++)
			m_rawLC[a + 64U] = rawData[a + 4U];

		// Show we are ready for the final LC block
		m_state = LCS_THIRD;
		return NULL;
	}

	// Is this the final block of a 4 block embedded LC ?
	if (lcss == 2U && m_state == LCS_THIRD)	{
		for (unsigned int a = 0U; a < 32U; a++)
			m_rawLC[a + 96U] = rawData[a + 4U];

		// Process the complete data block
		return processMultiBlockEmbeddedLC();
	}

	// Is this a single block embedded LC
	if (lcss == 0U) {
		processSingleBlockEmbeddedLC(rawData + 4U);
		return NULL;
	}

	return NULL;
}

void CDMREmbeddedLC::setData(const CDMRLC& lc)
{
	bool lcData[72U];
	lc.getData(lcData);
	
	unsigned int crc;
	CCRC::encodeFiveBit(lcData, crc);

	bool data[128U];
	::memset(data, 0x00U, 128U * sizeof(bool));

	data[106U] = (crc & 0x01U) == 0x01U;
	data[90U]  = (crc & 0x02U) == 0x02U;
	data[74U]  = (crc & 0x04U) == 0x04U;
	data[58U]  = (crc & 0x08U) == 0x08U;
	data[42U]  = (crc & 0x10U) == 0x10U;

	unsigned int b = 0U;
	for (unsigned int a = 0U; a < 11U; a++, b++)
		data[a] = lcData[b];
	for (unsigned int a = 16U; a < 27U; a++, b++)
		data[a] = lcData[b];
	for (unsigned int a = 32U; a < 42U; a++, b++)
		data[a] = lcData[b];
	for (unsigned int a = 48U; a < 58U; a++, b++)
		data[a] = lcData[b];
	for (unsigned int a = 64U; a < 74U; a++, b++)
		data[a] = lcData[b];
	for (unsigned int a = 80U; a < 90U; a++, b++)
		data[a] = lcData[b];
	for (unsigned int a = 96U; a < 106U; a++, b++)
		data[a] = lcData[b];

	// Hamming (16,11,4) check each row except the last one
	for (unsigned int a = 0U; a < 112U; a += 16U)
		CHamming::encode16114(data + a);

	// Add the parity bits for each column
	for (unsigned int a = 0U; a < 16U; a++)
		data[a + 112U] = data[a + 0U] ^ data[a + 16U] ^ data[a + 32U] ^ data[a + 48U] ^ data[a + 64U] ^ data[a + 80U] ^ data[a + 96U];

	// The data is packed downwards in columns
	b = 0U;
	for (unsigned int a = 0U; a < 128U; a++) {
		m_rawLC[a] = data[b];
		b += 16U;
		if (b > 127U)
			b -= 127U;
	}
}

unsigned char CDMREmbeddedLC::getData(unsigned char* data, unsigned char n) const
{
	assert(data != NULL);

	if (n >= 1U && n < 5U) {
		n--;

		bool bits[40U];
		::memset(bits, 0x00U, 40U * sizeof(bool));
		::memcpy(bits + 4U, m_rawLC + n * 32U, 32U * sizeof(bool));

		unsigned char bytes[5U];
		CUtils::bitsToByteBE(bits + 0U,  bytes[0U]);
		CUtils::bitsToByteBE(bits + 8U,  bytes[1U]);
		CUtils::bitsToByteBE(bits + 16U, bytes[2U]);
		CUtils::bitsToByteBE(bits + 24U, bytes[3U]);
		CUtils::bitsToByteBE(bits + 32U, bytes[4U]);

		data[14U] = (data[14U] & 0xF0U) | (bytes[0U] & 0x0FU);
		data[15U] = bytes[1U];
		data[16U] = bytes[2U];
		data[17U] = bytes[3U];
		data[18U] = (data[18U] & 0x0FU) | (bytes[4U] & 0xF0U);

		switch (n) {
		case 0U:
			return 1U;
		case 3U:
			return 2U;
		default:
			return 3U;
		}
	} else {
		data[14U] &= 0xF0U;
		data[15U]  = 0x00U;
		data[16U]  = 0x00U;
		data[17U]  = 0x00U;
		data[18U] &= 0x0FU;

		return 0U;
	}
}

// Unpack and error check an embedded LC
CDMRLC* CDMREmbeddedLC::processMultiBlockEmbeddedLC()
{
	// The data is unpacked downwards in columns
	bool data[128U];
	::memset(data, 0x00U, 128U * sizeof(bool));

	unsigned int b = 0U;
	for (unsigned int a = 0U; a < 128U; a++) {
		data[b] = m_rawLC[a];
		b += 16U;
		if (b > 127U)
			b -= 127U;
	}

	// Hamming (16,11,4) check each row except the last one
	for (unsigned int a = 0U; a < 112U; a += 16U) {
		if (!CHamming::decode16114(data + a))
			return NULL;
	}

	// Check the parity bits
	for (unsigned int a = 0U; a < 16U; a++) {
		bool parity = data[a + 0U] ^ data[a + 16U] ^ data[a + 32U] ^ data[a + 48U] ^ data[a + 64U] ^ data[a + 80U] ^ data[a + 96U] ^ data[a + 112U];
		if (parity)
			return NULL;
	}

	// We have passed the Hamming check so extract the actual payload
	bool lcData[72U];
	b = 0U;
	for (unsigned int a = 0U; a < 11U; a++, b++)
		lcData[b] = data[a];
	for (unsigned int a = 16U; a < 27U; a++, b++)
		lcData[b] = data[a];
	for (unsigned int a = 32U; a < 42U; a++, b++)
		lcData[b] = data[a];
	for (unsigned int a = 48U; a < 58U; a++, b++)
		lcData[b] = data[a];
	for (unsigned int a = 64U; a < 74U; a++, b++)
		lcData[b] = data[a];
	for (unsigned int a = 80U; a < 90U; a++, b++)
		lcData[b] = data[a];
	for (unsigned int a = 96U; a < 106U; a++, b++)
		lcData[b] = data[a];

	// Extract the 5 bit CRC
	unsigned int crc = 0U;
	if (data[42])  crc += 16U;
	if (data[58])  crc += 8U;
	if (data[74])  crc += 4U;
	if (data[90])  crc += 2U;
	if (data[106]) crc += 1U;

	// Now CRC check this
	if (!CCRC::checkFiveBit(lcData, crc))
		return NULL;

	CDMRLC* lc = new CDMRLC(lcData);

	// Only generate the LC when it's the correct FLCO
	switch (lc->getFLCO()) {
	case FLCO_GROUP:
	case FLCO_USER_USER:
		return lc;
	default:
		delete lc;
		return NULL;
	}
}

// Deal with a single block embedded LC
void CDMREmbeddedLC::processSingleBlockEmbeddedLC(const bool* data)
{
	// Nothing interesting, or just NULL (I think)
}
