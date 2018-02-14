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

#include "NXDNCRC.h"

#include <cstdio>
#include <cassert>

const uint8_t  BIT_MASK_TABLE1[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE1[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE1[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE1[(i)&7])

bool CNXDNCRC::checkCRC6(const unsigned char* in, unsigned int length)
{
	assert(in != NULL);

	uint8_t crc = createCRC6(in, length);

	uint8_t temp[1U];
	temp[0U] = 0x00U;
	unsigned int j = length;
	for (unsigned int i = 2U; i < 8U; i++, j++) {
		bool b = READ_BIT1(in, j);
		WRITE_BIT1(temp, i, b);
	}

	return crc == temp[0U];
}

void CNXDNCRC::encodeCRC6(unsigned char* in, unsigned int length)
{
	assert(in != NULL);

	uint8_t crc[1U];
	crc[0U] = createCRC6(in, length);

	unsigned int n = length;
	for (unsigned int i = 2U; i < 8U; i++, n++) {
		bool b = READ_BIT1(crc, i);
		WRITE_BIT1(in, n, b);
	}
}

bool CNXDNCRC::checkCRC12(const unsigned char* in, unsigned int length)
{
	assert(in != NULL);

	uint16_t crc = createCRC12(in, length);
	uint8_t temp1[2U];
	temp1[0U] = (crc >> 8) & 0xFFU;
	temp1[1U] = (crc >> 0) & 0xFFU;

	uint8_t temp2[2U];
	temp2[0U] = 0x00U;
	temp2[1U] = 0x00U;
	unsigned int j = length;
	for (unsigned int i = 4U; i < 16U; i++, j++) {
		bool b = READ_BIT1(in, j);
		WRITE_BIT1(temp2, i, b);
	}

	return temp1[0U] == temp2[0U] && temp1[1U] == temp2[1U];
}

void CNXDNCRC::encodeCRC12(unsigned char* in, unsigned int length)
{
	assert(in != NULL);

	uint16_t crc = createCRC12(in, length);

	uint8_t temp[2U];
	temp[0U] = (crc >> 8) & 0xFFU;
	temp[1U] = (crc >> 0) & 0xFFU;

	unsigned int n = length;
	for (unsigned int i = 4U; i < 16U; i++, n++) {
		bool b = READ_BIT1(temp, i);
		WRITE_BIT1(in, n, b);
	}
}

bool CNXDNCRC::checkCRC15(const unsigned char* in, unsigned int length)
{
	assert(in != NULL);

	uint16_t crc = createCRC15(in, length);
	uint8_t temp1[2U];
	temp1[0U] = (crc >> 8) & 0xFFU;
	temp1[1U] = (crc >> 0) & 0xFFU;

	uint8_t temp2[2U];
	temp2[0U] = 0x00U;
	temp2[1U] = 0x00U;
	unsigned int j = length;
	for (unsigned int i = 1U; i < 16U; i++, j++) {
		bool b = READ_BIT1(in, j);
		WRITE_BIT1(temp2, i, b);
	}

	return temp1[0U] == temp2[0U] && temp1[1U] == temp2[1U];
}

void CNXDNCRC::encodeCRC15(unsigned char* in, unsigned int length)
{
	assert(in != NULL);

	uint16_t crc = createCRC15(in, length);

	uint8_t temp[2U];
	temp[0U] = (crc >> 8) & 0xFFU;
	temp[1U] = (crc >> 0) & 0xFFU;

	unsigned int n = length;
	for (unsigned int i = 1U; i < 16U; i++, n++) {
		bool b = READ_BIT1(temp, i);
		WRITE_BIT1(in, n, b);
	}
}

uint8_t CNXDNCRC::createCRC6(const unsigned char* in, unsigned int length)
{
	uint8_t crc = 0x3FU;

	for (unsigned int i = 0U; i < length; i++) {
		bool bit1 = READ_BIT1(in, i) != 0x00U;
		bool bit2 = (crc & 0x20U) == 0x20U;

		crc <<= 1;

		if (bit1 ^ bit2)
			crc ^= 0x27U;
	}

	return crc & 0x3FU;
}

uint16_t CNXDNCRC::createCRC12(const unsigned char* in, unsigned int length)
{
	uint16_t crc = 0x0FFFU;

	for (unsigned int i = 0U; i < length; i++) {
		bool bit1 = READ_BIT1(in, i) != 0x00U;
		bool bit2 = (crc & 0x0800U) == 0x0800U;

		crc <<= 1;

		if (bit1 ^ bit2)
			crc ^= 0x080FU;
	}

	return crc & 0x0FFFU;
}

uint16_t CNXDNCRC::createCRC15(const unsigned char* in, unsigned int length)
{
	uint16_t crc = 0x7FFFU;

	for (unsigned int i = 0U; i < length; i++) {
		bool bit1 = READ_BIT1(in, i) != 0x00U;
		bool bit2 = (crc & 0x4000U) == 0x4000U;

		crc <<= 1;

		if (bit1 ^ bit2)
			crc ^= 0x4CC5U;
	}

	return crc & 0x7FFFU;
}
