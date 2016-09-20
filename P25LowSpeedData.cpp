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

#include "P25LowSpeedData.h"
#include "P25Utils.h"

#include <cstdio>
#include <cassert>

const unsigned int MAX_CCS_ERRS = 4U;

CP25LowSpeedData::CP25LowSpeedData()
{
}

CP25LowSpeedData::~CP25LowSpeedData()
{
}

void CP25LowSpeedData::process(unsigned char* data)
{
	assert(data != NULL);

	unsigned char lsd[4U];
	CP25Utils::decode(data, lsd, 1546U, 1578U);

	for (unsigned int a = 0x00U; a < 0x100U; a++) {
		unsigned char ccs[2U];
		ccs[0U] = a;
		ccs[1U] = encode(ccs[0U]);

		unsigned int errs = CP25Utils::compare(ccs, lsd + 0U, 2U);
		if (errs < MAX_CCS_ERRS) {
			lsd[0U] = ccs[0U];
			lsd[1U] = ccs[1U];
			break;
		}
	}

	for (unsigned int a = 0x00U; a < 0x100U; a++) {
		unsigned char ccs[2U];
		ccs[0U] = a;
		ccs[1U] = encode(ccs[0U]);

		unsigned int errs = CP25Utils::compare(ccs, lsd + 2U, 2U);
		if (errs < MAX_CCS_ERRS) {
			lsd[2U] = ccs[0U];
			lsd[3U] = ccs[1U];
			break;
		}
	}
}

unsigned char CP25LowSpeedData::encode(unsigned char in)
{
	unsigned char result = 0x00U;

	if ((in & 0x01U) == 0x01U)
		result ^= 0x39U;

	if ((in & 0x02U) == 0x02U)
		result ^= 0x72U;

	if ((in & 0x04U) == 0x04U)
		result ^= 0xE4U;

	if ((in & 0x08U) == 0x08U)
		result ^= 0xF1U;

	if ((in & 0x10U) == 0x10U)
		result ^= 0xDBU;

	if ((in & 0x20U) == 0x20U)
		result ^= 0x8FU;

	if ((in & 0x40U) == 0x40U)
		result ^= 0x27U;

	if ((in & 0x80U) == 0x80U)
		result ^= 0x4EU;

	return result;
}
