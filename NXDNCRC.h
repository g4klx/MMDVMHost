/*
 *   Copyright (C) 2018 by Jonathan Naylor G4KLX
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

#if !defined(NXDNCRC_H)
#define	NXDNCRC_H

#include <cstdint>

class CNXDNCRC
{
public:
	static bool checkCRC6(const unsigned char* in, unsigned int length);
	static void encodeCRC6(unsigned char* in, unsigned int length);

	static bool checkCRC12(const unsigned char* in, unsigned int length);
	static void encodeCRC12(unsigned char* in, unsigned int length);

	static bool checkCRC15(const unsigned char* in, unsigned int length);
	static void encodeCRC15(unsigned char* in, unsigned int length);

private:
	static uint8_t  createCRC6(const unsigned char* in, unsigned int length);
	static uint16_t createCRC12(const unsigned char* in, unsigned int length);
	static uint16_t createCRC15(const unsigned char* in, unsigned int length);
};

#endif
