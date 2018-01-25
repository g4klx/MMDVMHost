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

#include "NXDNFACCH1.h"

#include "NXDNConvolution.h"
#include "NXDNDefines.h"
#include "NXDNCRC.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CNXDNFACCH1::CNXDNFACCH1(const CNXDNFACCH1& facch1) :
m_data(NULL)
{
	m_data = new unsigned char[10U + 2U];
	::memcpy(m_data, facch1.m_data, 10U + 2U);
}

CNXDNFACCH1::CNXDNFACCH1() :
m_data(NULL)
{
	m_data = new unsigned char[10U + 2U];
}

CNXDNFACCH1::~CNXDNFACCH1()
{
	delete[] m_data;
}

bool CNXDNFACCH1::decode(const unsigned char* data)
{
	assert(data != NULL);

	return true;
}

void CNXDNFACCH1::encode(unsigned char* data) const
{
	assert(data != NULL);
}

void CNXDNFACCH1::getData(unsigned char* data) const
{
	assert(data != NULL);

	::memcpy(data, m_data, 10U);
}

void CNXDNFACCH1::setData(const unsigned char* data)
{
	assert(data != NULL);

	::memcpy(m_data, data, 10U);
}

CNXDNFACCH1& CNXDNFACCH1::operator=(const CNXDNFACCH1& facch1)
{
	if (&facch1 != this)
		::memcpy(m_data, facch1.m_data, 10U + 2U);

	return *this;
}
