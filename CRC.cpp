/*
 *   Copyright (C) 2015 by Jonathan Naylor G4KLX
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

#include "CRC.h"

#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cmath>

bool CCRC::checkFiveBit(bool* in, unsigned int tcrc)
{
	assert(in != NULL);

	unsigned int crc;
	encodeFiveBit(in, crc);

	return crc == tcrc;
}

void CCRC::encodeFiveBit(const bool* in, unsigned int& tcrc)
{
	assert(in != NULL);

	unsigned short total = 0U;
	for (unsigned int i = 0U; i < 72U; i += 8U) {
		unsigned char c;
		CUtils::bitsToByteBE(in + i, c);
		total += c;
	}

	total %= 31U;

	tcrc = total;
}

unsigned char CCRC::encodeEightBit(const unsigned char *in, unsigned int length)
{
	assert(in != NULL);

	unsigned char crc = 0x00U;

	for (unsigned int i = 0U; i < length; i++) {
		crc ^= in[i];
	
		for (unsigned int j = 0U; j < 8U; j++) {
			if ((crc & 0x80U) == 0x80U) {
				crc <<= 1;
				crc ^= 0x07U;
			} else {
				crc <<= 1;
			}
		}
	}

	return crc;
}

bool CCRC::checkCCITT16(const unsigned char *in, unsigned int length)
{
	assert(in != NULL);
	assert(length > 2U);

	unsigned short crc16 = 0U;

	for (unsigned int a = 0; a < (length - 2U); a++)	{
		unsigned char val = in[a];

		for (unsigned int i = 0U; i < 8U; i++) {
			bool c15 = (crc16 >> 15 & 0x01U) == 0x01U;
			bool bit = (val >> (7 - i) & 0x01U) == 0x01U;
			crc16 <<= 1;
			if (c15 ^ bit)
				crc16 ^= 0x1021U;
		}
	}

	LogMessage("CCITT16, %04X == %02X %02X", crc16, in[length - 2U], in[length - 1U]);

	return crc16 == (in[length - 2U] << 8 | in[length - 1U]);
}

unsigned char CCRC::crc8(const unsigned char *in, unsigned int length)
{
	unsigned int crc = 0U;

	for (unsigned int j = 0U; j < length; j++, in++) {
		crc ^= (*in << 8);

		for (unsigned int i = 0U; i < 8U; i++) {
			if (crc & 0x8000U)
				crc ^= (0x1070U << 3);
			crc <<= 1;
		}
	}

	return crc >> 8;
}
