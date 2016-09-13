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

#include "P25Data.h"
#include "P25Defines.h"
#include "P25Utils.h"
#include "Hamming.h"
#include "Utils.h"
#include "Log.h"

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CP25Data::CP25Data() :
m_source(0U),
m_group(true),
m_dest(0U)
{
}

CP25Data::~CP25Data()
{
}

void CP25Data::processHeader(unsigned char* data)
{
	unsigned char raw[81U];
	CP25Utils::decode(data, raw, 114U, 780U);

	CUtils::dump(1U, "P25, raw header", raw, 81U);

	// XXX Need to add FEC code
}

void CP25Data::processLDU1(unsigned char* data)
{
	unsigned char rs[18U];

	unsigned char raw[5U];
	CP25Utils::decode(data, raw, 410U, 452U);
	decodeLDUHamming(raw, rs + 0U);

	CP25Utils::decode(data, raw, 600U, 640U);
	decodeLDUHamming(raw, rs + 3U);

	CP25Utils::decode(data, raw, 788U, 830U);
	decodeLDUHamming(raw, rs + 6U);

	CP25Utils::decode(data, raw, 978U, 1020U);
	decodeLDUHamming(raw, rs + 9U);

	CP25Utils::decode(data, raw, 1168U, 1208U);
	decodeLDUHamming(raw, rs + 12U);

	CP25Utils::decode(data, raw, 1356U, 1398U);
	decodeLDUHamming(raw, rs + 15U);

	CUtils::dump(1U, "P25, LDU1 Data after Hamming", rs, 18U);

	switch (rs[0U]) {
	case P25_LCF_GROUP:
		m_dest   = (rs[4U] << 8) + rs[5U];
		m_source = (rs[6U] << 16) + (rs[7U] << 8) + rs[8U];
		m_group  = true;
		break;
	case P25_LCF_PRIVATE:
		m_dest   = (rs[3U] << 16) + (rs[4U] << 8) + rs[5U];
		m_source = (rs[6U] << 16) + (rs[7U] << 8) + rs[8U];
		m_group  = false;
		break;
	default:
		LogMessage("P25, unknown LCF value in LDU1 - $%02X", rs[0U]);
		break;
	}

	// XXX Need to add FEC code

	encodeLDUHamming(raw, rs + 0U);
	CP25Utils::encode(raw, data, 410U, 452U);

	encodeLDUHamming(raw, rs + 3U);
	CP25Utils::encode(raw, data, 600U, 640U);

	encodeLDUHamming(raw, rs + 6U);
	CP25Utils::encode(raw, data, 788U, 830U);

	encodeLDUHamming(raw, rs + 9U);
	CP25Utils::encode(raw, data, 978U, 1020U);

	encodeLDUHamming(raw, rs + 12U);
	CP25Utils::encode(raw, data, 1168U, 1208U);

	encodeLDUHamming(raw, rs + 15U);
	CP25Utils::encode(raw, data, 1356U, 1398U);
}

void CP25Data::processLDU2(unsigned char* data)
{
	unsigned char rs[18U];

	unsigned char raw[5U];
	CP25Utils::decode(data, raw, 410U, 452U);
	decodeLDUHamming(raw, rs + 0U);

	CP25Utils::decode(data, raw, 600U, 640U);
	decodeLDUHamming(raw, rs + 3U);

	CP25Utils::decode(data, raw, 788U, 830U);
	decodeLDUHamming(raw, rs + 6U);

	CP25Utils::decode(data, raw, 978U, 1020U);
	decodeLDUHamming(raw, rs + 9U);

	CP25Utils::decode(data, raw, 1168U, 1208U);
	decodeLDUHamming(raw, rs + 12U);

	CP25Utils::decode(data, raw, 1356U, 1398U);
	decodeLDUHamming(raw, rs + 15U);

	CUtils::dump(1U, "P25, LDU2 Data after Hamming", rs, 18U);

	// XXX Need to add FEC code

	encodeLDUHamming(raw, rs + 0U);
	CP25Utils::encode(raw, data, 410U, 452U);

	encodeLDUHamming(raw, rs + 3U);
	CP25Utils::encode(raw, data, 600U, 640U);

	encodeLDUHamming(raw, rs + 6U);
	CP25Utils::encode(raw, data, 788U, 830U);

	encodeLDUHamming(raw, rs + 9U);
	CP25Utils::encode(raw, data, 978U, 1020U);

	encodeLDUHamming(raw, rs + 12U);
	CP25Utils::encode(raw, data, 1168U, 1208U);

	encodeLDUHamming(raw, rs + 15U);
	CP25Utils::encode(raw, data, 1356U, 1398U);
}

void CP25Data::processTerminator(unsigned char* data)
{
	unsigned char raw[36U];
	CP25Utils::decode(data, raw, 114U, 210U);

	CUtils::dump(1U, "P25, raw terminator", raw, 36U);

	// XXX Need to add FEC code, or do we?
}

unsigned int CP25Data::getSource() const
{
	return m_source;
}

bool CP25Data::getGroup() const
{
	return m_group;
}

unsigned int CP25Data::getDest() const
{
	return m_dest;
}

void CP25Data::reset()
{
	m_source = 0U;
	m_dest = 0U;
}

void CP25Data::decodeLDUHamming(const unsigned char* data, unsigned char* raw)
{
	unsigned int n = 0U;
	unsigned int m = 0U;
	for (unsigned int i = 0U; i < 4U; i++) {
		bool hamming[10U];

		for (unsigned int j = 0U; j < 10U; j++) {
			hamming[j] = READ_BIT(data, n);
			n++;
		}

		CHamming::decode1063(hamming);

		for (unsigned int j = 0U; j < 6U; j++) {
			WRITE_BIT(raw, m, hamming[j]);
			m++;
		}
	}
}

void CP25Data::encodeLDUHamming(unsigned char* data, const unsigned char* raw)
{
	unsigned int n = 0U;
	unsigned int m = 0U;
	for (unsigned int i = 0U; i < 4U; i++) {
		bool hamming[10U];

		for (unsigned int j = 0U; j < 6U; j++) {
			hamming[j] = READ_BIT(raw, m);
			m++;
		}

		CHamming::encode1063(hamming);

		for (unsigned int j = 0U; j < 10U; j++) {
			WRITE_BIT(data, n, hamming[j]);
			n++;
		}
	}
}
