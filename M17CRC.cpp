/*
 *   Copyright (C) 2020,2021,2025 by Jonathan Naylor G4KLX
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

const uint16_t CRC16_TABLE[] = {0x0000U, 0x5935U, 0xB26AU, 0xEB5FU, 0x3DE1U, 0x64D4U, 0x8F8BU, 0xD6BEU, 0x7BC2U, 0x22F7U, 0xC9A8U,
			       0x909DU, 0x4623U, 0x1F16U, 0xF449U, 0xAD7CU, 0xF784U, 0xAEB1U, 0x45EEU, 0x1CDBU, 0xCA65U, 0x9350U,
			       0x780FU, 0x213AU, 0x8C46U, 0xD573U, 0x3E2CU, 0x6719U, 0xB1A7U, 0xE892U, 0x03CDU, 0x5AF8U, 0xB63DU,
			       0xEF08U, 0x0457U, 0x5D62U, 0x8BDCU, 0xD2E9U, 0x39B6U, 0x6083U, 0xCDFFU, 0x94CAU, 0x7F95U, 0x26A0U,
			       0xF01EU, 0xA92BU, 0x4274U, 0x1B41U, 0x41B9U, 0x188CU, 0xF3D3U, 0xAAE6U, 0x7C58U, 0x256DU, 0xCE32U,
			       0x9707U, 0x3A7BU, 0x634EU, 0x8811U, 0xD124U, 0x079AU, 0x5EAFU, 0xB5F0U, 0xECC5U, 0x354FU, 0x6C7AU,
			       0x8725U, 0xDE10U, 0x08AEU, 0x519BU, 0xBAC4U, 0xE3F1U, 0x4E8DU, 0x17B8U, 0xFCE7U, 0xA5D2U, 0x736CU,
			       0x2A59U, 0xC106U, 0x9833U, 0xC2CBU, 0x9BFEU, 0x70A1U, 0x2994U, 0xFF2AU, 0xA61FU, 0x4D40U, 0x1475U,
			       0xB909U, 0xE03CU, 0x0B63U, 0x5256U, 0x84E8U, 0xDDDDU, 0x3682U, 0x6FB7U, 0x8372U, 0xDA47U, 0x3118U,
			       0x682DU, 0xBE93U, 0xE7A6U, 0x0CF9U, 0x55CCU, 0xF8B0U, 0xA185U, 0x4ADAU, 0x13EFU, 0xC551U, 0x9C64U,
			       0x773BU, 0x2E0EU, 0x74F6U, 0x2DC3U, 0xC69CU, 0x9FA9U, 0x4917U, 0x1022U, 0xFB7DU, 0xA248U, 0x0F34U,
			       0x5601U, 0xBD5EU, 0xE46BU, 0x32D5U, 0x6BE0U, 0x80BFU, 0xD98AU, 0x6A9EU, 0x33ABU, 0xD8F4U, 0x81C1U,
			       0x577FU, 0x0E4AU, 0xE515U, 0xBC20U, 0x115CU, 0x4869U, 0xA336U, 0xFA03U, 0x2CBDU, 0x7588U, 0x9ED7U,
			       0xC7E2U, 0x9D1AU, 0xC42FU, 0x2F70U, 0x7645U, 0xA0FBU, 0xF9CEU, 0x1291U, 0x4BA4U, 0xE6D8U, 0xBFEDU,
			       0x54B2U, 0x0D87U, 0xDB39U, 0x820CU, 0x6953U, 0x3066U, 0xDCA3U, 0x8596U, 0x6EC9U, 0x37FCU, 0xE142U,
			       0xB877U, 0x5328U, 0x0A1DU, 0xA761U, 0xFE54U, 0x150BU, 0x4C3EU, 0x9A80U, 0xC3B5U, 0x28EAU, 0x71DFU,
			       0x2B27U, 0x7212U, 0x994DU, 0xC078U, 0x16C6U, 0x4FF3U, 0xA4ACU, 0xFD99U, 0x50E5U, 0x09D0U, 0xE28FU,
			       0xBBBAU, 0x6D04U, 0x3431U, 0xDF6EU, 0x865BU, 0x5FD1U, 0x06E4U, 0xEDBBU, 0xB48EU, 0x6230U, 0x3B05U,
			       0xD05AU, 0x896FU, 0x2413U, 0x7D26U, 0x9679U, 0xCF4CU, 0x19F2U, 0x40C7U, 0xAB98U, 0xF2ADU, 0xA855U,
			       0xF160U, 0x1A3FU, 0x430AU, 0x95B4U, 0xCC81U, 0x27DEU, 0x7EEBU, 0xD397U, 0x8AA2U, 0x61FDU, 0x38C8U,
			       0xEE76U, 0xB743U, 0x5C1CU, 0x0529U, 0xE9ECU, 0xB0D9U, 0x5B86U, 0x02B3U, 0xD40DU, 0x8D38U, 0x6667U,
			       0x3F52U, 0x922EU, 0xCB1BU, 0x2044U, 0x7971U, 0xAFCFU, 0xF6FAU, 0x1DA5U, 0x4490U, 0x1E68U, 0x475DU,
			       0xAC02U, 0xF537U, 0x2389U, 0x7ABCU, 0x91E3U, 0xC8D6U, 0x65AAU, 0x3C9FU, 0xD7C0U, 0x8EF5U, 0x584BU,
			       0x017EU, 0xEA21U, 0xB314U};

bool CM17CRC::checkCRC16(const unsigned char* in, unsigned int nBytes)
{
	assert(in != nullptr);
	assert(nBytes > 2U);

	uint16_t crc = createCRC16(in, nBytes - 2U);

	uint8_t temp[2U];
	temp[0U] = (crc >> 8) & 0xFFU;
	temp[1U] = (crc >> 0) & 0xFFU;

	return temp[0U] == in[nBytes - 2U] && temp[1U] == in[nBytes - 1U];
}

void CM17CRC::encodeCRC16(unsigned char* in, unsigned int nBytes)
{
	assert(in != nullptr);
	assert(nBytes > 2U);

	uint16_t crc = createCRC16(in, nBytes - 2U);

	in[nBytes - 2U] = (crc >> 8) & 0xFFU;
	in[nBytes - 1U] = (crc >> 0) & 0xFFU;
}

uint16_t CM17CRC::createCRC16(const unsigned char* in, unsigned int nBytes)
{
	assert(in != nullptr);

	uint16_t crc = 0xFFFFU;

	for (unsigned int i = 0U; i < nBytes; i++)
		crc = (crc << 8) ^ CRC16_TABLE[((crc >> 8) ^ uint16_t(in[i])) & 0x00FFU];

	return crc;
}
