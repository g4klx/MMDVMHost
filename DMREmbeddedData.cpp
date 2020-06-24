/*
 *   Copyright (C) 2015,2016,2017 by Jonathan Naylor G4KLX
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

#include "DMREmbeddedData.h"

#include "Hamming.h"
#include "Utils.h"
#include "CRC.h"

#include <cstdio>
#include <cassert>
#include <cstring>

CDMREmbeddedData::CDMREmbeddedData() :
m_raw(NULL),
m_state(LCS_NONE),
m_data(NULL),
m_FLCO(FLCO_GROUP),
m_valid(false)
{
	m_raw  = new bool[128U];
	m_data = new bool[72U];
}

CDMREmbeddedData::~CDMREmbeddedData()
{
	delete[] m_raw;
	delete[] m_data;
}

// Add LC data (which may consist of 4 blocks) to the data store
bool CDMREmbeddedData::addData(const unsigned char* data, unsigned char lcss)
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
			m_raw[a] = rawData[a + 4U];

		// Show we are ready for the next LC block
		m_state = LCS_FIRST;
		m_valid = false;

		return false;
	}

	// Is this the 2nd block of a 4 block embedded LC ?
	if (lcss == 3U && m_state == LCS_FIRST) {
		for (unsigned int a = 0U; a < 32U; a++)
			m_raw[a + 32U] = rawData[a + 4U];

		// Show we are ready for the next LC block
		m_state = LCS_SECOND;

		return false;
	}

	// Is this the 3rd block of a 4 block embedded LC ?
	if (lcss == 3U && m_state == LCS_SECOND) {
		for (unsigned int a = 0U; a < 32U; a++)
			m_raw[a + 64U] = rawData[a + 4U];

		// Show we are ready for the final LC block
		m_state = LCS_THIRD;

		return false;
	}

	// Is this the final block of a 4 block embedded LC ?
	if (lcss == 2U && m_state == LCS_THIRD)	{
		for (unsigned int a = 0U; a < 32U; a++)
			m_raw[a + 96U] = rawData[a + 4U];

		// Show that we're not ready for any more data
		m_state = LCS_NONE;

		// Process the complete data block
		decodeEmbeddedData();
		if (m_valid)
			encodeEmbeddedData();

		return m_valid;
	}

	return false;
}

void CDMREmbeddedData::setLC(const CDMRLC& lc)
{
	lc.getData(m_data);

	m_FLCO  = lc.getFLCO();
	m_valid = true;

	encodeEmbeddedData();
}

void CDMREmbeddedData::encodeEmbeddedData()
{
	unsigned int crc;
	CCRC::encodeFiveBit(m_data, crc);

	bool data[128U];
	::memset(data, 0x00U, 128U * sizeof(bool));

	data[106U] = (crc & 0x01U) == 0x01U;
	data[90U]  = (crc & 0x02U) == 0x02U;
	data[74U]  = (crc & 0x04U) == 0x04U;
	data[58U]  = (crc & 0x08U) == 0x08U;
	data[42U]  = (crc & 0x10U) == 0x10U;

	unsigned int b = 0U;
	for (unsigned int a = 0U; a < 11U; a++, b++)
		data[a] = m_data[b];
	for (unsigned int a = 16U; a < 27U; a++, b++)
		data[a] = m_data[b];
	for (unsigned int a = 32U; a < 42U; a++, b++)
		data[a] = m_data[b];
	for (unsigned int a = 48U; a < 58U; a++, b++)
		data[a] = m_data[b];
	for (unsigned int a = 64U; a < 74U; a++, b++)
		data[a] = m_data[b];
	for (unsigned int a = 80U; a < 90U; a++, b++)
		data[a] = m_data[b];
	for (unsigned int a = 96U; a < 106U; a++, b++)
		data[a] = m_data[b];

	// Hamming (16,11,4) check each row except the last one
	for (unsigned int a = 0U; a < 112U; a += 16U)
		CHamming::encode16114(data + a);

	// Add the parity bits for each column
	for (unsigned int a = 0U; a < 16U; a++)
		data[a + 112U] = data[a + 0U] ^ data[a + 16U] ^ data[a + 32U] ^ data[a + 48U] ^ data[a + 64U] ^ data[a + 80U] ^ data[a + 96U];

	// The data is packed downwards in columns
	b = 0U;
	for (unsigned int a = 0U; a < 128U; a++) {
		m_raw[a] = data[b];
		b += 16U;
		if (b > 127U)
			b -= 127U;
	}
}

unsigned char CDMREmbeddedData::getData(unsigned char* data, unsigned char n) const
{
	assert(data != NULL);

	if (n >= 1U && n < 5U) {
		n--;

		bool bits[40U];
		::memset(bits, 0x00U, 40U * sizeof(bool));
		::memcpy(bits + 4U, m_raw + n * 32U, 32U * sizeof(bool));

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
void CDMREmbeddedData::decodeEmbeddedData()
{
	// The data is unpacked downwards in columns
	bool data[128U];
	::memset(data, 0x00U, 128U * sizeof(bool));

	unsigned int b = 0U;
	for (unsigned int a = 0U; a < 128U; a++) {
		data[b] = m_raw[a];
		b += 16U;
		if (b > 127U)
			b -= 127U;
	}

	// Hamming (16,11,4) check each row except the last one
	for (unsigned int a = 0U; a < 112U; a += 16U) {
		if (!CHamming::decode16114(data + a))
			return;
	}

	// Check the parity bits
	for (unsigned int a = 0U; a < 16U; a++) {
		bool parity = data[a + 0U] ^ data[a + 16U] ^ data[a + 32U] ^ data[a + 48U] ^ data[a + 64U] ^ data[a + 80U] ^ data[a + 96U] ^ data[a + 112U];
		if (parity)
			return;
	}

	// We have passed the Hamming check so extract the actual payload
	b = 0U;
	for (unsigned int a = 0U; a < 11U; a++, b++)
		m_data[b] = data[a];
	for (unsigned int a = 16U; a < 27U; a++, b++)
		m_data[b] = data[a];
	for (unsigned int a = 32U; a < 42U; a++, b++)
		m_data[b] = data[a];
	for (unsigned int a = 48U; a < 58U; a++, b++)
		m_data[b] = data[a];
	for (unsigned int a = 64U; a < 74U; a++, b++)
		m_data[b] = data[a];
	for (unsigned int a = 80U; a < 90U; a++, b++)
		m_data[b] = data[a];
	for (unsigned int a = 96U; a < 106U; a++, b++)
		m_data[b] = data[a];

	// Extract the 5 bit CRC
	unsigned int crc = 0U;
	if (data[42])  crc += 16U;
	if (data[58])  crc += 8U;
	if (data[74])  crc += 4U;
	if (data[90])  crc += 2U;
	if (data[106]) crc += 1U;

	// Now CRC check this
	if (!CCRC::checkFiveBit(m_data, crc))
		return;

	m_valid = true;

	// Extract the FLCO
	unsigned char flco;
	CUtils::bitsToByteBE(m_data + 0U, flco);
	m_FLCO = FLCO(flco & 0x3FU);
}

CDMRLC* CDMREmbeddedData::getLC() const
{
	if (!m_valid)
		return NULL;

	if (m_FLCO != FLCO_GROUP && m_FLCO != FLCO_USER_USER)
		return NULL;

	return new CDMRLC(m_data);
}

bool CDMREmbeddedData::isValid() const
{
	return m_valid;
}

FLCO CDMREmbeddedData::getFLCO() const
{
	return m_FLCO;
}

void CDMREmbeddedData::reset()
{
	m_state = LCS_NONE;
	m_valid = false;
}

bool CDMREmbeddedData::getRawData(unsigned char* data) const
{
	assert(data != NULL);

	if (!m_valid)
		return false;

	CUtils::bitsToByteBE(m_data + 0U,  data[0U]);
	CUtils::bitsToByteBE(m_data + 8U,  data[1U]);
	CUtils::bitsToByteBE(m_data + 16U, data[2U]);
	CUtils::bitsToByteBE(m_data + 24U, data[3U]);
	CUtils::bitsToByteBE(m_data + 32U, data[4U]);
	CUtils::bitsToByteBE(m_data + 40U, data[5U]);
	CUtils::bitsToByteBE(m_data + 48U, data[6U]);
	CUtils::bitsToByteBE(m_data + 56U, data[7U]);
	CUtils::bitsToByteBE(m_data + 64U, data[8U]);

	return true;
}
