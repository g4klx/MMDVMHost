/*
 *   Copyright (C) 2015,2016,2021 by Jonathan Naylor G4KLX
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

#if !defined(DMRDefines_H)
#define	DMRDefines_H

#include "Defines.h"		// For TAG_DATA

const unsigned int DMR_FRAME_LENGTH_BITS  = 264U;
const unsigned int DMR_FRAME_LENGTH_BYTES = 33U;

const unsigned int DMR_SYNC_LENGTH_BITS  = 48U;
const unsigned int DMR_SYNC_LENGTH_BYTES = 6U;

const unsigned int DMR_EMB_LENGTH_BITS  = 8U;
const unsigned int DMR_EMB_LENGTH_BYTES = 1U;

const unsigned int DMR_SLOT_TYPE_LENGTH_BITS  = 8U;
const unsigned int DMR_SLOT_TYPE_LENGTH_BYTES = 1U;

const unsigned int DMR_EMBEDDED_SIGNALLING_LENGTH_BITS  = 32U;
const unsigned int DMR_EMBEDDED_SIGNALLING_LENGTH_BYTES = 4U;

const unsigned int DMR_AMBE_LENGTH_BITS  = 108U * 2U;
const unsigned int DMR_AMBE_LENGTH_BYTES = 27U;

const unsigned char BS_SOURCED_AUDIO_SYNC[]   = {0x07U, 0x55U, 0xFDU, 0x7DU, 0xF7U, 0x5FU, 0x70U};
const unsigned char BS_SOURCED_DATA_SYNC[]    = {0x0DU, 0xFFU, 0x57U, 0xD7U, 0x5DU, 0xF5U, 0xD0U};

const unsigned char MS_SOURCED_AUDIO_SYNC[]   = {0x07U, 0xF7U, 0xD5U, 0xDDU, 0x57U, 0xDFU, 0xD0U};
const unsigned char MS_SOURCED_DATA_SYNC[]    = {0x0DU, 0x5DU, 0x7FU, 0x77U, 0xFDU, 0x75U, 0x70U};

const unsigned char DIRECT_SLOT1_AUDIO_SYNC[] = {0x05U, 0xD5U, 0x77U, 0xF7U, 0x75U, 0x7FU, 0xF0U};
const unsigned char DIRECT_SLOT1_DATA_SYNC[]  = {0x0FU, 0x7FU, 0xDDU, 0x5DU, 0xDFU, 0xD5U, 0x50U};

const unsigned char DIRECT_SLOT2_AUDIO_SYNC[] = {0x07U, 0xDFU, 0xFDU, 0x5FU, 0x55U, 0xD5U, 0xF0U};
const unsigned char DIRECT_SLOT2_DATA_SYNC[]  = {0x0DU, 0x75U, 0x57U, 0xF5U, 0xFFU, 0x7FU, 0x50U};

const unsigned char SYNC_MASK[]               = {0x0FU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xF0U};

// The PR FILL and Data Sync pattern.
const unsigned char DMR_IDLE_DATA[] = {TAG_DATA, 0x00U,
							0x53U, 0xC2U, 0x5EU, 0xABU, 0xA8U, 0x67U, 0x1DU, 0xC7U, 0x38U, 0x3BU, 0xD9U,
							0x36U, 0x00U, 0x0DU, 0xFFU, 0x57U, 0xD7U, 0x5DU, 0xF5U, 0xD0U, 0x03U, 0xF6U,
							0xE4U, 0x65U, 0x17U, 0x1BU, 0x48U, 0xCAU, 0x6DU, 0x4FU, 0xC6U, 0x10U, 0xB4U};

// A silence frame only
const unsigned char DMR_SILENCE_DATA[] = {TAG_DATA, 0x00U,
							0xB9U, 0xE8U, 0x81U, 0x52U, 0x61U, 0x73U, 0x00U, 0x2AU, 0x6BU, 0xB9U, 0xE8U,
							0x81U, 0x52U, 0x60U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U, 0x73U, 0x00U,
							0x2AU, 0x6BU, 0xB9U, 0xE8U, 0x81U, 0x52U, 0x61U, 0x73U, 0x00U, 0x2AU, 0x6BU};

const unsigned char PAYLOAD_LEFT_MASK[]       = {0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xF0U};
const unsigned char PAYLOAD_RIGHT_MASK[]      = {0x0FU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};

const unsigned char VOICE_LC_HEADER_CRC_MASK[]    = {0x96U, 0x96U, 0x96U};
const unsigned char TERMINATOR_WITH_LC_CRC_MASK[] = {0x99U, 0x99U, 0x99U};
const unsigned char PI_HEADER_CRC_MASK[]          = {0x69U, 0x69U};
const unsigned char DATA_HEADER_CRC_MASK[]        = {0xCCU, 0xCCU};
const unsigned char CSBK_CRC_MASK[]               = {0xA5U, 0xA5U};

const unsigned int DMR_SLOT_TIME = 60U;
const unsigned int AMBE_PER_SLOT = 3U;

const unsigned char DT_MASK               = 0x0FU;
const unsigned char DT_VOICE_PI_HEADER    = 0x00U;
const unsigned char DT_VOICE_LC_HEADER    = 0x01U;
const unsigned char DT_TERMINATOR_WITH_LC = 0x02U;
const unsigned char DT_CSBK               = 0x03U;
const unsigned char DT_DATA_HEADER        = 0x06U;
const unsigned char DT_RATE_12_DATA       = 0x07U;
const unsigned char DT_RATE_34_DATA       = 0x08U;
const unsigned char DT_IDLE               = 0x09U;
const unsigned char DT_RATE_1_DATA        = 0x0AU;

// Dummy values
const unsigned char DT_VOICE_SYNC  = 0xF0U;
const unsigned char DT_VOICE       = 0xF1U;

const unsigned char DMR_IDLE_RX    = 0x80U;
const unsigned char DMR_SYNC_DATA  = 0x40U;
const unsigned char DMR_SYNC_AUDIO = 0x20U;

const unsigned char DMR_SLOT1      = 0x00U;
const unsigned char DMR_SLOT2      = 0x80U;

const unsigned char DPF_UDT              = 0x00U;
const unsigned char DPF_RESPONSE         = 0x01U;
const unsigned char DPF_UNCONFIRMED_DATA = 0x02U;
const unsigned char DPF_CONFIRMED_DATA   = 0x03U;
const unsigned char DPF_DEFINED_SHORT    = 0x0DU;
const unsigned char DPF_DEFINED_RAW      = 0x0EU;
const unsigned char DPF_PROPRIETARY      = 0x0FU;

const unsigned char FID_ETSI = 0U;
const unsigned char FID_DMRA = 16U;

enum FLCO {
	FLCO_GROUP               = 0,
	FLCO_USER_USER           = 3,
	FLCO_TALKER_ALIAS_HEADER = 4,
	FLCO_TALKER_ALIAS_BLOCK1 = 5,
	FLCO_TALKER_ALIAS_BLOCK2 = 6,
	FLCO_TALKER_ALIAS_BLOCK3 = 7,
	FLCO_GPS_INFO            = 8
};

#endif
