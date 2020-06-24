/*
*   Copyright (C) 2018 by Jonathan Naylor G4KLX
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

#include "NXDNSACCH.h"

#include "NXDNConvolution.h"
#include "NXDNDefines.h"
#include "NXDNCRC.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned int INTERLEAVE_TABLE[] = {
	0U, 5U, 10U, 15U, 20U, 25U, 30U, 35U, 40U, 45U, 50U, 55U,
	1U, 6U, 11U, 16U, 21U, 26U, 31U, 36U, 41U, 46U, 51U, 56U,
	2U, 7U, 12U, 17U, 22U, 27U, 32U, 37U, 42U, 47U, 52U, 57U,
	3U, 8U, 13U, 18U, 23U, 28U, 33U, 38U, 43U, 48U, 53U, 58U,
	4U, 9U, 14U, 19U, 24U, 29U, 34U, 39U, 44U, 49U, 54U, 59U
};

const unsigned int PUNCTURE_LIST[] = { 5U, 11U, 17U, 23U, 29U, 35U, 41U, 47U, 53U, 59U, 65U, 71U };

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CNXDNSACCH::CNXDNSACCH(const CNXDNSACCH& sacch) :
m_data(NULL)
{
	m_data = new unsigned char[5U];
	::memcpy(m_data, sacch.m_data, 5U);
}

CNXDNSACCH::CNXDNSACCH() :
m_data(NULL)
{
	m_data = new unsigned char[5U];
}

CNXDNSACCH::~CNXDNSACCH()
{
	delete[] m_data;
}

bool CNXDNSACCH::decode(const unsigned char* data)
{
	assert(data != NULL);

	unsigned char temp1[8U];

	for (unsigned int i = 0U; i < NXDN_SACCH_LENGTH_BITS; i++) {
		unsigned int n = INTERLEAVE_TABLE[i] + NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS;
		bool b = READ_BIT1(data, n);
		WRITE_BIT1(temp1, i, b);
	}

	uint8_t temp2[90U];

	unsigned int n = 0U;
	unsigned int index = 0U;
	for (unsigned int i = 0U; i < NXDN_SACCH_LENGTH_BITS; i++) {
		if (n == PUNCTURE_LIST[index]) {
			temp2[n++] = 1U;
			index++;
		}

		bool b = READ_BIT1(temp1, i);
		temp2[n++] = b ? 2U : 0U;
	}

	for (unsigned int i = 0U; i < 8U; i++) {
		temp2[n++] = 0U;
	}

	CNXDNConvolution conv;
	conv.start();

	n = 0U;
	for (unsigned int i = 0U; i < 40U; i++) {
		uint8_t s0 = temp2[n++];
		uint8_t s1 = temp2[n++];

		conv.decode(s0, s1);
	}

	conv.chainback(m_data, 36U);

	return CNXDNCRC::checkCRC6(m_data, 26U);
}

void CNXDNSACCH::encode(unsigned char* data) const
{
	assert(data != NULL);

	unsigned char temp1[5U];
	::memset(temp1, 0x00U, 5U);

	for (unsigned int i = 0U; i < 26U; i++) {
		bool b = READ_BIT1(m_data, i);
		WRITE_BIT1(temp1, i, b);
	}

	CNXDNCRC::encodeCRC6(temp1, 26U);

	unsigned char temp2[9U];

	CNXDNConvolution conv;
	conv.encode(temp1, temp2, 36U);

	unsigned char temp3[8U];

	unsigned int n = 0U;
	unsigned int index = 0U;
	for (unsigned int i = 0U; i < 72U; i++) {
		if (i != PUNCTURE_LIST[index]) {
			bool b = READ_BIT1(temp2, i);
			WRITE_BIT1(temp3, n, b);
			n++;
		} else {
			index++;
		}
	}

	for (unsigned int i = 0U; i < NXDN_SACCH_LENGTH_BITS; i++) {
		unsigned int n = INTERLEAVE_TABLE[i] + NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS;
		bool b = READ_BIT1(temp3, i);
		WRITE_BIT1(data, n, b);
	}
}

unsigned char CNXDNSACCH::getRAN() const
{
	return m_data[0U] & 0x3FU;
}

unsigned char CNXDNSACCH::getStructure() const
{
	return (m_data[0U] >> 6) & 0x03U;
}

void CNXDNSACCH::getData(unsigned char* data) const
{
	assert(data != NULL);

	unsigned int offset = 8U;
	for (unsigned int i = 0U; i < 18U; i++, offset++) {
		bool b = READ_BIT1(m_data, offset);
		WRITE_BIT1(data, i, b);
	}
}

void CNXDNSACCH::getRaw(unsigned char* data) const
{
	assert(data != NULL);

	::memcpy(data, m_data, 4U);

	CNXDNCRC::encodeCRC6(data, 26U);
}

void CNXDNSACCH::setRAN(unsigned char ran)
{
	m_data[0U] &= 0xC0U;
	m_data[0U] |= ran;
}

void CNXDNSACCH::setStructure(unsigned char structure)
{
	m_data[0U] &= 0x3FU;
	m_data[0U] |= (structure << 6) & 0xC0U;
}

void CNXDNSACCH::setData(const unsigned char* data)
{
	assert(data != NULL);

	unsigned int offset = 8U;
	for (unsigned int i = 0U; i < 18U; i++, offset++) {
		bool b = READ_BIT1(data, i);
		WRITE_BIT1(m_data, offset, b);
	}
}

void CNXDNSACCH::setRaw(const unsigned char* data)
{
	assert(data != NULL);

	::memcpy(m_data, data, 4U);
}

CNXDNSACCH& CNXDNSACCH::operator=(const CNXDNSACCH& sacch)
{
	if (&sacch != this)
		::memcpy(m_data, sacch.m_data, 5U);

	return *this;
}
