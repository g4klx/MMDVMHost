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

#include "P25Data.h"
#include "Log.h"

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CP25Data::CP25Data() :
m_source(0U),
m_group(true),
m_dest(0U)
{
}

CP25Data::~CP25Data()
{
}

void CP25Data::processHeader(unsigned char* data)
{
}

void CP25Data::processLDU1(unsigned char* data)
{
	// XXX No FEC done yet
	bool b[24U];
	b[7U] = READ_BIT(data, 411U);
	b[6U] = READ_BIT(data, 410U);
	b[5U] = READ_BIT(data, 413U);
	b[4U] = READ_BIT(data, 412U);
	b[3U] = READ_BIT(data, 415U);
	b[2U] = READ_BIT(data, 414U);
	b[1U] = READ_BIT(data, 421U);
	b[0U] = READ_BIT(data, 420U);

	unsigned char format = 0U;
	unsigned char mult   = 1U;
	for (unsigned int i = 0U; i < 8U; i++, mult <<= 1)
		format += b[i] ? mult : 0U;

	LogDebug("P25, LC_format = $%02X", format);

	if (format == 0x03U) {
		LogDebug("P25, non talk group destination");
		m_group = false;
	} else {
		m_group = true;

		b[15U] = READ_BIT(data, 613U);	// 39
		b[14U] = READ_BIT(data, 612U);
		b[13U] = READ_BIT(data, 615U);	// 37
		b[12U] = READ_BIT(data, 614U);
		b[11U] = READ_BIT(data, 621U);	// 35
		b[10U] = READ_BIT(data, 620U);
		b[9U] = READ_BIT(data, 623U);	// 33
		b[8U] = READ_BIT(data, 622U);
		b[7U] = READ_BIT(data, 625U);	// 31
		b[6U] = READ_BIT(data, 624U);
		b[5U] = READ_BIT(data, 631U);	// 29
		b[4U] = READ_BIT(data, 630U);
		b[3U] = READ_BIT(data, 633U);	// 27
		b[2U] = READ_BIT(data, 632U);
		b[1U] = READ_BIT(data, 635U);
		b[0U] = READ_BIT(data, 634U);	// 24

		mult = 1U;
		for (unsigned int i = 0U; i < 16U; i++, mult <<= 1)
			m_dest += b[i] ? mult : 0U;

		LogDebug("P25, TG ID = %u", m_dest);
	}

	b[23U] = READ_BIT(data, 789U);
	b[22U] = READ_BIT(data, 788U);
	b[21U] = READ_BIT(data, 793U);
	b[20U] = READ_BIT(data, 792U);
	b[19U] = READ_BIT(data, 795U);
	b[18U] = READ_BIT(data, 794U);
	b[17U] = READ_BIT(data, 801U);
	b[16U] = READ_BIT(data, 800U);
	b[15U] = READ_BIT(data, 803U);
	b[14U] = READ_BIT(data, 802U);
	b[13U] = READ_BIT(data, 805U);
	b[12U] = READ_BIT(data, 804U);
	b[11U] = READ_BIT(data, 811U);
	b[10U] = READ_BIT(data, 810U);
	b[9U]  = READ_BIT(data, 813U);
	b[8U]  = READ_BIT(data, 812U);
	b[7U]  = READ_BIT(data, 815U);
	b[6U]  = READ_BIT(data, 814U);
	b[5U]  = READ_BIT(data, 821U);
	b[4U]  = READ_BIT(data, 820U);
	b[3U]  = READ_BIT(data, 823U);
	b[2U]  = READ_BIT(data, 822U);
	b[1U]  = READ_BIT(data, 825U);
	b[0U]  = READ_BIT(data, 824U);

	mult = 1U;
	for (unsigned int i = 0U; i < 24U; i++, mult <<= 1)
		m_source += b[i] ? mult : 0U;

	LogDebug("P25, SRC ID = %u", m_source);
}

void CP25Data::processLDU2(unsigned char* data)
{
}

void CP25Data::processTerminator(unsigned char* data)
{
}

unsigned int CP25Data::getSource() const
{
	return m_source;
}

bool CP25Data::getGroup() const
{
	return m_group;
}

unsigned int CP25Data::getDest() const
{
	return m_dest;
}

void CP25Data::reset()
{
	m_source = 0U;
	m_dest = 0U;
}
