/*
 *   Copyright (C) 2009-2016 by Jonathan Naylor G4KLX
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

#include "YSFConvolution.h"
#include "Golay24128.h"
#include "YSFFICH.h"
#include "CRC.h"

#include <cstdio>
#include <cassert>


const unsigned char BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

const unsigned int INTERLEAVE_TABLE_RX[] = {
   0U, 40U,  80U, 120U, 160U,
   2U, 42U,  82U, 122U, 162U,
   4U, 44U,  84U, 124U, 164U,
   6U, 46U,  86U, 126U, 166U,
   8U, 48U,  88U, 128U, 168U,
  10U, 50U,  90U, 130U, 170U,
  12U, 52U,  92U, 132U, 172U,
  14U, 54U,  94U, 134U, 174U,
  16U, 56U,  96U, 136U, 176U,
  18U, 58U,  98U, 138U, 178U,
  20U, 60U, 100U, 140U, 180U,
  22U, 62U, 102U, 142U, 182U,
  24U, 64U, 104U, 144U, 184U,
  26U, 66U, 106U, 146U, 186U,
  28U, 68U, 108U, 148U, 188U,
  30U, 70U, 110U, 150U, 190U,
  32U, 72U, 112U, 152U, 192U,
  34U, 74U, 114U, 154U, 194U,
  36U, 76U, 116U, 156U, 196U,
  38U, 78U, 118U, 158U, 198U};

CYSFFICH::CYSFFICH()
{
}

CYSFFICH::~CYSFFICH()
{
}

bool CYSFFICH::decode(const unsigned char* data, unsigned char* fich) const
{
	assert(data != NULL);
	assert(fich != NULL);

	CYSFConvolution viterbi;
	viterbi.start();

	// Deinterleave the FICH and send bits to the Viterbi decoder
	for (unsigned int i = 0U; i < 100U; i++) {
		unsigned int n = INTERLEAVE_TABLE_RX[i];
		unsigned int s0 = READ_BIT1(data, n) ? 1U : 0U;

		n++;
		unsigned int s1 = READ_BIT1(data, n) ? 1U : 0U;

		viterbi.decode(s0, s1);
  }

	unsigned char output[13U];
	viterbi.chainback(output);

	unsigned int b0 = CGolay24128::decode24128(output + 0U);
	unsigned int b1 = CGolay24128::decode24128(output + 3U);
	unsigned int b2 = CGolay24128::decode24128(output + 6U);
	unsigned int b3 = CGolay24128::decode24128(output + 9U);

	fich[0U] = (b0 >> 16) & 0xFFU;
	fich[1U] = ((b0 >> 8) & 0xF0U) | ((b1 >> 20) & 0x0FU);
	fich[2U] = (b1 >> 12) & 0xFFU;
	fich[3U] = (b2 >> 16) & 0xFFU;
	fich[4U] = ((b2 >> 8) & 0xF0U) | ((b3 >> 20) & 0x0FU);
	fich[5U] = (b3 >> 12) & 0xFFU;

	return CCRC::crcFICH(fich);
}
