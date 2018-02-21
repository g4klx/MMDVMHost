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

#include "NXDNFACCH1.h"

#include "NXDNConvolution.h"
#include "NXDNDefines.h"
#include "NXDNCRC.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned int INTERLEAVE_TABLE[] = {
	0U,  9U, 18U, 27U, 36U, 45U, 54U, 63U, 72U, 81U, 90U,  99U, 108U, 117U, 126U, 135U,
	1U, 10U, 19U, 28U, 37U, 46U, 55U, 64U, 73U, 82U, 91U, 100U, 109U, 118U, 127U, 136U,
	2U, 11U, 20U, 29U, 38U, 47U, 56U, 65U, 74U, 83U, 92U, 101U, 110U, 119U, 128U, 137U,
	3U, 12U, 21U, 30U, 39U, 48U, 57U, 66U, 75U, 84U, 93U, 102U, 111U, 120U, 129U, 138U,
	4U, 13U, 22U, 31U, 40U, 49U, 58U, 67U, 76U, 85U, 94U, 103U, 112U, 121U, 130U, 139U,
	5U, 14U, 23U, 32U, 41U, 50U, 59U, 68U, 77U, 86U, 95U, 104U, 113U, 122U, 131U, 140U,
	6U, 15U, 24U, 33U, 42U, 51U, 60U, 69U, 78U, 87U, 96U, 105U, 114U, 123U, 132U, 141U,
	7U, 16U, 25U, 34U, 43U, 52U, 61U, 70U, 79U, 88U, 97U, 106U, 115U, 124U, 133U, 142U,
	8U, 17U, 26U, 35U, 44U, 53U, 62U, 71U, 80U, 89U, 98U, 107U, 116U, 125U, 134U, 143U };

const unsigned int PUNCTURE_LIST[] = {  1U,   5U,   9U,  13U,  17U,  21U,  25U,  29U,  33U,  37U, 
									   41U,  45U,  49U,  53U,  57U,  61U,  65U,  69U,  73U,  77U, 
									   81U,  85U,  89U,  93U,  97U, 101U, 105U, 109U, 113U, 117U,
									  121U, 125U, 129U, 133U, 137U, 141U, 145U, 149U, 153U, 157U,
									  161U, 165U, 169U, 173U, 177U, 181U, 185U, 189U };

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CNXDNFACCH1::CNXDNFACCH1(const CNXDNFACCH1& facch1) :
m_data(NULL)
{
	m_data = new unsigned char[10U + 2U];
	::memcpy(m_data, facch1.m_data, 10U + 2U);
}

CNXDNFACCH1::CNXDNFACCH1() :
m_data(NULL)
{
	m_data = new unsigned char[10U + 2U];
}

CNXDNFACCH1::~CNXDNFACCH1()
{
	delete[] m_data;
}

bool CNXDNFACCH1::decode(const unsigned char* data, unsigned int offset)
{
	assert(data != NULL);

	unsigned char temp1[18U];

	for (unsigned int i = 0U; i < NXDN_FACCH1_LENGTH_BITS; i++) {
		unsigned int n = INTERLEAVE_TABLE[i] + offset;
		bool b = READ_BIT1(data, n);
		WRITE_BIT1(temp1, i, b);
	}

	uint8_t temp2[210U];

	unsigned int n = 0U;
	unsigned int index = 0U;
	for (unsigned int i = 0U; i < NXDN_FACCH1_LENGTH_BITS; i++) {
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
	for (unsigned int i = 0U; i < 100U; i++) {
		uint8_t s0 = temp2[n++];
		uint8_t s1 = temp2[n++];

		conv.decode(s0, s1);
	}

	conv.chainback(m_data, 96U);

	return CNXDNCRC::checkCRC12(m_data, 80U);
}

void CNXDNFACCH1::encode(unsigned char* data, unsigned int offset) const
{
	assert(data != NULL);

	unsigned char temp1[12U];
	::memset(temp1, 0x00U, 12U);
	::memcpy(temp1, m_data, 10U);

	CNXDNCRC::encodeCRC12(temp1, 80U);

	unsigned char temp2[24U];

	CNXDNConvolution conv;
	conv.encode(temp1, temp2, 96U);

	unsigned char temp3[18U];

	unsigned int n = 0U;
	unsigned int index = 0U;
	for (unsigned int i = 0U; i < 192U; i++) {
		if (i != PUNCTURE_LIST[index]) {
			bool b = READ_BIT1(temp2, i);
			WRITE_BIT1(temp3, n, b);
			n++;
		} else {
			index++;
		}
	}

	for (unsigned int i = 0U; i < NXDN_FACCH1_LENGTH_BITS; i++) {
		unsigned int n = INTERLEAVE_TABLE[i] + offset;
		bool b = READ_BIT1(temp3, i);
		WRITE_BIT1(data, n, b);
	}
}

void CNXDNFACCH1::getData(unsigned char* data) const
{
	assert(data != NULL);

	::memcpy(data, m_data, 10U);
}

void CNXDNFACCH1::getRaw(unsigned char* data) const
{
	assert(data != NULL);

	::memset(data, 0x00U, 12U);
	::memcpy(data, m_data, 10U);

	CNXDNCRC::encodeCRC12(data, 80U);
}

void CNXDNFACCH1::setData(const unsigned char* data)
{
	assert(data != NULL);

	::memcpy(m_data, data, 10U);
}

void CNXDNFACCH1::setRaw(const unsigned char* data)
{
	assert(data != NULL);

	::memcpy(m_data, data, 12U);
}

CNXDNFACCH1& CNXDNFACCH1::operator=(const CNXDNFACCH1& facch1)
{
	if (&facch1 != this)
		::memcpy(m_data, facch1.m_data, 10U + 2U);

	return *this;
}
