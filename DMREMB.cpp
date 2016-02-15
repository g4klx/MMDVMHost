/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
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

#include "DMREMB.h"

#include "QR1676.h"

#include <cstdio>
#include <cassert>

CDMREMB::CDMREMB() :
m_colorCode(0U),
m_PI(false),
m_LCSS(0U)
{
}

CDMREMB::~CDMREMB()
{
}

void CDMREMB::putData(const unsigned char* data)
{
	assert(data != NULL);

	unsigned char DMREMB[2U];
	DMREMB[0U]  = (data[13U] << 4) & 0xF0U;
	DMREMB[0U] |= (data[14U] >> 4) & 0x0FU;
	DMREMB[1U]  = (data[18U] << 4) & 0xF0U;
	DMREMB[1U] |= (data[19U] >> 4) & 0x0FU;

	CQR1676::decode(DMREMB);

	m_colorCode = (DMREMB[0U] >> 4) & 0x0FU;
	m_PI        = (DMREMB[0U] & 0x08U) == 0x08U;
	m_LCSS      = (DMREMB[0U] >> 1) & 0x03U;
}

void CDMREMB::getData(unsigned char* data) const
{
	assert(data != NULL);

	unsigned char DMREMB[2U];
	DMREMB[0U]  = (m_colorCode << 4) & 0xF0U;
	DMREMB[0U] |= m_PI ? 0x08U : 0x00U;
	DMREMB[0U] |= (m_LCSS << 1) & 0x06U;
	DMREMB[1U]  = 0x00U;

	CQR1676::encode(DMREMB);

	data[13U] = (data[13U] & 0xF0U) | ((DMREMB[0U] >> 4U) & 0x0FU);
	data[14U] = (data[14U] & 0x0FU) | ((DMREMB[0U] << 4U) & 0xF0U);
	data[18U] = (data[18U] & 0xF0U) | ((DMREMB[1U] >> 4U) & 0x0FU);
	data[19U] = (data[19U] & 0x0FU) | ((DMREMB[1U] << 4U) & 0xF0U);
}

unsigned char CDMREMB::getColorCode() const
{
	return m_colorCode;
}

void CDMREMB::setColorCode(unsigned char code)
{
	m_colorCode = code;
}

bool CDMREMB::getPI() const
{
	return m_PI;
}

void CDMREMB::setPI(bool pi)
{
	m_PI = pi;
}

unsigned char CDMREMB::getLCSS() const
{
	return m_LCSS;
}

void CDMREMB::setLCSS(unsigned char lcss)
{
	m_LCSS = lcss;
}
