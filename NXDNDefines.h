/*
 *   Copyright (C) 2016,2017,2018 by Jonathan Naylor G4KLX
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

#if !defined(NXDNDEFINES_H)
#define  NXDNDEFINES_H

const unsigned int NXDN_RADIO_SYMBOL_LENGTH = 5U;      // At 24 kHz sample rate

const unsigned int NXDN_FRAME_LENGTH_BITS    = 384U;
const unsigned int NXDN_FRAME_LENGTH_BYTES   = NXDN_FRAME_LENGTH_BITS / 8U;
const unsigned int NXDN_FRAME_LENGTH_SYMBOLS = NXDN_FRAME_LENGTH_BITS / 2U;

const unsigned int NXDN_FSW_LENGTH_BITS    = 20U;
const unsigned int NXDN_FSW_LENGTH_SYMBOLS = NXDN_FSW_LENGTH_BITS / 2U;
const unsigned int NXDN_FSW_LENGTH_SAMPLES = NXDN_FSW_LENGTH_SYMBOLS * NXDN_RADIO_SYMBOL_LENGTH;

const unsigned char NXDN_FSW_BYTES[]      = {0xCDU, 0xF5U, 0x90U};
const unsigned char NXDN_FSW_BYTES_MASK[] = {0xFFU, 0xFFU, 0xF0U};
const unsigned int  NXDN_FSW_BYTES_LENGTH = 3U;

const unsigned int NXDN_LICH_LENGTH_BITS = 16U;

const unsigned int NXDN_SACCH_LENGTH_BITS  = 60U;
const unsigned int NXDN_FACCH1_LENGTH_BITS = 144U;
const unsigned int NXDN_FACCH2_LENGTH_BITS = 348U;

const unsigned int NXDN_FSW_LICH_SACCH_LENGTH_BITS  = NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS + NXDN_SACCH_LENGTH_BITS;
const unsigned int NXDN_FSW_LICH_SACCH_LENGTH_BYTES = NXDN_FSW_LICH_SACCH_LENGTH_BITS / 8U;

const unsigned char NXDN_LICH_RFCT_RCCH   = 0U;
const unsigned char NXDN_LICH_RFCT_RTCH   = 1U;
const unsigned char NXDN_LICH_RFCT_RDCH   = 2U;
const unsigned char NXDN_LICH_RFCT_RTCH_C = 3U;

const unsigned char NXDN_LICH_USC_SACCH_NS      = 0U;
const unsigned char NXDN_LICH_USC_UDCH          = 1U;
const unsigned char NXDN_LICH_USC_SACCH_SS      = 2U;
const unsigned char NXDN_LICH_USC_SACCH_SS_IDLE = 3U;

const unsigned char NXDN_LICH_STEAL_NONE     = 3U;
const unsigned char NXDN_LICH_STEAL_FACCH1_2 = 2U;
const unsigned char NXDN_LICH_STEAL_FACCH1_1 = 1U;
const unsigned char NXDN_LICH_STEAL_FACCH    = 0U;

const unsigned char NXDN_LICH_DIRECTION_INBOUND  = 0U;
const unsigned char NXDN_LICH_DIRECTION_OUTBOUND = 1U;

const unsigned char NXDN_SR_SINGLE = 0U;
const unsigned char NXDN_SR_4_4    = 0U;
const unsigned char NXDN_SR_3_4    = 1U;
const unsigned char NXDN_SR_2_4    = 2U;
const unsigned char NXDN_SR_1_4    = 3U;

#endif
