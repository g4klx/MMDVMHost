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

#include "NXDNDefines.h"
#include "NXDNLICH.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned char BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CNXDNLICH::CNXDNLICH(const CNXDNLICH& lich) :
m_lich(NULL)
{
	m_lich = new unsigned char[1U];
	m_lich[0U] = lich.m_lich[0U];
}

CNXDNLICH::CNXDNLICH() :
m_lich(NULL)
{
	m_lich = new unsigned char[1U];
}

CNXDNLICH::~CNXDNLICH()
{
	delete[] m_lich;
}

bool CNXDNLICH::decode(const unsigned char* bytes)
{
	assert(bytes != NULL);

	bool b[8U];
	unsigned int offset1 = NXDN_FSW_LENGTH_BITS;
	unsigned int offset2 = 7U;
	for (unsigned int i = 0U; i < (NXDN_LICH_LENGTH_BITS / 2U); i++, offset1 += 2U, offset2--) {
		b[offset2] = READ_BIT1(bytes, offset1);
		WRITE_BIT1(m_lich, i, b[offset2]);
	}

	bool parity = b[7U] ^ b[6U] ^ b[5U] ^ b[4U];

	if (parity != b[0U])
		return false;

	return true;
}

void CNXDNLICH::encode(unsigned char* bytes)
{
	assert(bytes != NULL);

	bool b[8U];
	unsigned int offset = 7U;
	for (unsigned int i = 0U; i < (NXDN_LICH_LENGTH_BITS / 2U); i++, offset--)
		b[offset] = READ_BIT1(m_lich, i);

	b[0U] = b[7U] ^ b[6U] ^ b[5U] ^ b[4U];

	unsigned int offset1 = NXDN_FSW_LENGTH_BITS;
	unsigned int offset2 = 7U;
	for (unsigned int i = 0U; i < (NXDN_LICH_LENGTH_BITS / 2U); i++, offset2--) {
		WRITE_BIT1(bytes, offset1, b[offset2]);
		offset1++;
		WRITE_BIT1(bytes, offset1, true);
		offset1++;
	}
}

unsigned char CNXDNLICH::getRFCT() const
{
	return (m_lich[0U] >> 6) & 0x03U;
}

unsigned char CNXDNLICH::getFCT() const
{
	return (m_lich[0U] >> 4) & 0x03U;
}

unsigned char CNXDNLICH::getOption() const
{
	return (m_lich[0U] >> 2) & 0x03U;
}

unsigned char CNXDNLICH::getDirection() const
{
	return (m_lich[0U] >> 1) & 0x01U;
}

unsigned char CNXDNLICH::getRaw() const
{
	switch (m_lich[0U] & 0xF0U) {
	case 0x80U:
	case 0xB0U:
		m_lich[0U] |= 0x01U;
		break;
	default:
		m_lich[0U] &= 0xFEU;
		break;
	}

	return m_lich[0U];
}

void CNXDNLICH::setRFCT(unsigned char rfct)
{
	m_lich[0U] &= 0x3FU;
	m_lich[0U] |= (rfct << 6) & 0xC0U;
}

void CNXDNLICH::setFCT(unsigned char usc)
{
	m_lich[0U] &= 0xCFU;
	m_lich[0U] |= (usc << 4) & 0x30U;
}

void CNXDNLICH::setOption(unsigned char option)
{
	m_lich[0U] &= 0xF3U;
	m_lich[0U] |= (option << 2) & 0x0CU;
}

void CNXDNLICH::setDirection(unsigned char direction)
{
	m_lich[0U] &= 0xFDU;
	m_lich[0U] |= (direction << 1) & 0x02U;
}

void CNXDNLICH::setRaw(unsigned char lich)
{
	m_lich[0U] = lich;
}

CNXDNLICH& CNXDNLICH::operator=(const CNXDNLICH& lich)
{
	if (&lich != this)
		m_lich[0U] = lich.m_lich[0U];

	return *this;
}
