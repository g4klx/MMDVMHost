/*
 *   Copyright (C) 2020 by Jonathan Naylor G4KLX
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

#include <cassert>

const std::string M17_CHARS = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/.";

void CM17Utils::encodeCallsign(const std::string& callsign, unsigned char* encoded)
{
	assert(encoded != NULL);

	unsigned int len = callsign.size();
	if (len > 9U)
		len = 9U;

	uint64_t enc = 0ULL;
	for (int i = len - 1; i >= 0; i--) {
		size_t pos = M17_CHARS.find(callsign[i]);
		if (pos == std::string::npos)
			pos = 0ULL;

		enc *= 40ULL;
		enc += pos;
	}

	encoded[0U] = enc >> 40;
	encoded[1U] = enc >> 32;
	encoded[2U] = enc >> 24;
	encoded[3U] = enc >> 16;
	encoded[4U] = enc >> 8;
	encoded[5U] = enc >> 0;
}

void CM17Utils::decodeCallsign(const unsigned char* encoded, std::string& callsign)
{
	assert(encoded != NULL);

	callsign.empty();

	uint64_t enc = (uint64_t(encoded[0U]) << 40) +
		       (uint64_t(encoded[1U]) << 32) +
		       (uint64_t(encoded[2U]) << 24) +
		       (uint64_t(encoded[3U]) << 16) +
		       (uint64_t(encoded[4U]) << 8)  +
		       (uint64_t(encoded[5U]) << 0);

	if (enc >= 262144000000000ULL)	// 40^9
		return;

	while (enc > 0ULL) {
		callsign += " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/."[enc % 40ULL];
		enc /= 40ULL;
	}
}

