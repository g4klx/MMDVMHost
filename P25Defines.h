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
const unsigned int P25_HDR_FRAME_LENGTH_BITS  = P25_HDR_FRAME_LENGTH_BYTES * 8U;

const unsigned int P25_LDU_FRAME_LENGTH_BYTES = 216U;
const unsigned int P25_LDU_FRAME_LENGTH_BITS  = P25_LDU_FRAME_LENGTH_BYTES * 8U;

const unsigned int P25_TERM_FRAME_LENGTH_BYTES = 18U;
const unsigned int P25_TERM_FRAME_LENGTH_BITS  = P25_TERM_FRAME_LENGTH_BYTES * 8U;

const unsigned int P25_TERMLC_FRAME_LENGTH_BYTES = 54U;
const unsigned int P25_TERMLC_FRAME_LENGTH_BITS  = P25_TERMLC_FRAME_LENGTH_BYTES * 8U;

const unsigned int P25_SYNC_LENGTH_BYTES = 6U;

const unsigned int P25_NID_LENGTH_BYTES  = 8U;
const unsigned int P25_NID_LENGTH_BITS   = P25_NID_LENGTH_BYTES * 8U;

const unsigned char P25_SYNC_BYTES[]      = {0x55U, 0x75U, 0xF5U, 0xFFU, 0x77U, 0xFFU};
const unsigned char P25_SYNC_BYTES_LENGTH = 6U;

const unsigned char P25_LCF_GROUP   = 0x00U;
const unsigned char P25_LCF_PRIVATE = 0x03U;

const unsigned int  P25_SS0_START    = 70U;
const unsigned int  P25_SS1_START    = 71U;
const unsigned int  P25_SS_INCREMENT = 72U;

const unsigned char P25_DUID_HEADER  = 0x00U;
const unsigned char P25_DUID_TERM    = 0x03U;
const unsigned char P25_DUID_LDU1    = 0x05U;
const unsigned char P25_DUID_LDU2    = 0x0AU;
const unsigned char P25_DUID_PDU     = 0x0CU;
const unsigned char P25_DUID_TERM_LC = 0x0FU;

const unsigned char P25_RAW_HDR[]  = {
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x08U, 0xDCU, 0x60U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x18U, 0xEBU, 0x5EU, 0x09U, 0x54U,
	0x9FU, 0x0AU, 0x88U, 0xD5U, 0x03U, 0x67U, 0xCDU, 0x1FU, 0xF1U, 0xD4U, 0x1EU, 0x42U, 0x1EU, 0xA2U, 0x35U, 0x8BU,
	0xFCU, 0xC4U, 0xA9U, 0x78U, 0xDCU, 0x61U, 0x2AU, 0x59U, 0x46U, 0xE3U, 0x0AU, 0x4FU, 0x80U, 0xC7U, 0x54U, 0xC7U,
	0x51U};

const unsigned char P25_RAW_LDU2[] = {
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x83U,
	0x80U, 0x00U, 0x00U, 0x00U, 0xAEU, 0x8BU, 0x48U, 0xB6U, 0x49U, 0x9AU, 0xBDU, 0x3CU, 0x7FU, 0x58U};

#endif
