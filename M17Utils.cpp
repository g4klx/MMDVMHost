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

void CM17Utils::encodeCallsign(const std::string& callsign, unsigned char* encoded)
{
	assert(encoded != NULL);

    std::string call = callsign;
    call.resize(8U, ' ');

    uint64_t enc = 0ULL;
    for (int i = 7U; i >= 0; i--) {
        char c = call.at(i);

        enc *= 40ULL;

        // If speed is more important than code space, you can replace this with a lookup into a 256 byte array.
        if (c >= 'A' && c <= 'Z') // 1-26
            enc += c - 'A' + 1ULL;
        else if (c >= '0' && c <= '9') // 27-36
            enc += c - '0' + 27ULL;
        else if (c == '-') // 37
            enc += 37ULL;

        // These are just place holders. If other characters make more sense, change these.
        // Be sure to change them in the decode array below too.
        else if (c == '/') // 38
            enc += 38ULL;
        else if (c == '.') // 39
            enc += 39ULL;
        else
            // Invalid character or space, represented by 0, decoded as a space.
            enc += 0ULL;
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

    uint64_t enc = (uint64_t(encoded[5U]) << 40) +
                   (uint64_t(encoded[4U]) << 32) +
                   (uint64_t(encoded[3U]) << 24) +
                   (uint64_t(encoded[2U]) << 16) +
                   (uint64_t(encoded[1U]) << 8)  +
                   (uint64_t(encoded[0U]) << 0);

    if (enc >= 262144000000000ULL) // 40^9
        return;

    while (enc > 0ULL) {
        callsign += " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/."[enc % 40ULL];
        enc /= 40ULL;
    }
}
