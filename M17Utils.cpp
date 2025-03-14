/*
 *   Copyright (C) 2020,2021,2024,2025 by Jonathan Naylor G4KLX
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

#include "M17Utils.h"
#include "M17Defines.h"

#include <cstdint>
#include <cassert>

const std::string M17_CHARS = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/.";

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

void CM17Utils::encodeCallsign(const std::string& callsign, unsigned char* encoded)
{
	assert(encoded != nullptr);

	if (callsign == "ALL" || callsign == "ALL      ") {
		encoded[0U] = 0xFFU;
		encoded[1U] = 0xFFU;
		encoded[2U] = 0xFFU;
		encoded[3U] = 0xFFU;
		encoded[4U] = 0xFFU;
		encoded[5U] = 0xFFU;
		return;
	}

	unsigned int len = (unsigned int)callsign.size();
	if (len > 9U)
		len = 9U;

	uint64_t enc = 0ULL;
	for (int i = len - 1; i >= 0; i--) {
		if ((i == 0) && (callsign[i] == '#')) {
			enc += 262144000000000ULL;
		} else {
			size_t pos = M17_CHARS.find(callsign[i]);
			if (pos == std::string::npos)
				pos = 0ULL;

			enc *= 40ULL;
			enc += pos;
		}
	}

	encoded[0U] = (enc >> 40) & 0xFFU;
	encoded[1U] = (enc >> 32) & 0xFFU;
	encoded[2U] = (enc >> 24) & 0xFFU;
	encoded[3U] = (enc >> 16) & 0xFFU;
	encoded[4U] = (enc >> 8) & 0xFFU;
	encoded[5U] = (enc >> 0) & 0xFFU;
}

void CM17Utils::decodeCallsign(const unsigned char* encoded, std::string& callsign)
{
	assert(encoded != nullptr);

	callsign.clear();

	uint64_t enc = (uint64_t(encoded[0U]) << 40) +
	               (uint64_t(encoded[1U]) << 32) +
	               (uint64_t(encoded[2U]) << 24) +
	               (uint64_t(encoded[3U]) << 16) +
	               (uint64_t(encoded[4U]) << 8) +
	               (uint64_t(encoded[5U]) << 0);

	if (enc == 281474976710655ULL) {
		callsign = "ALL";
		return;
	}

	if (enc >= 268697600000000ULL) {
		callsign = "Invalid";
		return;
	}

	if (enc >= 262144000000000ULL) {
		callsign = "#";
		enc -= 262144000000000ULL;
	}

	while (enc > 0ULL) {
		callsign += " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/."[enc % 40ULL];
		enc /= 40ULL;
	}
}

void CM17Utils::splitFragmentLICH(const unsigned char* data, unsigned int& frag1, unsigned int& frag2, unsigned int& frag3, unsigned int& frag4)
{
	assert(data != nullptr);

	frag1 = frag2 = frag3 = frag4 = 0x00U;

	unsigned int offset = 0U;
	unsigned int MASK = 0x800U;
	for (unsigned int i = 0U; i < (M17_LICH_FRAGMENT_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = READ_BIT1(data, offset) != 0x00U;
		if (b)
			frag1 |= MASK;
	}

	MASK = 0x800U;
	for (unsigned int i = 0U; i < (M17_LICH_FRAGMENT_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = READ_BIT1(data, offset) != 0x00U;
		if (b)
			frag2 |= MASK;
	}

	MASK = 0x800U;
	for (unsigned int i = 0U; i < (M17_LICH_FRAGMENT_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = READ_BIT1(data, offset) != 0x00U;
		if (b)
			frag3 |= MASK;
	}

	MASK = 0x800U;
	for (unsigned int i = 0U; i < (M17_LICH_FRAGMENT_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = READ_BIT1(data, offset) != 0x00U;
		if (b)
			frag4 |= MASK;
	}
}

void CM17Utils::splitFragmentLICHFEC(const unsigned char* data, unsigned int& frag1, unsigned int& frag2, unsigned int& frag3, unsigned int& frag4)
{
	assert(data != nullptr);

	frag1 = frag2 = frag3 = frag4 = 0x00U;

	unsigned int offset = 0U;
	unsigned int MASK = 0x800000U;
	for (unsigned int i = 0U; i < (M17_LICH_FRAGMENT_FEC_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = READ_BIT1(data, offset) != 0x00U;
		if (b)
			frag1 |= MASK;
	}

	MASK = 0x800000U;
	for (unsigned int i = 0U; i < (M17_LICH_FRAGMENT_FEC_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = READ_BIT1(data, offset) != 0x00U;
		if (b)
			frag2 |= MASK;
	}

	MASK = 0x800000U;
	for (unsigned int i = 0U; i < (M17_LICH_FRAGMENT_FEC_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = READ_BIT1(data, offset) != 0x00U;
		if (b)
			frag3 |= MASK;
	}

	MASK = 0x800000U;
	for (unsigned int i = 0U; i < (M17_LICH_FRAGMENT_FEC_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = READ_BIT1(data, offset) != 0x00U;
		if (b)
			frag4 |= MASK;
	}
}

void CM17Utils::combineFragmentLICH(unsigned int frag1, unsigned int frag2, unsigned int frag3, unsigned int frag4, unsigned char* data)
{
	assert(data != nullptr);

	unsigned int offset = 0U;
	unsigned int MASK = 0x800U;
	for (unsigned int i = 0U; i < (M17_LICH_FRAGMENT_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = (frag1 & MASK) == MASK;
		WRITE_BIT1(data, offset, b);
	}

	MASK = 0x800U;
	for (unsigned int i = 0U; i < (M17_LICH_FRAGMENT_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = (frag2 & MASK) == MASK;
		WRITE_BIT1(data, offset, b);
	}

	MASK = 0x800U;
	for (unsigned int i = 0U; i < (M17_LICH_FRAGMENT_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = (frag3 & MASK) == MASK;
		WRITE_BIT1(data, offset, b);
	}

	MASK = 0x800U;
	for (unsigned int i = 0U; i < (M17_LICH_FRAGMENT_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = (frag4 & MASK) == MASK;
		WRITE_BIT1(data, offset, b);
	}
}

void CM17Utils::combineFragmentLICHFEC(unsigned int frag1, unsigned int frag2, unsigned int frag3, unsigned int frag4, unsigned char* data)
{
	assert(data != nullptr);

	unsigned int offset = 0U;
	unsigned int MASK = 0x800000U;
	for (unsigned int i = 0U; i < (M17_LICH_FRAGMENT_FEC_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = (frag1 & MASK) == MASK;
		WRITE_BIT1(data, offset, b);
	}

	MASK = 0x800000U;
	for (unsigned int i = 0U; i < (M17_LICH_FRAGMENT_FEC_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = (frag2 & MASK) == MASK;
		WRITE_BIT1(data, offset, b);
	}

	MASK = 0x800000U;
	for (unsigned int i = 0U; i < (M17_LICH_FRAGMENT_FEC_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = (frag3 & MASK) == MASK;
		WRITE_BIT1(data, offset, b);
	}

	MASK = 0x800000U;
	for (unsigned int i = 0U; i < (M17_LICH_FRAGMENT_FEC_LENGTH_BITS / 4U); i++, offset++, MASK >>= 1) {
		bool b = (frag4 & MASK) == MASK;
		WRITE_BIT1(data, offset, b);
	}
}
