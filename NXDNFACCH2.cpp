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

#include "NXDNFACCH2.h"

#include "NXDNDefines.h"
#include "NXDNUDCH.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CNXDNFACCH2::CNXDNFACCH2(const CNXDNFACCH2& facch2) :
m_data(NULL)
{
	m_data = new unsigned char[23U];
	::memcpy(m_data, facch2.m_data, 23U);
}

CNXDNFACCH2::CNXDNFACCH2() :
m_data(NULL)
{
	m_data = new unsigned char[23U];
}

CNXDNFACCH2::~CNXDNFACCH2()
{
	delete[] m_data;
}

bool CNXDNFACCH2::decode(const unsigned char* data)
{
	assert(data != NULL);

	CNXDNUDCH udch;
	bool valid = udch.decode(data);
	if (!valid)
		return false;

	udch.getData(m_data);

	return true;
}

void CNXDNFACCH2::encode(unsigned char* data) const
{
	assert(data != NULL);

	CNXDNUDCH udch;

	udch.setData(m_data);

	udch.encode(data);
}

unsigned char CNXDNFACCH2::getRAN() const
{
	return m_data[0U] & 0x3FU;
}

void CNXDNFACCH2::getData(unsigned char* data) const
{
	assert(data != NULL);

	::memcpy(data, m_data + 1U, 22U);
}

void CNXDNFACCH2::setRAN(unsigned char ran)
{
	m_data[0U] &= 0xC0U;
	m_data[0U] |= ran;
}

void CNXDNFACCH2::setData(const unsigned char* data)
{
	assert(data != NULL);

	::memcpy(m_data + 1U, data, 22U);
}

CNXDNFACCH2& CNXDNFACCH2::operator=(const CNXDNFACCH2& facch2)
{
	if (&facch2 != this)
		::memcpy(m_data, facch2.m_data, 23U);

	return *this;
}
