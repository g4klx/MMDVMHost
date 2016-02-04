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

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned int BUFFER_LENGTH = 200U;

const unsigned char BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

const unsigned char BRANCH_TABLE1[] = {0U, 0U, 0U, 0U, 1U, 1U, 1U, 1U};
const unsigned char BRANCH_TABLE2[] = {0U, 1U, 1U, 0U, 0U, 1U, 1U, 0U};

CYSFConvolution::CYSFConvolution() :
m_metrics1(NULL),
m_metrics2(NULL),
m_oldMetrics(NULL),
m_newMetrics(NULL),
m_decisions(NULL),
m_dp(NULL)
{
	m_metrics1  = new unsigned short[16U];
	m_metrics2  = new unsigned short[16U];
	m_decisions = new unsigned long long[100U];
}

CYSFConvolution::~CYSFConvolution()
{
	delete[] m_metrics1;
	delete[] m_metrics2;
	delete[] m_decisions;
}

const unsigned int NUM_OF_STATES_D2 = 8U;
const unsigned int NUM_OF_STATES = 16U;
const unsigned int M = 3U;
const unsigned int K = 5U;

void CYSFConvolution::start()
{
	::memset(m_metrics1, 0x00U, NUM_OF_STATES * sizeof(unsigned short));
	::memset(m_metrics2, 0x00U, NUM_OF_STATES * sizeof(unsigned short));

	m_oldMetrics = m_metrics1;
	m_newMetrics = m_metrics2;
	m_dp = m_decisions;
}

void CYSFConvolution::decode(unsigned char s0, unsigned char s1)
{
  *m_dp = 0U;

  for (unsigned int i = 0U; i < NUM_OF_STATES_D2; i++) {
    unsigned int j = i * 2U;

    unsigned short metric = (BRANCH_TABLE1[i] ^ s0) + (BRANCH_TABLE2[i] ^ s1);

    unsigned short m0 = m_oldMetrics[i] + metric;
    unsigned short m1 = m_oldMetrics[i + NUM_OF_STATES_D2] + (M - metric);
    unsigned char  decision0 = (m0 >= m1) ? 1U : 0U;
    m_newMetrics[j + 0U] = decision0 != 0U ? m1 : m0;

    m0 = m_oldMetrics[i] + (M - metric);
    m1 = m_oldMetrics[i + NUM_OF_STATES_D2] + metric;
    unsigned char decision1 = (m0 >= m1) ? 1U : 0U;
    m_newMetrics[j + 1U] = decision1 != 0U ? m1 : m0;

    *m_dp |= ((unsigned long long)(decision1) << (j + 1U)) | ((unsigned long long)(decision0) << (j + 0U));
  }

  ++m_dp;

  unsigned short* tmp = m_oldMetrics;
  m_oldMetrics = m_newMetrics;
  m_newMetrics = tmp;
}

void CYSFConvolution::chainback(unsigned char* out)
{
	assert(out != NULL);

	unsigned int state = 0U;

	unsigned char nbits = 96U;
	while (nbits-- > 0) {
		--m_dp;

		unsigned int  i = state >> (9 - K);
		unsigned char bit = (unsigned char)(*m_dp >> i) & 1;
		state = (bit << 7) | (state >> 1);

		WRITE_BIT1(out, nbits, bit != 0U);
	}
}
