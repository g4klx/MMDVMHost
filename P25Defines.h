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
const unsigned int P25_SYNC_LENGTH_BITS  = P25_SYNC_LENGTH_BYTES * 8U;

const unsigned int P25_NID_LENGTH_BYTES  = 8U;
const unsigned int P25_NID_LENGTH_BITS   = P25_NID_LENGTH_BYTES * 8U;

const unsigned char P25_SYNC_BYTES[]      = {0x55U, 0x75U, 0xF5U, 0xFFU, 0x77U, 0xFFU};
const unsigned char P25_SYNC_BYTES_LENGTH = 6U;

const unsigned int  P25_MAX_PDU_COUNT = 10U;

const unsigned int  P25_PDU_HEADER_LENGTH_BYTES      = 12U;
const unsigned int  P25_PDU_CONFIRMED_LENGTH_BYTES   = 18U;
const unsigned int  P25_PDU_UNCONFIRMED_LENGTH_BYTES = 12U;

const unsigned int  P25_PDU_FEC_LENGTH_BYTES         = 24U;
const unsigned int  P25_PDU_FEC_LENGTH_BITS          = P25_PDU_FEC_LENGTH_BYTES * 8U;

const unsigned int  P25_MI_LENGTH_BYTES = 9U;

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

const unsigned char P25_NULL_IMBE[] = {0x04U, 0x0CU, 0xFDU, 0x7BU, 0xFBU, 0x7DU, 0xF2U, 0x7BU, 0x3DU, 0x9EU, 0x45U};

#endif
