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

#include "P25Audio.h"

#include <cstdio>
#include <cassert>

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CP25Audio::CP25Audio() :
m_fec()
{
}

CP25Audio::~CP25Audio()
{
}

unsigned int CP25Audio::process(unsigned char* data)
{
	assert(data != NULL);

	unsigned int errs = 0U;

	unsigned char imbe[18U];

	read(data, imbe, 114U, 262U, 142U, 143U, 214U, 215U);
	errs += m_fec.regenerateIMBE(imbe);
	write(data, imbe, 114U, 262U, 142U, 143U, 214U, 215U);

	read(data, imbe, 262U, 410U, 286U, 287U, 358U, 359U);
	errs += m_fec.regenerateIMBE(imbe);
	write(data, imbe, 262U, 410U, 286U, 287U, 358U, 359U);

	read(data, imbe, 452U, 600U, 502U, 503U, 574U, 575U);
	errs += m_fec.regenerateIMBE(imbe);
	write(data, imbe, 452U, 600U, 502U, 503U, 574U, 575U);

	read(data, imbe, 640U, 788U, 646U, 647U, 718U, 719U);
	errs += m_fec.regenerateIMBE(imbe);
	write(data, imbe, 640U, 788U, 646U, 647U, 718U, 719U);

	read(data, imbe, 830U, 978U, 862U, 863U, 934U, 935U);
	errs += m_fec.regenerateIMBE(imbe);
	write(data, imbe, 830U, 978U, 862U, 863U, 934U, 935U);

	read(data, imbe, 1020U, 1168U, 1078U, 1079U, 1150U, 1151U);
	errs += m_fec.regenerateIMBE(imbe);
	write(data, imbe, 1020U, 1168U, 1078U, 1079U, 1150U, 1151U);

	read(data, imbe, 1208U, 1356U, 1222U, 1223U, 1294U, 1295U);
	errs += m_fec.regenerateIMBE(imbe);
	write(data, imbe, 1208U, 1356U, 1222U, 1223U, 1294U, 1295U);

	read(data, imbe, 1398U, 1546U, 1438U, 1439U, 1510U, 1511U);
	errs += m_fec.regenerateIMBE(imbe);
	write(data, imbe, 1398U, 1546U, 1438U, 1439U, 1510U, 1511U);

	read(data, imbe, 1578U, 1726U, 1582U, 1583U, 1654U, 1655U);
	errs += m_fec.regenerateIMBE(imbe);
	write(data, imbe, 1578U, 1726U, 1582U, 1583U, 1654U, 1655U);

	return errs;
}

void CP25Audio::read(const unsigned char* data, unsigned char* out, unsigned int start, unsigned int stop, unsigned int avoid1, unsigned int avoid2, unsigned int avoid3, unsigned int avoid4)
{
	unsigned int n = 0U;
	for (unsigned int offset = start; offset < stop; offset++) {
		if (offset != avoid1 && offset != avoid2 && offset != avoid3 && offset != avoid4) {
			bool b = READ_BIT(data, offset);
			WRITE_BIT(out, n, b);
			n++;
		}
	}
}

void CP25Audio::write(unsigned char* data, const unsigned char* in, unsigned int start, unsigned int stop, unsigned int avoid1, unsigned int avoid2, unsigned int avoid3, unsigned int avoid4)
{
	unsigned int n = 0U;
	for (unsigned int offset = start; offset < stop; offset++) {
		if (offset != avoid1 && offset != avoid2 && offset != avoid3 && offset != avoid4) {
			bool b = READ_BIT(in, n);
			WRITE_BIT(data, offset, b);
			n++;
		}
	}
}
