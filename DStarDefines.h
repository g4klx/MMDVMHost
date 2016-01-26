/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
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

#if !defined(DStarDefines_H)
#define	DStarDefines_H

const unsigned int DSTAR_HEADER_LENGTH_BYTES = 41U;
const unsigned int DSTAR_FRAME_LENGTH_BYTES  = 12U;

const unsigned char DSTAR_END_PATTERN_BYTES[] = { 0x55, 0x55, 0x55, 0x55, 0xC8, 0x7A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const unsigned int  DSTAR_END_PATTERN_LENGTH_BYTES = 6U;

const unsigned char DSTAR_NULL_AMBE_DATA_BYTES[] = { 0x9E, 0x8D, 0x32, 0x88, 0x26, 0x1A, 0x3F, 0x61, 0xE8 };

// Note that these are already scrambled, 0x66 0x66 0x66 otherwise
const unsigned char DSTAR_NULL_SLOW_DATA_BYTES[] = { 0x16, 0x29, 0xF5 };

const unsigned char DSTAR_NULL_FRAME_DATA_BYTES[] = { 0x9E, 0x8D, 0x32, 0x88, 0x26, 0x1A, 0x3F, 0x61, 0xE8, 0x16, 0x29, 0xF5 };

const unsigned int  DSTAR_VOICE_FRAME_LENGTH_BYTES = 9U;
const unsigned int  DSTAR_DATA_FRAME_LENGTH_BYTES  = 3U;

const unsigned int  DSTAR_LONG_CALLSIGN_LENGTH  = 8U;
const unsigned int  DSTAR_SHORT_CALLSIGN_LENGTH = 4U;

const unsigned char DSTAR_SLOW_DATA_TYPE_MASK    = 0xF0U;
const unsigned char DSTAR_SLOW_DATA_TYPE_GPSDATA = 0x30U;
const unsigned char DSTAR_SLOW_DATA_TYPE_TEXT    = 0x40U;
const unsigned char DSTAR_SLOW_DATA_TYPE_HEADER  = 0x50U;
const unsigned char DSTAR_SLOW_DATA_TYPE_SQUELCH = 0xC0U;
const unsigned char DSTAR_SLOW_DATA_LENGTH_MASK  = 0x0FU;

const unsigned char DSTAR_SCRAMBLER_BYTEs[] = {0x70U, 0x4FU, 0x93U};

const unsigned char DSTAR_REPEATER_CONTROL = 0x07U;

const unsigned char DSTAR_SYNC_BYTES[] = {0x55U, 0x2DU, 0x16U};

#endif
