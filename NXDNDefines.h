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

const unsigned int NXDN_FRAME_LENGTH_BYTES  = 48U;

const unsigned int NXDN_FSW_LENGTH_BITS    = 20U;
const unsigned int NXDN_FSW_LENGTH_SYMBOLS = NXDN_FSW_LENGTH_BITS / 2U;
const unsigned int NXDN_FSW_LENGTH_SAMPLES = NXDN_FSW_LENGTH_SYMBOLS * NXDN_RADIO_SYMBOL_LENGTH;

const unsigned char NXDN_FSW_BYTES[]      = {0x0CU, 0xDFU, 0x59U};
const unsigned char NXDN_FSW_BYTES_MASK[] = {0x0FU, 0xFFU, 0xFFU};
const unsigned int  NXDN_FSW_BYTES_LENGTH = 3U;

#endif

