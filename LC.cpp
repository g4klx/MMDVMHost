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

#include "LC.h"

#include "Utils.h"

#include <cstdio>
#include <cassert>

CLC::CLC(FLCO flco, unsigned int srcId, unsigned int dstId) :
m_PF(false),
m_FLCO(flco),
m_FID(0U),
m_srcId(srcId),
m_dstId(dstId)
{
}

CLC::CLC(const unsigned char* bytes) :
m_PF(false),
m_FLCO(FLCO_GROUP),
m_FID(0U),
m_srcId(0U),
m_dstId(0U)
{
	assert(bytes != NULL);

	m_PF = (bytes[0U] & 0x80U) == 0x80U;

	m_FLCO = FLCO(bytes[0U] & 0x3FU);

	m_FID = bytes[1U];

	m_dstId = bytes[3U] << 16 | bytes[4U] << 8 | bytes[5U];
	m_srcId = bytes[6U] << 16 | bytes[7U] << 8 | bytes[8U];
}

CLC::CLC(const bool* bits) :
m_PF(false),
m_FLCO(FLCO_GROUP),
m_FID(0U),
m_srcId(0U),
m_dstId(0U)
{
	assert(bits != NULL);

	m_PF = bits[0U];

	unsigned char temp1, temp2;
	CUtils::bitsToByteBE(bits + 0U, temp1);
	m_FLCO = FLCO(temp1 & 0x3FU);

	CUtils::bitsToByteBE(bits + 8U, temp2);
	m_FID = temp2;

	unsigned char d1, d2, d3;
	CUtils::bitsToByteBE(bits + 24U, d1);
	CUtils::bitsToByteBE(bits + 32U, d2);
	CUtils::bitsToByteBE(bits + 40U, d3);

	unsigned char s1, s2, s3;
	CUtils::bitsToByteBE(bits + 48U, s1);
	CUtils::bitsToByteBE(bits + 56U, s2);
	CUtils::bitsToByteBE(bits + 64U, s3);

	m_srcId = s1 << 16 | s2 << 8 | s3;
	m_dstId = d1 << 16 | d2 << 8 | d3;
}

CLC::CLC() :
m_PF(false),
m_FLCO(FLCO_GROUP),
m_FID(0U),
m_srcId(0U),
m_dstId(0U)
{
}

CLC::~CLC()
{
}

void CLC::getData(unsigned char* bytes) const
{
	assert(bytes != NULL);

	bytes[0U] = (unsigned char)m_FLCO;

	if (m_PF)
		bytes[0U] |= 0x80U;

	bytes[1U] = m_FID;

	bytes[3U] = m_dstId >> 16;
	bytes[4U] = m_dstId >> 8;
	bytes[5U] = m_dstId >> 0;

	bytes[6U] = m_srcId >> 16;
	bytes[7U] = m_srcId >> 8;
	bytes[8U] = m_srcId >> 0;
}

void CLC::getData(bool* bits) const
{
	unsigned char bytes[9U];
	getData(bytes);

	CUtils::byteToBitsBE(bytes[0U], bits + 0U);
	CUtils::byteToBitsBE(bytes[1U], bits + 8U);
	CUtils::byteToBitsBE(bytes[2U], bits + 16U);
	CUtils::byteToBitsBE(bytes[3U], bits + 24U);
	CUtils::byteToBitsBE(bytes[4U], bits + 32U);
	CUtils::byteToBitsBE(bytes[5U], bits + 40U);
	CUtils::byteToBitsBE(bytes[6U], bits + 48U);
	CUtils::byteToBitsBE(bytes[7U], bits + 56U);
	CUtils::byteToBitsBE(bytes[8U], bits + 64U);
}

bool CLC::getPF() const
{
	return m_PF;
}

void CLC::setPF(bool pf)
{
	m_PF = pf;
}

FLCO CLC::getFLCO() const
{
	return m_FLCO;
}

void CLC::setFLCO(FLCO flco)
{
	m_FLCO = flco;
}

unsigned char CLC::getFID() const
{
	return m_FID;
}

void CLC::setFID(unsigned char fid)
{
	m_FID = fid;
}

unsigned int CLC::getSrcId() const
{
	return m_srcId;
}

void CLC::setSrcId(unsigned int id)
{
	m_srcId = id;
}

unsigned int CLC::getDstId() const
{
	return m_dstId;
}

void CLC::setDstId(unsigned int id)
{
	m_dstId = id;
}
