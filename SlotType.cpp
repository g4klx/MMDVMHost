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

#include "SlotType.h"

#include "Golay2087.h"

#include <cstdio>
#include <cassert>

CSlotType::CSlotType() :
m_colorCode(0U),
m_dataType(0U)
{
}

CSlotType::~CSlotType()
{
}

void CSlotType::putData(const unsigned char* data)
{
	assert(data != NULL);

	unsigned char slotType[3U];
	slotType[0U]  = (data[12U] << 2) & 0xFCU;
	slotType[0U] |= (data[13U] >> 6) & 0x03U;

	slotType[1U]  = (data[13U] << 2) & 0xC0U;
	slotType[1U] |= (data[19U] << 2) & 0x3CU;
	slotType[1U] |= (data[20U] >> 6) & 0x03U;

	slotType[2U]  = (data[20U] << 2) & 0xF0U;

	unsigned char code = CGolay2087::decode(slotType);

	m_colorCode = (code >> 4) & 0x0FU;
	m_dataType  = (code >> 0) & 0x0FU;
}

void CSlotType::getData(unsigned char* data) const
{
	assert(data != NULL);

	unsigned char slotType[3U];
	slotType[0U]  = (m_colorCode << 4) & 0xF0U;
	slotType[0U] |= (m_dataType  << 0) & 0x0FU;
	slotType[1U]  = 0x00U;
	slotType[2U]  = 0x00U;

	CGolay2087::encode(slotType);

	data[12U] = (data[12U] & 0xC0U) | ((slotType[0U] >> 2) & 0x3FU);
	data[13U] = (data[13U] & 0x0FU) | ((slotType[0U] << 6) & 0xC0U) | ((slotType[1U] >> 2) & 0x30U);
	data[19U] = (data[19U] & 0xF0U) | ((slotType[1U] >> 2) & 0x0FU);
	data[20U] = (data[20U] & 0x03U) | ((slotType[1U] << 6) & 0xC0U) | ((slotType[2U] >> 2) & 0x3CU);
}

unsigned char CSlotType::getColorCode() const
{
	return m_colorCode;
}

void CSlotType::setColorCode(unsigned char code)
{
	m_colorCode = code;
}

unsigned char CSlotType::getDataType() const
{
	return m_dataType;
}

void CSlotType::setDataType(unsigned char type)
{
	m_dataType = type;
}
