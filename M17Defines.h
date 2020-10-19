/*
 *   Copyright (C) 2016,2017,2018,2020 by Jonathan Naylor G4KLX
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

#if !defined(M17DEFINES_H)
#define  M17DEFINES_H

const unsigned int M17_RADIO_SYMBOL_LENGTH = 5U;      // At 24 kHz sample rate

const unsigned int M17_FRAME_LENGTH_BITS    = 384U;
const unsigned int M17_FRAME_LENGTH_BYTES   = M17_FRAME_LENGTH_BITS / 8U;

const unsigned char M17_SYNC_BYTES[] = {0x32U, 0x43U};
const unsigned int  M17_SYNC_LENGTH_BITS  = 16U;
const unsigned int  M17_SYNC_LENGTH_BYTES = M17_SYNC_LENGTH_BITS / 8U;

const unsigned int M17_LICH_LENGTH_BITS  = 240U;
const unsigned int M17_LICH_LENGTH_BYTES = M17_LICH_LENGTH_BITS / 8U;

const unsigned int M17_LICH_FRAGMENT_LENGTH_BITS  = M17_LICH_LENGTH_BITS / 5U;
const unsigned int M17_LICH_FRAGMENT_LENGTH_BYTES = M17_LICH_FRAGMENT_LENGTH_BITS / 8U;

const unsigned int M17_LICH_FRAGMENT_FEC_LENGTH_BITS  = M17_LICH_FRAGMENT_LENGTH_BITS * 2U;
const unsigned int M17_LICH_FRAGMENT_FEC_LENGTH_BYTES = M17_LICH_FRAGMENT_FEC_LENGTH_BITS / 8U;

const unsigned int M17_PAYLOAD_LENGTH_BITS  = 128U;
const unsigned int M17_PAYLOAD_LENGTH_BYTES = M17_PAYLOAD_LENGTH_BITS / 8U;

const unsigned int M17_FN_LENGTH_BITS  = 16U;
const unsigned int M17_FN_LENGTH_BYTES = M17_FN_LENGTH_BITS / 8U;

const unsigned int M17_CRC_LENGTH_BITS  = 16U;
const unsigned int M17_CRC_LENGTH_BYTES = M17_CRC_LENGTH_BITS / 8U;

const unsigned char M17_3200_SILENCE[] = {0x01U, 0x00U, 0x09U, 0x43U, 0x9CU, 0xE4U, 0x21U, 0x08U};
const unsigned char M17_1600_SILENCE[] = {0x01U, 0x00U, 0x04U, 0x00U, 0x25U, 0x75U, 0xDDU, 0xF2U};

#endif
