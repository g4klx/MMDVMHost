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

#if !defined(Defines_H)
#define	Defines_H

const unsigned char MODE_IDLE  = 0U;
const unsigned char MODE_DSTAR = 1U;
const unsigned char MODE_DMR   = 2U;
const unsigned char MODE_YSF   = 3U;

const unsigned char TAG_HEADER = 0x00U;
const unsigned char TAG_DATA   = 0x01U;
const unsigned char TAG_LOST   = 0x02U;
const unsigned char TAG_EOT    = 0x03U;

enum RPT_STATE {
	RS_LISTENING,
	RS_LATE_ENTRY,
	RS_RELAYING_RF_AUDIO,
	RS_RELAYING_NETWORK_AUDIO,
	RS_RELAYING_RF_DATA,
	RS_RELAYING_NETWORK_DATA
};

#endif
