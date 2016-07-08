/*
*	Copyright (C) 2016 by Jonathan Naylor, G4KLX
*
*	This program is free software; you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation; version 2 of the License.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*/

#include "DMRTrellis.h"
#include "DMRDefines.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>

const unsigned int INTERLEAVE_TABLE[] = {
	0U, 4U,  8U, 12U, 16U, 20U, 24U, 28U, 32U, 36U, 40U, 44U, 48U,
	1U, 5U,  9U, 13U, 17U, 21U, 25U, 29U, 33U, 37U, 41U, 45U,
	2U, 6U, 10U, 14U, 18U, 22U, 26U, 30U, 34U, 38U, 42U, 46U,
	3U, 7U, 11U, 15U, 19U, 23U, 27U, 31U, 35U, 39U, 43U, 47U};

const unsigned char DIBITS_TO_POINT[] = { 11U,  12U,   0U,   7U,  14U,   9U,   5U,   2U,  10U,  13U,   1U,   6U,  15U,   8U,   4U,   3U};
const unsigned char POINT_TO_DIBITS[] = {0x2U, 0xAU, 0x7U, 0xFU, 0xEU, 0x6U, 0xBU, 0x3U, 0xDU, 0x5U, 0x8U, 0x0U, 0x1U, 0x9U, 0x4U, 0xCU};

const unsigned char ENCODE_TABLE[] = {
	0U,  8U, 4U, 12U, 2U, 10U, 6U, 14U,
	4U, 12U, 2U, 10U, 6U, 14U, 0U,  8U,
	1U,  9U, 5U, 13U, 3U, 11U, 7U, 15U,
	5U, 13U, 3U, 11U, 7U, 15U, 1U,  9U,
	3U, 11U, 7U, 15U, 1U,  9U, 5U, 13U,
	7U, 15U, 1U, 9U,  5U, 13U, 3U, 11U,
	2U, 10U, 6U, 14U, 0U,  8U, 4U, 12U,
	6U, 14U, 0U,  8U, 4U, 12U, 2U, 10U
};

const unsigned char BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CDMRTrellis::CDMRTrellis()
{
}

CDMRTrellis::~CDMRTrellis()
{
}

void CDMRTrellis::decode(const unsigned char* data, unsigned char* payload)
{
	assert(data != NULL);
	assert(payload != NULL);

	CUtils::dump(1U, "Payload", data, DMR_FRAME_LENGTH_BYTES);

	// unsigned char points[49U];
	// deinterleave(data, points);
}

void CDMRTrellis::encode(const unsigned char* payload, unsigned char* data)
{
	assert(payload != NULL);
	assert(data != NULL);

	unsigned char tribits[49U];
	totribits(payload, tribits);

	unsigned char points[49U];
	unsigned char state = 0U;

	for (unsigned int i = 0U; i < 49U; i++) {
		unsigned char tribit = tribits[i];

		points[i] = ENCODE_TABLE[state * 8U + tribit];

		state = tribit;
	}

	interleave(points, data);
}

void CDMRTrellis::deinterleave(const unsigned char* data, unsigned char* points) const
{
	for (unsigned int i = 0U; i < 49U; i++) {
		unsigned int n = INTERLEAVE_TABLE[i] * 4U;
		if (n > 108U) n += 48U;

		bool b1 = READ_BIT(data, n) != 0x00U;
		n++;
		bool b2 = READ_BIT(data, n) != 0x00U;
		n++;
		bool b3 = READ_BIT(data, n) != 0x00U;
		n++;
		bool b4 = READ_BIT(data, n) != 0x00U;

		unsigned int dibits = 0U;
		dibits |= b1 ? 8U : 0U;
		dibits |= b2 ? 4U : 0U;
		dibits |= b3 ? 2U : 0U;
		dibits |= b4 ? 1U : 0U;

		points[i] = DIBITS_TO_POINT[dibits];
	}
}

void CDMRTrellis::interleave(const unsigned char* points, unsigned char* data) const
{
	for (unsigned int i = 0U; i < 49U; i++) {
		unsigned char point = points[i];
		unsigned char dibits = POINT_TO_DIBITS[point];

		bool b1 = (dibits & 0x08U) == 0x08U;
		bool b2 = (dibits & 0x04U) == 0x04U;
		bool b3 = (dibits & 0x02U) == 0x02U;
		bool b4 = (dibits & 0x01U) == 0x01U;

		unsigned int n = INTERLEAVE_TABLE[i] * 4U;
		if (n > 108U) n += 48U;

		WRITE_BIT(data, n, b1);
		n++;
		WRITE_BIT(data, n, b2);
		n++;
		WRITE_BIT(data, n, b3);
		n++;
		WRITE_BIT(data, n, b4);
	}
}

void CDMRTrellis::totribits(const unsigned char* payload, unsigned char* tribits) const
{
	for (unsigned int i = 0U; i < 48U; i++) {
		unsigned int n = 143U - i * 3U;

		bool b1 = READ_BIT(payload, n) != 0x00U;
		n--;
		bool b2 = READ_BIT(payload, n) != 0x00U;
		n--;
		bool b3 = READ_BIT(payload, n) != 0x00U;

		unsigned char tribit = 0U;
		tribit |= b1 ? 4U : 0U;
		tribit |= b2 ? 2U : 0U;
		tribit |= b3 ? 1U : 0U;

		tribits[i] = tribit;
	}

	tribits[48U] = 0U;
}
