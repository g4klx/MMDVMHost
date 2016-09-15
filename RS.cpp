/*
*   Copyright (C) 2016 by Jonathan Naylor G4KLX
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

#include "RS.h"

#include <cstdio>
#include <cassert>

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

static int bin2Hex(const unsigned char* input, unsigned int offset)
{
	int output = 0;

	output |= READ_BIT(input, offset + 0U) ? 0x20 : 0x00;
	output |= READ_BIT(input, offset + 1U) ? 0x10 : 0x00;
	output |= READ_BIT(input, offset + 2U) ? 0x08 : 0x00;
	output |= READ_BIT(input, offset + 3U) ? 0x04 : 0x00;
	output |= READ_BIT(input, offset + 4U) ? 0x02 : 0x00;
	output |= READ_BIT(input, offset + 5U) ? 0x01 : 0x00;

	return output;
}

static void hex2Bin(int input, unsigned char* output, unsigned int offset)
{
	WRITE_BIT(output, offset + 0U, input & 0x20);
	WRITE_BIT(output, offset + 1U, input & 0x10);
	WRITE_BIT(output, offset + 2U, input & 0x08);
	WRITE_BIT(output, offset + 3U, input & 0x04);
	WRITE_BIT(output, offset + 4U, input & 0x02);
	WRITE_BIT(output, offset + 5U, input & 0x01);
}

// tt = (dd-1)/2
// dd = 17 --> tt = 8
CRS362017::CRS362017() :
CReedSolomon63<8>()
{
}

CRS362017::~CRS362017()
{
}

/**
* Does a Reed-Solomon decode adapting the input and output to the expected DSD data format.
* \param hex_data Data packed bits, originally a char[20][6], so containing 20 hex works, each char
*                 is a bit. Bits are corrected in place.
* \param hex_parity Parity packed bits, originally a char[16][6], 16 hex words.
* \return 1 if irrecoverable errors have been detected, 0 otherwise.
*/
bool CRS362017::decode(unsigned char* data)
{
	assert(data != NULL);

	int input[63];
	int output[63];

	// Fill up with zeros to complete the 47 expected hex words of data
	for (unsigned int i = 0U; i < 63U; i++)
		input[i] = 0;

	// First put the parity data, 16 hex words
	unsigned int offset = 120U;
	for (unsigned int i = 0U; i < 16U; i++, offset += 6U)
		input[i] = bin2Hex(data, offset);

	// Then the 20 hex words of data
	offset = 0U;
	for (unsigned int i = 16U; i < 36U; i++, offset += 6U)
		input[i] = bin2Hex(data, offset);

	// Now we can call decode on the base class
	int irrecoverable_errors = CReedSolomon63<8>::decode(input, output);
	if (irrecoverable_errors != 0)
		return false;

	// Convert it back to binary and put it into hex_data.
	offset = 0U;
	for (unsigned int i = 16U; i < 36U; i++, offset += 6U)
		hex2Bin(output[i], data, offset);

	return true;
}

void CRS362017::encode(unsigned char* data)
{
	assert(data != NULL);

	int input[47];
	int output[63];

	// Fill up with zeros to complete the 47 expected hex words of data
	for (unsigned int i = 0U; i < 47U; i++)
		input[i] = 0;

	// Put the 20 hex words of data
	unsigned int offset = 0U;
	for (unsigned int i = 0U; i < 20U; i++, offset += 6U)
		input[i] = bin2Hex(data, offset);

	// Now we can call encode on the base class
	CReedSolomon63<8>::encode(input, output);

	// Convert it back to binary form and put it into the parity
	offset = 120U;
	for (unsigned int i = 0U; i < 16U; i++, offset += 6U)
		hex2Bin(output[i], data, offset);
}

// tt = (dd-1)/2
// dd = 13 --> tt = 6
CRS241213::CRS241213() :
CReedSolomon63<6>()
{
}

CRS241213::~CRS241213()
{
}

