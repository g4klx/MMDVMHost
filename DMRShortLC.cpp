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

#include "DMRShortLC.h"

#include "Hamming.h"
#include "Utils.h"

#include <cstdio>
#include <cassert>
#include <cstring>

CDMRShortLC::CDMRShortLC() :
m_rawData(NULL),
m_deInterData(NULL)
{
	m_rawData     = new bool[72U];
	m_deInterData = new bool[68U];
}

CDMRShortLC::~CDMRShortLC()
{
	delete[] m_rawData;
	delete[] m_deInterData;
}

// The main decode function
bool CDMRShortLC::decode(const unsigned char* in, unsigned char* out)
{
	assert(in != NULL);
	assert(out != NULL);

	//  Get the raw binary
	decodeExtractBinary(in);

	// Deinterleave
	decodeDeInterleave();

	// Error check
	bool ret = decodeErrorCheck();
	if (!ret)
		return false;

	// Extract Data
	decodeExtractData(out);

	return true;
}

// The main encode function
void CDMRShortLC::encode(const unsigned char* in, unsigned char* out)
{
	assert(in != NULL);
	assert(out != NULL);

	// Extract Data
	encodeExtractData(in);

	// Error check
	encodeErrorCheck();

	// Deinterleave
	encodeInterleave();

	//  Get the raw binary
	encodeExtractBinary(out);
}

void CDMRShortLC::decodeExtractBinary(const unsigned char* in)
{
	assert(in != NULL);

	CUtils::byteToBitsBE(in[0U], m_rawData + 0U);
	CUtils::byteToBitsBE(in[1U], m_rawData + 8U);
	CUtils::byteToBitsBE(in[2U], m_rawData + 16U);
	CUtils::byteToBitsBE(in[3U], m_rawData + 24U);
	CUtils::byteToBitsBE(in[4U], m_rawData + 32U);
	CUtils::byteToBitsBE(in[5U], m_rawData + 40U);
	CUtils::byteToBitsBE(in[6U], m_rawData + 48U);
	CUtils::byteToBitsBE(in[7U], m_rawData + 56U);
	CUtils::byteToBitsBE(in[8U], m_rawData + 64U);
}

// Deinterleave the raw data
void CDMRShortLC::decodeDeInterleave()
{
	for (unsigned int i = 0U; i < 68U; i++)
		m_deInterData[i] = false;

	for (unsigned int a = 0U; a < 67U; a++)	{
		// Calculate the interleave sequence
		unsigned int interleaveSequence = (a * 4U) % 67U;
		// Shuffle the data
		m_deInterData[a] = m_rawData[interleaveSequence];
	}

	m_deInterData[67U] = m_rawData[67U];
}
	
// Check each row with a Hamming (17,12,3) code and each column with a parity bit
bool CDMRShortLC::decodeErrorCheck()
{
	// Run through each of the 3 rows containing data
	CHamming::decode17123(m_deInterData + 0U);
	CHamming::decode17123(m_deInterData + 17U);
	CHamming::decode17123(m_deInterData + 34U);

	// Run through each of the 17 columns
	for (unsigned int c = 0U; c < 17U; c++) {
		bool bit = m_deInterData[c + 0U] ^ m_deInterData[c + 17U] ^ m_deInterData[c + 34U];
		if (bit != m_deInterData[c + 51U])
			return false;
	}

	return true;
}

// Extract the 36 bits of payload
void CDMRShortLC::decodeExtractData(unsigned char* data) const
{
	assert(data != NULL);

	bool bData[40U];

	for (unsigned int i = 0U; i < 40U; i++)
		bData[i] = false;

	unsigned int pos = 4U;
	for (unsigned int a = 0U; a < 12U; a++, pos++)
		bData[pos] = m_deInterData[a];

	for (unsigned int a = 17U; a < 29U; a++, pos++)
		bData[pos] = m_deInterData[a];

	for (unsigned int a = 34U; a < 46U; a++, pos++)
		bData[pos] = m_deInterData[a];

	CUtils::bitsToByteBE(bData + 0U,  data[0U]);
	CUtils::bitsToByteBE(bData + 8U,  data[1U]);
	CUtils::bitsToByteBE(bData + 16U, data[2U]);
	CUtils::bitsToByteBE(bData + 24U, data[3U]);
	CUtils::bitsToByteBE(bData + 32U, data[4U]);
}

// Extract the 36 bits of payload
void CDMRShortLC::encodeExtractData(const unsigned char* in) const
{
	assert(in != NULL);

	bool bData[40U];
	CUtils::byteToBitsBE(in[0U], bData + 0U);
	CUtils::byteToBitsBE(in[1U], bData + 8U);
	CUtils::byteToBitsBE(in[2U], bData + 16U);
	CUtils::byteToBitsBE(in[3U], bData + 24U);
	CUtils::byteToBitsBE(in[4U], bData + 32U);

	for (unsigned int i = 0U; i < 68U; i++)
		m_deInterData[i] = false;

	unsigned int pos = 4U;
	for (unsigned int a = 0U; a < 12U; a++, pos++)
		m_deInterData[a] = bData[pos];

	for (unsigned int a = 17U; a < 29U; a++, pos++)
		m_deInterData[a] = bData[pos];

	for (unsigned int a = 34U; a < 46U; a++, pos++)
		m_deInterData[a] = bData[pos];
}

// Check each row with a Hamming (17,12,3) code and each column with a parity bit
void CDMRShortLC::encodeErrorCheck()
{
	// Run through each of the 3 rows containing data
	CHamming::encode17123(m_deInterData + 0U);
	CHamming::encode17123(m_deInterData + 17U);
	CHamming::encode17123(m_deInterData + 34U);

	// Run through each of the 17 columns
	for (unsigned int c = 0U; c < 17U; c++)
		m_deInterData[c + 51U] = m_deInterData[c + 0U] ^ m_deInterData[c + 17U] ^ m_deInterData[c + 34U];
}

// Interleave the raw data
void CDMRShortLC::encodeInterleave()
{
	for (unsigned int i = 0U; i < 72U; i++)
		m_rawData[i] = false;

	for (unsigned int a = 0U; a < 67U; a++)	{
		// Calculate the interleave sequence
		unsigned int interleaveSequence = (a * 4U) % 67U;
		// Unshuffle the data
		m_rawData[interleaveSequence] = m_deInterData[a];
	}

	m_rawData[67U] = m_deInterData[67U];
}

void CDMRShortLC::encodeExtractBinary(unsigned char* data)
{
	assert(data != NULL);

	CUtils::bitsToByteBE(m_rawData + 0U,  data[0U]);
	CUtils::bitsToByteBE(m_rawData + 8U,  data[1U]);
	CUtils::bitsToByteBE(m_rawData + 16U, data[2U]);
	CUtils::bitsToByteBE(m_rawData + 24U, data[3U]);
	CUtils::bitsToByteBE(m_rawData + 32U, data[4U]);
	CUtils::bitsToByteBE(m_rawData + 40U, data[5U]);
	CUtils::bitsToByteBE(m_rawData + 48U, data[6U]);
	CUtils::bitsToByteBE(m_rawData + 56U, data[7U]);
	CUtils::bitsToByteBE(m_rawData + 64U, data[8U]);
}
