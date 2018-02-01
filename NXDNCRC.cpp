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
#include "Log.h"

#include <cstdio>
#include <cassert>

const uint8_t  BIT_MASK_TABLE1[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };
const uint16_t BIT_MASK_TABLE2[] = { 0x8000U, 0x4000U, 0x2000U, 0x1000U, 0x0800U, 0x0400U, 0x0200U, 0x0100U,
									 0x0080U, 0x0040U, 0x0020U, 0x0010U, 0x0008U, 0x0004U, 0x0002U, 0x0001U };

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE1[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE1[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE1[(i)&7])

#define WRITE_BIT2(p,i,b) p[(i)>>4] = (b) ? (p[(i)>>4] | BIT_MASK_TABLE2[(i)&15]) : (p[(i)>>3] & ~BIT_MASK_TABLE2[(i)&15])
#define READ_BIT2(p,i)    (p[(i)>>4] & BIT_MASK_TABLE2[(i)&15])

bool CNXDNCRC::checkCRC6(const unsigned char* in, unsigned int length)
{
	assert(in != NULL);

	uint8_t crc[1U];
	crc[0U] = createCRC6(in, length);

	uint8_t temp[1U];
	temp[0U] = 0x00U;
	unsigned int j = length;
	for (unsigned int i = 0U; i < 6U; i++, j++) {
		bool b = READ_BIT1(in, j);
		WRITE_BIT1(temp, i, b);
	}

	LogMessage("NXDN, CRC6: new:%02X old:%02X", crc[0U], temp[0U]);

	unsigned int n = length;
	for (unsigned int i = 0U; i < 6U; i++, n++) {
		bool b1 = READ_BIT1(crc, i);
		bool b2 = READ_BIT1(in, n);
		if (b1 != b2)
			return false;
	}

	return true;
}

void CNXDNCRC::encodeCRC6(unsigned char* in, unsigned int length)
{
	assert(in != NULL);

	uint8_t crc[1U];
	crc[0U] = createCRC6(in, length);

	unsigned int n = length;
	for (unsigned int i = 0U; i < 6U; i++, n++) {
		bool b = READ_BIT1(crc, i);
		WRITE_BIT1(in, n, b);
	}
}

bool CNXDNCRC::checkCRC12(const unsigned char* in, unsigned int length)
{
	assert(in != NULL);

	uint16_t crc[1U];
	crc[0U] = createCRC12(in, length);

	uint16_t temp[1U];
	temp[0U] = 0x00U;
	unsigned int j = length;
	for (unsigned int i = 0U; i < 12U; i++, j++) {
		bool b = READ_BIT1(in, j);
		WRITE_BIT2(temp, i, b);
	}

	LogMessage("NXDN, CRC12: new:%04X old:%04X", crc[0U], temp[0U]);

	unsigned int n = length;
	for (unsigned int i = 0U; i < 12U; i++, n++) {
		bool b1 = READ_BIT2(crc, i);
		bool b2 = READ_BIT1(in, n);
		if (b1 != b2)
			return false;
	}

	return true;
}

void CNXDNCRC::encodeCRC12(unsigned char* in, unsigned int length)
{
	assert(in != NULL);

	uint16_t crc[1U];
	crc[0U] = createCRC12(in, length);

	unsigned int n = length;
	for (unsigned int i = 0U; i < 12U; i++, n++) {
		bool b = READ_BIT2(crc, i);
		WRITE_BIT1(in, n, b);
	}
}

bool CNXDNCRC::checkCRC15(const unsigned char* in, unsigned int length)
{
	assert(in != NULL);

	uint16_t crc[1U];
	crc[0U] = createCRC15(in, length);

	uint16_t temp[1U];
	temp[0U] = 0x00U;
	unsigned int j = length;
	for (unsigned int i = 0U; i < 15U; i++, j++) {
		bool b = READ_BIT1(in, j);
		WRITE_BIT2(temp, i, b);
	}

	LogMessage("NXDN, CRC15: new:%04X old:%04X", crc[0U], temp[0U]);

	unsigned int n = length;
	for (unsigned int i = 0U; i < 15U; i++, n++) {
		bool b1 = READ_BIT2(crc, i);
		bool b2 = READ_BIT1(in, n);
		if (b1 != b2)
			return false;
	}

	return true;
}

void CNXDNCRC::encodeCRC15(unsigned char* in, unsigned int length)
{
	assert(in != NULL);

	uint16_t crc[1U];
	crc[0U] = createCRC15(in, length);

	unsigned int n = length;
	for (unsigned int i = 0U; i < 15U; i++, n++) {
		bool b = READ_BIT2(crc, i);
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
		crc &= 0x3FU;

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
		crc &= 0x0FFFU;

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
		crc &= 0x7FFFU;

		if (bit1 ^ bit2)
			crc ^= 0x4CC5U;
	}

	return crc & 0x7FFFU;
}
