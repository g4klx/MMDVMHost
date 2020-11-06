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

#if !defined(AX25Defines_H)
#define	AX25Defines_H

const unsigned int AX25_CALLSIGN_TEXT_LENGTH = 6U;
const unsigned int AX25_SSID_LENGTH          = 1U;
const unsigned int AX25_CALLSIGN_LENGTH      = 7U;

const unsigned int AX25_MAX_DIGIPEATERS = 6U;

const unsigned char AX25_PID_NOL3       = 0xF0U;

const unsigned int  AX25_MAX_FRAME_LENGTH_BYTES = 330U;     // Callsign (7) + Callsign (7) + 8 Digipeaters (56) +
															// Control (1) + PID (1) + Data (256) + Checksum (2)
const unsigned char AX25_KISS_DATA = 0x00U;

const unsigned char AX25_FEND  = 0xC0U;
const unsigned char AX25_FESC  = 0xDBU;
const unsigned char AX25_TFEND = 0xDCU;
const unsigned char AX25_TFESC = 0xDDU;

#endif
