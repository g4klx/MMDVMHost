/*
 *   Copyright (C) 2018,2020 by Jonathan Naylor G4KLX
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

#include "M17CRC.h"

#include <cstdio>
#include <cassert>

const uint8_t  BIT_MASK_TABLE1[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE1[(i)&7])

bool CM17CRC::checkCRC(const unsigned char* in, unsigned int nBytes)
{
	assert(in != NULL);
	assert(nBytes > 2U);

	uint16_t crc = createCRC(in, nBytes);

	uint8_t temp[2U];
	temp[0U] = (crc >> 8) & 0xFFU;
	temp[1U] = (crc >> 0) & 0xFFU;

	return temp[0U] == in[nBytes - 2U] && temp[1U] == in[nBytes - 1U];
}

void CM17CRC::encodeCRC(unsigned char* in, unsigned int nBytes)
{
	assert(in != NULL);
	assert(nBytes > 2U);

	uint16_t crc = createCRC(in, nBytes);

	in[nBytes - 2U] = (crc >> 8) & 0xFFU;
	in[nBytes - 1U] = (crc >> 0) & 0xFFU;
}
uint16_t CM17CRC::createCRC(const unsigned char* in, unsigned int nBytes)
{
	assert(in != NULL);

	uint16_t crc = 0xFFFFU;

	for (unsigned int i = 0U; i < nBytes; i++) {
		bool bit1 = READ_BIT(in, i) != 0x00U;
		bool bit2 = (crc & 0x8000U) == 0x8000U;

		crc <<= 1;

		if (bit1 ^ bit2)
			crc ^= 0x5935U;
	}

	return crc;
}
