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
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned char BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CNXDNLICH::CNXDNLICH(const CNXDNLICH& lich) :
m_lich(lich.m_lich)
{
}

CNXDNLICH::CNXDNLICH() :
m_lich(0U)
{
}

CNXDNLICH::~CNXDNLICH()
{
}

bool CNXDNLICH::decode(const unsigned char* bytes)
{
	assert(bytes != NULL);

	unsigned char lich[1U];
	lich[0U] = 0x00U;

	unsigned int offset = NXDN_FSW_LENGTH_BITS;
	bool b7 = READ_BIT1(bytes, offset);
	WRITE_BIT1(lich, 7U, b7);

	offset++;
	bool b6 = READ_BIT1(bytes, offset);
	WRITE_BIT1(lich, 6U, b6);

	offset++;
	bool b5 = READ_BIT1(bytes, offset);
	WRITE_BIT1(lich, 5U, b5);

	offset++;
	bool b4 = READ_BIT1(bytes, offset);
	WRITE_BIT1(lich, 4U, b4);

	offset++;
	bool b3 = READ_BIT1(bytes, offset);
	WRITE_BIT1(lich, 3U, b3);

	offset++;
	bool b2 = READ_BIT1(bytes, offset);
	WRITE_BIT1(lich, 2U, b2);

	offset++;
	bool b1 = READ_BIT1(bytes, offset);
	WRITE_BIT1(lich, 1U, b1);

	offset++;
	bool b0 = READ_BIT1(bytes, offset);
	WRITE_BIT1(lich, 0U, b0);

	bool parity = b7 ^ b6 ^ b5 ^ b4;

	LogMessage("NXDN, LICH bits: %d%d %d%d %d%d %d - %d, parity: %d", b7 ? 1 : 0, b6 ? 1 : 0, b5 ? 1 : 0, b4 ? 1 : 0, b3 ? 1 : 0, b2 ? 1 : 0, b1 ? 1 : 0, b0 ? 1 : 0, parity ? 1 : 0);

	if (parity != b0)
		return false;

	m_lich = lich[0U] >> 1;

	return true;
}

void CNXDNLICH::encode(unsigned char* bytes)
{
	assert(bytes != NULL);

	unsigned char lich[1U];

	lich[0U] = m_lich << 1;

	bool b7 = READ_BIT1(lich, 7U);
	bool b6 = READ_BIT1(lich, 6U);
	bool b5 = READ_BIT1(lich, 5U);
	bool b4 = READ_BIT1(lich, 4U);
	bool b3 = READ_BIT1(lich, 3U);
	bool b2 = READ_BIT1(lich, 2U);
	bool b1 = READ_BIT1(lich, 1U);
	bool b0 = READ_BIT1(lich, 0U);

	bool parity = b7 ^ b6 ^ b5 ^ b4;

	WRITE_BIT1(lich, 0U, parity);

	unsigned int offset = NXDN_FSW_LENGTH_BITS;
	WRITE_BIT1(bytes, offset, b7);
	offset++;
	WRITE_BIT1(bytes, offset, true);
	offset++;

	WRITE_BIT1(bytes, offset, b6);
	offset++;
	WRITE_BIT1(bytes, offset, true);
	offset++;

	WRITE_BIT1(bytes, offset, b5);
	offset++;
	WRITE_BIT1(bytes, offset, true);
	offset++;

	WRITE_BIT1(bytes, offset, b4);
	offset++;
	WRITE_BIT1(bytes, offset, true);
	offset++;

	WRITE_BIT1(bytes, offset, b3);
	offset++;
	WRITE_BIT1(bytes, offset, true);
	offset++;

	WRITE_BIT1(bytes, offset, b2);
	offset++;
	WRITE_BIT1(bytes, offset, true);
	offset++;

	WRITE_BIT1(bytes, offset, b1);
	offset++;
	WRITE_BIT1(bytes, offset, true);
	offset++;

	WRITE_BIT1(bytes, offset, b0);
	offset++;
	WRITE_BIT1(bytes, offset, true);
}

unsigned char CNXDNLICH::getRFCT() const
{
	return (m_lich >> 5) & 0x03U;
}

unsigned char CNXDNLICH::getFCT() const
{
	return (m_lich >> 3) & 0x03U;
}

unsigned char CNXDNLICH::getOption() const
{
	return (m_lich >> 1) & 0x03U;
}

unsigned char CNXDNLICH::getDirection() const
{
	return m_lich & 0x01U;
}

void CNXDNLICH::setRFCT(unsigned char rfct)
{
	m_lich &= 0x1FU;
	m_lich |= (rfct << 5) & 0x60U;
}

void CNXDNLICH::setFCT(unsigned char usc)
{
	m_lich &= 0x67U;
	m_lich |= (usc << 3) & 0x18U;
}

void CNXDNLICH::setOption(unsigned char option)
{
	m_lich &= 0x79U;
	m_lich |= (option << 1) & 0x06U;
}

void CNXDNLICH::setDirection(unsigned char direction)
{
	m_lich &= 0x7EU;
	m_lich |= direction & 0x01U;
}

CNXDNLICH& CNXDNLICH::operator=(const CNXDNLICH& lich)
{
	if (&lich != this)
		m_lich = lich.m_lich;

	return *this;
}
