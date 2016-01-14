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

#include "EMB.h"

#include "QR1676.h"

#include <cstdio>
#include <cassert>

CEMB::CEMB() :
m_colorCode(0U),
m_PI(false),
m_LCSS(0U)
{
}

CEMB::~CEMB()
{
}

void CEMB::putData(const unsigned char* data)
{
	assert(data != NULL);

	unsigned char emb[2U];
	emb[0U]  = (data[13U] << 4) & 0xF0U;
	emb[0U] |= (data[14U] >> 4) & 0x0FU;
	emb[1U]  = (data[18U] << 4) & 0xF0U;
	emb[1U] |= (data[19U] >> 4) & 0x0FU;

	CQR1676::decode(emb);

	m_colorCode = (emb[0U] >> 4) & 0x0FU;
	m_PI        = (emb[0U] & 0x08U) == 0x08U;
	m_LCSS      = (emb[0U] >> 1) & 0x03U;
}

void CEMB::getData(unsigned char* data) const
{
	assert(data != NULL);

	unsigned char emb[2U];
	emb[0U]  = (m_colorCode << 4) & 0xF0U;
	emb[0U] |= m_PI ? 0x08U : 0x00U;
	emb[0U] |= (m_LCSS << 1) & 0x06U;
	emb[1U]  = 0x00U;

	CQR1676::encode(emb);

	data[13U] = (data[13U] & 0xF0U) | ((emb[0U] >> 4U) & 0x0FU);
	data[14U] = (data[14U] & 0x0FU) | ((emb[0U] << 4U) & 0xF0U);
	data[18U] = (data[18U] & 0xF0U) | ((emb[1U] >> 4U) & 0x0FU);
	data[19U] = (data[19U] & 0x0FU) | ((emb[1U] << 4U) & 0xF0U);
}

unsigned char CEMB::getColorCode() const
{
	return m_colorCode;
}

void CEMB::setColorCode(unsigned char code)
{
	m_colorCode = code;
}

bool CEMB::getPI() const
{
	return m_PI;
}

void CEMB::setPI(bool pi)
{
	m_PI = pi;
}

unsigned char CEMB::getLCSS() const
{
	return m_LCSS;
}

void CEMB::setLCSS(unsigned char lcss)
{
	m_LCSS = lcss;
}
