/*
 *   Copyright (C) 2015,2016,2018,2019,2021,2025 by Jonathan Naylor G4KLX
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

#include "Defines.h"

const unsigned int DSTAR_HEADER_LENGTH_BYTES = 41U;
const unsigned int DSTAR_FRAME_LENGTH_BYTES  = 12U;

const unsigned char DSTAR_END_PATTERN_BYTES[] = { TAG_EOT, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xC8, 0x7A };
const unsigned int  DSTAR_END_PATTERN_LENGTH_BYTES = 6U;

const unsigned char DSTAR_NULL_AMBE_DATA_BYTES[] = { 0x9E, 0x8D, 0x32, 0x88, 0x26, 0x1A, 0x3F, 0x61, 0xE8 };
// DSTAR_NULL_AMBE_DATA_BYTES_SCRAMBLED is DSTAR_NULL_AMBE_DATA_BYTES XORed with DSTAR_SCRAMBLER_BYTES.
const unsigned char DSTAR_NULL_AMBE_DATA_BYTES_SCRAMBLED[] = { 0xEEU, 0xC2U, 0xA1U, 0xC8U, 0x42U, 0x6EU, 0x52U, 0x51U, 0xC3U };

const unsigned char DSTAR_NULL_SLOW_SYNC_BYTES[] = { 0x55, 0x2D, 0x16 };
// Note that these are already scrambled, 0x66 0x66 0x66 otherwise
const unsigned char DSTAR_NULL_SLOW_DATA_BYTES[] = { 0x16, 0x29, 0xF5 };

const unsigned char DSTAR_NULL_FRAME_SYNC_BYTES[] = { TAG_DATA, 0x9E, 0x8D, 0x32, 0x88, 0x26, 0x1A, 0x3F, 0x61, 0xE8, 0x55, 0x2D, 0x16 };
const unsigned char DSTAR_NULL_FRAME_DATA_BYTES[] = { TAG_DATA, 0x9E, 0x8D, 0x32, 0x88, 0x26, 0x1A, 0x3F, 0x61, 0xE8, 0x16, 0x29, 0xF5 };

const unsigned int  DSTAR_VOICE_FRAME_LENGTH_BYTES = 9U;
const unsigned int  DSTAR_DATA_FRAME_LENGTH_BYTES  = 3U;

const unsigned int  DSTAR_LONG_CALLSIGN_LENGTH  = 8U;
const unsigned int  DSTAR_SHORT_CALLSIGN_LENGTH = 4U;

const unsigned char DSTAR_SLOW_DATA_TYPE_MASK           = 0xF0U;
const unsigned char DSTAR_SLOW_DATA_TYPE_GPSDATA        = 0x30U;
const unsigned char DSTAR_SLOW_DATA_TYPE_TEXT           = 0x40U;
const unsigned char DSTAR_SLOW_DATA_TYPE_HEADER         = 0x50U;
const unsigned char DSTAR_SLOW_DATA_TYPE_FASTDATA_END   = 0x80U;
const unsigned char DSTAR_SLOW_DATA_TYPE_FASTDATA_BEGIN = 0x90U;
const unsigned char DSTAR_SLOW_DATA_TYPE_SQUELCH        = 0xC0U;
const unsigned char DSTAR_SLOW_DATA_LENGTH_MASK         = 0x0FU;

// Data Frames are always scrambled using the first three bytes of
// DSTAR_SCRAMBLER_BYTES, and Voice Frames are scrambled with all nine
// bytes when Fast Data is in use
const unsigned char DSTAR_SCRAMBLER_BYTES[] = { 0x70U, 0x4FU, 0x93U, 0x40U, 0x64U, 0x74U, 0x6DU, 0x30U, 0x2BU };

const unsigned char DSTAR_DATA_MASK           = 0x80U;
const unsigned char DSTAR_REPEATER_MASK       = 0x40U;
const unsigned char DSTAR_INTERRUPTED_MASK    = 0x20U;
const unsigned char DSTAR_CONTROL_SIGNAL_MASK = 0x10U;
const unsigned char DSTAR_URGENT_MASK         = 0x08U;
const unsigned char DSTAR_REPEATER_CONTROL    = 0x07U;
const unsigned char DSTAR_AUTO_REPLY          = 0x06U;
const unsigned char DSTAR_RESEND_REQUESTED    = 0x04U;
const unsigned char FLAG            = 0x03U;
const unsigned char DSTAR_NO_RESPONSE         = 0x02U;
const unsigned char DSTAR_RELAY_UNAVAILABLE   = 0x01U;

const unsigned char DSTAR_SYNC_BYTES[] = {0x55U, 0x2DU, 0x16U};

const unsigned char DSTAR_DTMF_MASK[] = { 0x82U, 0x08U, 0x20U, 0x82U, 0x00U, 0x00U, 0x82U, 0x00U, 0x00U };
const unsigned char DSTAR_DTMF_SIG[]  = { 0x82U, 0x08U, 0x20U, 0x82U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U };

const unsigned int DSTAR_FRAME_TIME = 20U;

enum class LINK_STATUS {
	NONE,
	PENDING_IRCDDB,
	LINKING_LOOPBACK,
	LINKING_DEXTRA,
	LINKING_DPLUS,
	LINKING_DCS,
	LINKING_CCS,
	LINKED_LOOPBACK,
	LINKED_DEXTRA,
	LINKED_DPLUS,
	LINKED_DCS,
	LINKED_CCS
};

#endif
