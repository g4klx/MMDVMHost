/*
*	Copyright (C) 2016 Jonathan Naylor, G4KLX
*	Copyright (C) 2012 Ian Wraith
*
*	This program is free software; you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation; version 2 of the License.
*
*	This program is distributed in the hope that it will be usefulU, 
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*/

#include "Trellis.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned int INTERLEAVE[] = {
	0U, 1U,  8U,  9U, 16U, 17U, 24U, 25U, 32U, 33U, 40U, 41U, 48U, 49U, 56U, 57U, 64U, 65U, 72U, 73U, 80U, 81U, 88U, 89U, 96U, 97U, 
	2U, 3U, 10U, 11U, 18U, 19U, 26U, 27U, 34U, 35U, 42U, 43U, 50U, 51U, 58U, 59U, 66U, 67U, 74U, 75U, 82U, 83U, 90U, 91U,
	4U, 5U, 12U, 13U, 20U, 21U, 28U, 29U, 36U, 37U, 44U, 45U, 52U, 53U, 60U, 61U, 68U, 69U, 76U, 77U, 84U, 85U, 92U, 93U,
	6U, 7U, 14U, 15U, 22U, 23U, 30U, 31U, 38U, 39U, 46U, 47U, 54U, 55U, 62U, 63U, 70U, 71U, 78U, 79U, 86U, 87U, 94U, 95U};

const unsigned int STATETABLE[] = {
	0U,  8U, 4U, 12U, 2U, 10U, 6U, 14U, 
	4U, 12U, 2U, 10U, 6U, 14U, 0U,  8U, 
	1U,  9U, 5U, 13U, 3U, 11U, 7U, 15U, 
	5U, 13U, 3U, 11U, 7U, 15U, 1U,  9U, 
	3U, 11U, 7U, 15U, 1U,  9U, 5U, 13U, 
	7U, 15U, 1U,  9U, 5U, 13U, 3U, 11U, 
	2U, 10U, 6U, 14U, 0U,  8U, 4U, 12U, 
	6U, 14U, 0U,  8U, 4U, 12U, 2U, 10U};

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CTrellis::CTrellis()
{
}

CTrellis::~CTrellis()
{
}

// Converts the 3/4 rate trellis encoded bits to plain binary
void CTrellis::decode(const unsigned char* in, unsigned char* out)
{
	assert(in != NULL);
	assert(out != NULL);

	int8_t dibits[98U];
	extractDiBits(in, dibits);

	uint8_t cons[49U];
	constellationOut(dibits, cons);

	uint16_t tris[49U];
	bool ret = tribitExtract(cons, tris);
	// If the output of tribitExtract() is false then we have an error so return
	if (!ret)
		return;

	binaryConvert(tris, out);
}

// Extract and deinterleave the dibits
void CTrellis::extractDiBits(const unsigned char* in, int8_t* dibits) const
{
	for (unsigned int index = 0U; index < 98U; index++) {
		unsigned int a = index * 2U;
		if (a >= 98U)
			a += 68U;

		bool b0 = READ_BIT(in, a) != 0x00U;
		a++;
		bool b1 = READ_BIT(in, a) != 0x00U;

		// Set the dibits
		// 01 = +3
		// 00 = +1
		// 10 = -1
		// 11 = -3
		uint8_t dibit = 0U;
		if (!b0 && b1) dibit = +3;
		else if (!b0 && !b1) dibit = +1;
		else if (b0 && !b1) dibit = -1;
		else if (b0 && b1) dibit = -3;
		// Deinterleave
		unsigned int deinterleave = INTERLEAVE[index];
		dibits[deinterleave] = dibit;
	}
}

// Extract the constellation points
void CTrellis::constellationOut(const int8_t* dibits, uint8_t* cons) const
{
	unsigned int index = 0U;
	for (unsigned int a = 0U; a < 98U; a += 2U, index++) {
		if ((dibits[a] == +1) && (dibits[a + 1] == -1)) cons[index] = 0;
		else if ((dibits[a] == -1) && (dibits[a + 1] == -1)) cons[index] = 1;
		else if ((dibits[a] == +3) && (dibits[a + 1] == -3)) cons[index] = 2;
		else if ((dibits[a] == -3) && (dibits[a + 1] == -3)) cons[index] = 3;
		else if ((dibits[a] == -3) && (dibits[a + 1] == -1)) cons[index] = 4;
		else if ((dibits[a] == +3) && (dibits[a + 1] == -1)) cons[index] = 5;
		else if ((dibits[a] == -1) && (dibits[a + 1] == -3)) cons[index] = 6;
		else if ((dibits[a] == +1) && (dibits[a + 1] == -3)) cons[index] = 7;
		else if ((dibits[a] == -3) && (dibits[a + 1] == +3)) cons[index] = 8;
		else if ((dibits[a] == +3) && (dibits[a + 1] == +3)) cons[index] = 9;
		else if ((dibits[a] == -1) && (dibits[a + 1] == +1)) cons[index] = 10;
		else if ((dibits[a] == +1) && (dibits[a + 1] == +1)) cons[index] = 11;
		else if ((dibits[a] == +1) && (dibits[a + 1] == +3)) cons[index] = 12;
		else if ((dibits[a] == -1) && (dibits[a + 1] == +3)) cons[index] = 13;
		else if ((dibits[a] == +3) && (dibits[a + 1] == +1)) cons[index] = 14;
		else if ((dibits[a] == -3) && (dibits[a + 1] == +1)) cons[index] = 15;
	}
}

// Extract tribits (as ints) from the constellation points
bool CTrellis::tribitExtract(const uint8_t* cons, uint16_t* tris) const
{
	unsigned int lastState = 0U;
	for (unsigned int a = 0U; a < 49U; a++) {
		// The lastState variable decides which row of STATETABLE we should use
		unsigned int rowStart = lastState * 8;
		bool match = false;
		for (unsigned int b = rowStart; b < (rowStart + 8U); b++) {
			// Check if this constellation point matches an element of this row of STATETABLE
			if (cons[a] == STATETABLE[b]) {
				// Yes it does
				match = true;
				lastState = b - rowStart;
				tris[a] = lastState;
			}
		}

		// If no match found then we have a problem
		if (!match)
			return false;
	}

	return true;
}

// Extract the 144 binary bits from the dibits
void CTrellis::binaryConvert(const uint16_t* tris, unsigned char* out) const
{
	unsigned int a = 0U;
	for (unsigned int b = 0U; b < 48U; b++) {
		// Convert three bits at a time
		WRITE_BIT(out, a, (tris[b] & 0x04U));
		a++;
		WRITE_BIT(out, a, (tris[b] & 0x02U));
		a++;
		WRITE_BIT(out, a, (tris[b] & 0x01U));
		a++;
	}
}
