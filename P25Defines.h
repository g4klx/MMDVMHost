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

#if !defined(P25DEFINES_H)
#define  P25DEFINES_H

const unsigned int P25_HDR_FRAME_LENGTH_BYTES = 99U;

const unsigned int P25_LDU_FRAME_LENGTH_BYTES = 216U;

const unsigned int P25_SYNC_LENGTH_BYTES = 6U;
const unsigned int P25_NID_LENGTH_BYTES  = 8U;

const unsigned char P25_SYNC_BYTES[]      = {0x55U, 0x75U, 0xF5U, 0xFFU, 0x77U, 0xFFU};
const unsigned char P25_SYNC_BYTES_LENGTH = 6U;

#endif

