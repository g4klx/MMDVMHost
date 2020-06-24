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
#include "NXDNLayer3.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CNXDNLayer3::CNXDNLayer3(const CNXDNLayer3& layer3) :
m_data(NULL)
{
	m_data = new unsigned char[22U];
	::memcpy(m_data, layer3.m_data, 22U);
}

CNXDNLayer3::CNXDNLayer3() :
m_data(NULL)
{
	m_data = new unsigned char[22U];
	::memset(m_data, 0x00U, 22U);
}

CNXDNLayer3::~CNXDNLayer3()
{
	delete[] m_data;
}

void CNXDNLayer3::decode(const unsigned char* bytes, unsigned int length, unsigned int offset)
{
	assert(bytes != NULL);

	for (unsigned int i = 0U; i < length; i++, offset++) {
		bool b = READ_BIT1(bytes, i);
		WRITE_BIT1(m_data, offset, b);
	}
}

void CNXDNLayer3::encode(unsigned char* bytes, unsigned int length, unsigned int offset)
{
	assert(bytes != NULL);

	for (unsigned int i = 0U; i < length; i++, offset++) {
		bool b = READ_BIT1(m_data, offset);
		WRITE_BIT1(bytes, i, b);
	}
}

unsigned char CNXDNLayer3::getMessageType() const
{
	return m_data[0U] & 0x3FU;
}

unsigned short CNXDNLayer3::getSourceUnitId() const
{
	return (m_data[3U] << 8) | m_data[4U];
}

unsigned short CNXDNLayer3::getDestinationGroupId() const
{
	return (m_data[5U] << 8) | m_data[6U];
}

bool CNXDNLayer3::getIsGroup() const
{
	return (m_data[2U] & 0x80U) != 0x80U;
}

unsigned char CNXDNLayer3::getDataBlocks() const
{
	return (m_data[8U] & 0x0FU) + 1U;
}

void CNXDNLayer3::getData(unsigned char* data) const
{
	::memcpy(data, m_data, 22U);
}

void CNXDNLayer3::setData(const unsigned char* data, unsigned int length)
{
	::memset(m_data, 0x00U, 22U);
	::memcpy(m_data, data, length);
}

void CNXDNLayer3::reset()
{
	::memset(m_data, 0x00U, 22U);
}

CNXDNLayer3& CNXDNLayer3::operator=(const CNXDNLayer3& layer3)
{
	if (&layer3 != this)
		::memcpy(m_data, layer3.m_data, 22U);

	return *this;
}
