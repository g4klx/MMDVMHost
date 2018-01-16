/*
*	Copyright (C) 2016,2018 by Jonathan Naylor, G4KLX
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

#include "P25Trellis.h"
#include "Log.h"

#include <cstdio>
#include <cassert>

const unsigned int INTERLEAVE_TABLE[] = {
	0U, 1U, 8U,   9U, 16U, 17U, 24U, 25U, 32U, 33U, 40U, 41U, 48U, 49U, 56U, 57U, 64U, 65U, 72U, 73U, 80U, 81U, 88U, 89U, 96U, 97U,
	2U, 3U, 10U, 11U, 18U, 19U, 26U, 27U, 34U, 35U, 42U, 43U, 50U, 51U, 58U, 59U, 66U, 67U, 74U, 75U, 82U, 83U, 90U, 91U,
	4U, 5U, 12U, 13U, 20U, 21U, 28U, 29U, 36U, 37U, 44U, 45U, 52U, 53U, 60U, 61U, 68U, 69U, 76U, 77U, 84U, 85U, 92U, 93U,
	6U, 7U, 14U, 15U, 22U, 23U, 30U, 31U, 38U, 39U, 46U, 47U, 54U, 55U, 62U, 63U, 70U, 71U, 78U, 79U, 86U, 87U, 94U, 95U};

const unsigned char ENCODE_TABLE_34[] = {
	0U,  8U, 4U, 12U, 2U, 10U, 6U, 14U,
	4U, 12U, 2U, 10U, 6U, 14U, 0U,  8U,
	1U,  9U, 5U, 13U, 3U, 11U, 7U, 15U,
	5U, 13U, 3U, 11U, 7U, 15U, 1U,  9U,
	3U, 11U, 7U, 15U, 1U,  9U, 5U, 13U,
	7U, 15U, 1U,  9U, 5U, 13U, 3U, 11U,
	2U, 10U, 6U, 14U, 0U,  8U, 4U, 12U,
	6U, 14U, 0U,  8U, 4U, 12U, 2U, 10U};

const unsigned char ENCODE_TABLE_12[] = {
	0U,  15U, 12U,  3U,
	4U,  11U,  8U,  7U,
	13U,  2U,  1U, 14U,
	9U,   6U,  5U, 10U};

const unsigned char BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CP25Trellis::CP25Trellis()
{
}

CP25Trellis::~CP25Trellis()
{
}

bool CP25Trellis::decode34(const unsigned char* data, unsigned char* payload)
{
	assert(data != NULL);
	assert(payload != NULL);

	signed char dibits[98U];
	deinterleave(data, dibits);

	unsigned char points[49U];
	dibitsToPoints(dibits, points);

	// Check the original code
	unsigned char tribits[49U];
	unsigned int failPos = checkCode34(points, tribits);
	if (failPos == 999U) {
		tribitsToBits(tribits, payload);
		return true;
	}

	unsigned char savePoints[49U];
	for (unsigned int i = 0U; i < 49U; i++)
		savePoints[i] = points[i];

	bool ret = fixCode34(points, failPos, payload);
	if (ret)
		return true;

	if (failPos == 0U)
		return false;

	// Backtrack one place for a last go
	return fixCode34(savePoints, failPos - 1U, payload);
}

void CP25Trellis::encode34(const unsigned char* payload, unsigned char* data)
{
	assert(payload != NULL);
	assert(data != NULL);

	unsigned char tribits[49U];
	bitsToTribits(payload, tribits);

	unsigned char points[49U];
	unsigned char state = 0U;

	for (unsigned int i = 0U; i < 49U; i++) {
		unsigned char tribit = tribits[i];

		points[i] = ENCODE_TABLE_34[state * 8U + tribit];

		state = tribit;
	}

	signed char dibits[98U];
	pointsToDibits(points, dibits);

	interleave(dibits, data);
}

bool CP25Trellis::decode12(const unsigned char* data, unsigned char* payload)
{
	assert(data != NULL);
	assert(payload != NULL);

	signed char dibits[98U];
	deinterleave(data, dibits);

	unsigned char points[49U];
	dibitsToPoints(dibits, points);

	// Check the original code
	unsigned char bits[49U];
	unsigned int failPos = checkCode12(points, bits);
	if (failPos == 999U) {
		dibitsToBits(bits, payload);
		return true;
	}

	unsigned char savePoints[49U];
	for (unsigned int i = 0U; i < 49U; i++)
		savePoints[i] = points[i];

	bool ret = fixCode12(points, failPos, payload);
	if (ret)
		return true;

	if (failPos == 0U)
		return false;

	// Backtrack one place for a last go
	return fixCode12(savePoints, failPos - 1U, payload);
}

void CP25Trellis::encode12(const unsigned char* payload, unsigned char* data)
{
	assert(payload != NULL);
	assert(data != NULL);

	unsigned char bits[49U];
	bitsToDibits(payload, bits);

	unsigned char points[49U];
	unsigned char state = 0U;

	for (unsigned int i = 0U; i < 49U; i++) {
		unsigned char bit = bits[i];

		points[i] = ENCODE_TABLE_12[state * 4U + bit];

		state = bit;
	}

	signed char dibits[98U];
	pointsToDibits(points, dibits);

	interleave(dibits, data);
}

void CP25Trellis::deinterleave(const unsigned char* data, signed char* dibits) const
{
	for (unsigned int i = 0U; i < 98U; i++) {
		unsigned int n = i * 2U + 0U;
		bool b1 = READ_BIT(data, n) != 0x00U;

		n = i * 2U + 1U;
		bool b2 = READ_BIT(data, n) != 0x00U;

		signed char dibit;
		if (!b1 && b2)
			dibit = +3;
		else if (!b1 && !b2)
			dibit = +1;
		else if (b1 && !b2)
			dibit = -1;
		else
			dibit = -3;

		n = INTERLEAVE_TABLE[i];
		dibits[n] = dibit;
	}
}

void CP25Trellis::interleave(const signed char* dibits, unsigned char* data) const
{
	for (unsigned int i = 0U; i < 98U; i++) {
		unsigned int n = INTERLEAVE_TABLE[i];

		bool b1, b2;
		switch (dibits[n]) {
		case +3:
			b1 = false;
			b2 = true;
			break;
		case +1:
			b1 = false;
			b2 = false;
			break;
		case -1:
			b1 = true;
			b2 = false;
			break;
		default:
			b1 = true;
			b2 = true;
			break;
		}

		n = i * 2U + 0U;
		WRITE_BIT(data, n, b1);

		n = i * 2U + 1U;
		WRITE_BIT(data, n, b2);
	}
}

void CP25Trellis::dibitsToPoints(const signed char* dibits, unsigned char* points) const
{
	for (unsigned int i = 0U; i < 49U; i++) {
		if (dibits[i * 2U + 0U] == +1 && dibits[i * 2U + 1U] == -1)
			points[i] = 0U;
		else if (dibits[i * 2U + 0U] == -1 && dibits[i * 2U + 1U] == -1)
			points[i] = 1U;
		else if (dibits[i * 2U + 0U] == +3 && dibits[i * 2U + 1U] == -3)
			points[i] = 2U;
		else if (dibits[i * 2U + 0U] == -3 && dibits[i * 2U + 1U] == -3)
			points[i] = 3U;
		else if (dibits[i * 2U + 0U] == -3 && dibits[i * 2U + 1U] == -1)
			points[i] = 4U;
		else if (dibits[i * 2U + 0U] == +3 && dibits[i * 2U + 1U] == -1)
			points[i] = 5U;
		else if (dibits[i * 2U + 0U] == -1 && dibits[i * 2U + 1U] == -3)
			points[i] = 6U;
		else if (dibits[i * 2U + 0U] == +1 && dibits[i * 2U + 1U] == -3)
			points[i] = 7U;
		else if (dibits[i * 2U + 0U] == -3 && dibits[i * 2U + 1U] == +3)
			points[i] = 8U;
		else if (dibits[i * 2U + 0U] == +3 && dibits[i * 2U + 1U] == +3)
			points[i] = 9U;
		else if (dibits[i * 2U + 0U] == -1 && dibits[i * 2U + 1U] == +1)
			points[i] = 10U;
		else if (dibits[i * 2U + 0U] == +1 && dibits[i * 2U + 1U] == +1)
			points[i] = 11U;
		else if (dibits[i * 2U + 0U] == +1 && dibits[i * 2U + 1U] == +3)
			points[i] = 12U;
		else if (dibits[i * 2U + 0U] == -1 && dibits[i * 2U + 1U] == +3)
			points[i] = 13U;
		else if (dibits[i * 2U + 0U] == +3 && dibits[i * 2U + 1U] == +1)
			points[i] = 14U;
		else if (dibits[i * 2U + 0U] == -3 && dibits[i * 2U + 1U] == +1)
			points[i] = 15U;
	}
}

void CP25Trellis::pointsToDibits(const unsigned char* points, signed char* dibits) const
{
	for (unsigned int i = 0U; i < 49U; i++) {
		switch (points[i]) {
		case 0U:
			dibits[i * 2U + 0U] = +1;
			dibits[i * 2U + 1U] = -1;
			break;
		case 1U:
			dibits[i * 2U + 0U] = -1;
			dibits[i * 2U + 1U] = -1;
			break;
		case 2U:
			dibits[i * 2U + 0U] = +3;
			dibits[i * 2U + 1U] = -3;
			break;
		case 3U:
			dibits[i * 2U + 0U] = -3;
			dibits[i * 2U + 1U] = -3;
			break;
		case 4U:
			dibits[i * 2U + 0U] = -3;
			dibits[i * 2U + 1U] = -1;
			break;
		case 5U:
			dibits[i * 2U + 0U] = +3;
			dibits[i * 2U + 1U] = -1;
			break;
		case 6U:
			dibits[i * 2U + 0U] = -1;
			dibits[i * 2U + 1U] = -3;
			break;
		case 7U:
			dibits[i * 2U + 0U] = +1;
			dibits[i * 2U + 1U] = -3;
			break;
		case 8U:
			dibits[i * 2U + 0U] = -3;
			dibits[i * 2U + 1U] = +3;
			break;
		case 9U:
			dibits[i * 2U + 0U] = +3;
			dibits[i * 2U + 1U] = +3;
			break;
		case 10U:
			dibits[i * 2U + 0U] = -1;
			dibits[i * 2U + 1U] = +1;
			break;
		case 11U:
			dibits[i * 2U + 0U] = +1;
			dibits[i * 2U + 1U] = +1;
			break;
		case 12U:
			dibits[i * 2U + 0U] = +1;
			dibits[i * 2U + 1U] = +3;
			break;
		case 13U:
			dibits[i * 2U + 0U] = -1;
			dibits[i * 2U + 1U] = +3;
			break;
		case 14U:
			dibits[i * 2U + 0U] = +3;
			dibits[i * 2U + 1U] = +1;
			break;
		default:
			dibits[i * 2U + 0U] = -3;
			dibits[i * 2U + 1U] = +1;
			break;
		}
	}
}

void CP25Trellis::bitsToTribits(const unsigned char* payload, unsigned char* tribits) const
{
	for (unsigned int i = 0U; i < 48U; i++) {
		unsigned int n = i * 3U;

		bool b1 = READ_BIT(payload, n) != 0x00U;
		n++;
		bool b2 = READ_BIT(payload, n) != 0x00U;
		n++;
		bool b3 = READ_BIT(payload, n) != 0x00U;

		unsigned char tribit = 0U;
		tribit |= b1 ? 4U : 0U;
		tribit |= b2 ? 2U : 0U;
		tribit |= b3 ? 1U : 0U;

		tribits[i] = tribit;
	}

	tribits[48U] = 0U;
}

void CP25Trellis::bitsToDibits(const unsigned char* payload, unsigned char* dibits) const
{
	for (unsigned int i = 0U; i < 48U; i++) {
		unsigned int n = i * 2U;

		bool b1 = READ_BIT(payload, n) != 0x00U;
		n++;
		bool b2 = READ_BIT(payload, n) != 0x00U;

		unsigned char dibit = 0U;
		dibit |= b1 ? 2U : 0U;
		dibit |= b2 ? 1U : 0U;

		dibits[i] = dibit;
	}

	dibits[48U] = 0U;
}

void CP25Trellis::tribitsToBits(const unsigned char* tribits, unsigned char* payload) const
{
	for (unsigned int i = 0U; i < 48U; i++) {
		unsigned char tribit = tribits[i];

		bool b1 = (tribit & 0x04U) == 0x04U;
		bool b2 = (tribit & 0x02U) == 0x02U;
		bool b3 = (tribit & 0x01U) == 0x01U;

		unsigned int n = i * 3U;

		WRITE_BIT(payload, n, b1);
		n++;
		WRITE_BIT(payload, n, b2);
		n++;
		WRITE_BIT(payload, n, b3);
	}
}

void CP25Trellis::dibitsToBits(const unsigned char* dibits, unsigned char* payload) const
{
	for (unsigned int i = 0U; i < 48U; i++) {
		unsigned char dibit = dibits[i];

		bool b1 = (dibit & 0x02U) == 0x02U;
		bool b2 = (dibit & 0x01U) == 0x01U;

		unsigned int n = i * 2U;

		WRITE_BIT(payload, n, b1);
		n++;
		WRITE_BIT(payload, n, b2);
	}
}

bool CP25Trellis::fixCode34(unsigned char* points, unsigned int failPos, unsigned char* payload) const
{
	for (unsigned j = 0U; j < 20U; j++) {
		unsigned int bestPos = 0U;
		unsigned int bestVal = 0U;

		for (unsigned int i = 0U; i < 16U; i++) {
			points[failPos] = i;

			unsigned char tribits[49U];
			unsigned int pos = checkCode34(points, tribits);
			if (pos == 999U) {
				tribitsToBits(tribits, payload);
				return true;
			}

			if (pos > bestPos) {
				bestPos = pos;
				bestVal = i;
			}
		}

		points[failPos] = bestVal;
		failPos = bestPos;
	}

	return false;
}

unsigned int CP25Trellis::checkCode34(const unsigned char* points, unsigned char* tribits) const
{
	unsigned char state = 0U;

	for (unsigned int i = 0U; i < 49U; i++) {
		tribits[i] = 9U;

		for (unsigned int j = 0U; j < 8U; j++) {
			if (points[i] == ENCODE_TABLE_34[state * 8U + j]) {
				tribits[i] = j;
				break;
			}
		}

		if (tribits[i] == 9U)
			return i;

		state = tribits[i];
	}

	if (tribits[48U] != 0U)
		return 48U;

	return 999U;
}


bool CP25Trellis::fixCode12(unsigned char* points, unsigned int failPos, unsigned char* payload) const
{
	for (unsigned j = 0U; j < 20U; j++) {
		unsigned int bestPos = 0U;
		unsigned int bestVal = 0U;

		for (unsigned int i = 0U; i < 4U; i++) {
			points[failPos] = i;

			unsigned char dibits[49U];
			unsigned int pos = checkCode12(points, dibits);
			if (pos == 999U) {
				dibitsToBits(dibits, payload);
				return true;
			}

			if (pos > bestPos) {
				bestPos = pos;
				bestVal = i;
			}
		}

		points[failPos] = bestVal;
		failPos = bestPos;
	}

	return false;
}

unsigned int CP25Trellis::checkCode12(const unsigned char* points, unsigned char* dibits) const
{
	unsigned char state = 0U;

	for (unsigned int i = 0U; i < 49U; i++) {
		dibits[i] = 5U;

		for (unsigned int j = 0U; j < 4U; j++) {
			if (points[i] == ENCODE_TABLE_12[state * 4U + j]) {
				dibits[i] = j;
				break;
			}
		}

		if (dibits[i] == 5U)
			return i;

		state = dibits[i];
	}

	if (dibits[48U] != 0U)
		return 48U;

	return 999U;
}
