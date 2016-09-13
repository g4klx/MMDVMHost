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

#include "P25NID.h"
#include "P25Defines.h"

#include <cstdio>
#include <cassert>

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CP25NID::CP25NID() :
m_duid(0U),
m_nac(0U)
{
}

CP25NID::~CP25NID()
{
}

void CP25NID::process(unsigned char* data)
{
	assert(data != NULL);

	unsigned char nid[P25_NID_LENGTH_BYTES];

	unsigned int n = 0U;
	for (unsigned int offset = 48U; offset < 114U; offset++) {
		if (offset != P25_SS0_START && offset != P25_SS1_START) {
			bool b = READ_BIT(data, offset);
			WRITE_BIT(nid, n, b);
			n++;
		}
	}

	// XXX Process FEC here

	m_duid = nid[1U] & 0x0FU;

	m_nac  = (nid[0U] << 4) & 0xFF0U;
	m_nac |= (nid[1U] >> 4) & 0x00FU;

	n = 0U;
	for (unsigned int offset = 48U; offset < 114U; offset++) {
		if (offset != P25_SS0_START && offset != P25_SS1_START) {
			bool b = READ_BIT(nid, n);
			WRITE_BIT(data, offset, b);
			n++;
		}
	}
}

unsigned char CP25NID::getDUID() const
{
	return m_duid;
}

unsigned int CP25NID::getNAC() const
{
	return m_nac;
}