/**
* Does a Reed-Solomon decode adapting the input and output to the expected DSD data format.
* \param hex_data Data packed bits, originally a char[12][6], so containing 12 hex works, each char
*                 is a bit. Bits are corrected in place.
* \param hex_parity Parity packed bits, originally a char[12][6], 12 hex words.
* \return 1 if irrecoverable errors have been detected, 0 otherwise.
*/
bool CRS241213::decode(unsigned char* data)
{
	assert(data != NULL);

	int input[63];
	int output[63];

	// Fill up with zeros to complete the 51 expected hex words of data
	for (unsigned int i = 0U; i < 63U; i++)
		input[i] = 0;

	// First put the parity data, 12 hex words
	unsigned int offset = 72U;
	for (unsigned int i = 0U; i < 12U; i++, offset += 6U)
		input[i] = bin2Hex(data, offset);

	// Then the 12 hex words of data
	offset = 0U;
	for (unsigned int i = 12U; i < 24U; i++, offset += 6U)
		input[i] = bin2Hex(data, offset);

	// Now we can call decode on the base class
	int irrecoverable_errors = CReedSolomon63<6>::decode(input, output);
	if (irrecoverable_errors != 0)
		return false;

	// Convert it back to binary and put it into hex_data.
	offset = 0U;
	for (unsigned int i = 12U; i < 24U; i++, offset += 6U)
		hex2Bin(output[i], data, offset);

	return true;
}

void CRS241213::encode(unsigned char* data)
{
	assert(data != NULL);

	int input[51];
	int output[63];

	// Fill up with zeros to complete the 51 expected hex words of data
	for (unsigned int i = 0U; i < 51U; i++)
		input[i] = 0;

	// Put the 12 hex words of data
	unsigned int offset = 0U;
	for (unsigned int i = 0U; i < 12U; i++, offset += 6U)
		input[i] = bin2Hex(data, offset);

	// Now we can call encode on the base class
	CReedSolomon63<6>::encode(input, output);

	// Convert it back to binary form and put it into the parity
	offset = 72U;
	for (unsigned int i = 0U; i < 12U; i++, offset += 6U)
		hex2Bin(output[i], data, offset);
}

// tt = (dd-1)/2
// dd = 9 --> tt = 4
CRS24169::CRS24169() :
CReedSolomon63<4>()
{
}

CRS24169::~CRS24169()
{
}

/**
* Does a Reed-Solomon decode adapting the input and output to the expected DSD data format.
* \param hex_data Data packed bits, originally a char[16][6], so containing 16 hex works, each char
*                 is a bit. Bits are corrected in place.
* \param hex_parity Parity packed bits, originally a char[8][6], 8 hex words.
* \return 1 if irrecoverable errors have been detected, 0 otherwise.
*/
bool CRS24169::decode(unsigned char* data)
{
	assert(data != NULL);

	int input[63];
	int output[63];

	// Fill up with zeros to complete the 55 expected hex words of data
	for (unsigned int i = 0U; i < 63U; i++)
		input[i] = 0;

	// First put the parity data, 8 hex words
	unsigned int offset = 96U;
	for (unsigned int i = 0U; i < 8U; i++, offset += 6U)
		input[i] = bin2Hex(data, offset);

	// Then the 16 hex words of data
	offset = 0U;
	for (unsigned int i = 8U; i < 24U; i++, offset += 6U)
		input[i] = bin2Hex(data, offset);

	// Now we can call decode on the base class
	int irrecoverable_errors = CReedSolomon63<4>::decode(input, output);
	if (irrecoverable_errors != 0)
		return false;

	// Convert it back to binary and put it into hex_data.
	offset = 0U;
	for (unsigned int i = 8U; i < 24U; i++, offset += 6U)
		hex2Bin(output[i], data, offset);

	return true;
}

void CRS24169::encode(unsigned char* data)
{
	assert(data != NULL);

	int input[55];
	int output[63];

	// Fill up with zeros to complete the 55 expected hex words of data
	for (unsigned int i = 0U; i < 55U; i++)
		input[i] = 0;

	// Put the 16 hex words of data
	unsigned int offset = 0U;
	for (unsigned int i = 0U; i < 16U; i++, offset += 6U)
		input[i] = bin2Hex(data, offset);

	// Now we can call encode on the base class
	CReedSolomon63<4>::encode(input, output);

	// Convert it back to binary form and put it into the parity
	offset = 96U;
	for (unsigned int i = 0U; i < 8U; i++, offset += 6U)
		hex2Bin(output[i], data, offset);
}
