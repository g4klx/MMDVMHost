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

#if !defined(M17Utils_H)
#define  M17Utils_H

#include <string>

class CM17Utils {
public:
	CM17Utils();
	~CM17Utils();

	static void encodeCallsign(const std::string& callsign, unsigned char* encoded);
	static void decodeCallsign(const unsigned char* encoded, std::string& callsign);

	static void splitFragmentLICH(const unsigned char* data, unsigned int& frag1, unsigned int& frag2, unsigned int& frag3, unsigned int& frag4);
	static void splitFragmentLICHFEC(const unsigned char* data, unsigned int& frag1, unsigned int& frag2, unsigned int& frag3, unsigned int& frag4);

	static void combineFragmentLICH(unsigned int frag1, unsigned int frag2, unsigned int frag3, unsigned int frag4, unsigned char* data);
	static void combineFragmentLICHFEC(unsigned int frag1, unsigned int frag2, unsigned int frag3, unsigned int frag4, unsigned char* data);

private:
};

#endif
