/*
 *   Copyright (C) 2020,2021,2025 by Jonathan Naylor G4KLX
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

#include "M17Convolution.h"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <cstdlib>

const unsigned int PUNCTURE_LIST_LINK_SETUP_COUNT = 60U;

const unsigned int PUNCTURE_LIST_LINK_SETUP[] = {
	  2U,   6U,  10U,  14U,  18U,  22U,  26U,  30U,  34U,  38U,  42U,  46U,  50U,  54U,  58U,  63U,  67U,  71U,  75U,  79U,  83U,
	 87U,  91U,  95U,  99U, 103U, 107U, 111U, 115U, 119U, 124U, 128U, 132U, 136U, 140U, 144U, 148U, 152U, 156U, 160U, 164U, 168U,
	172U, 176U, 180U, 185U, 189U, 193U, 197U, 201U, 205U, 209U, 213U, 217U, 221U, 225U, 229U, 233U, 237U, 241U, 246U, 250U, 254U,
	258U, 262U, 266U, 270U, 274U, 278U, 282U, 286U, 290U, 294U, 298U, 302U, 307U, 311U, 315U, 319U, 323U, 327U, 331U, 335U, 339U,
	343U, 347U, 351U, 355U, 359U, 363U, 368U, 372U, 376U, 380U, 384U, 388U, 392U, 396U, 400U, 404U, 408U, 412U, 416U, 420U, 424U,
	429U, 433U, 437U, 441U, 445U, 449U, 453U, 457U, 461U, 465U, 469U, 473U, 477U, 481U, 485U};

const unsigned int PUNCTURE_LIST_DATA_COUNT = 12U;

const unsigned int PUNCTURE_LIST_DATA[] = {
	 11U,  23U,  35U,  47U,  59U,  71U,  83U,  95U, 107U, 119U, 131U, 143U, 155U, 167U, 179U, 191U, 203U, 215U, 227U, 239U, 251U,
	263U, 275U, 287U};

const unsigned char BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

const uint8_t BRANCH_TABLE1[] = {0U, 0U, 0U, 0U, 2U, 2U, 2U, 2U};
const uint8_t BRANCH_TABLE2[] = {0U, 2U, 2U, 0U, 0U, 2U, 2U, 0U};

const unsigned int NUM_OF_STATES_D2 = 8U;
const unsigned int NUM_OF_STATES = 16U;
const uint32_t     M = 4U;
const unsigned int K = 5U;

CM17Convolution::CM17Convolution() :
m_metrics1(nullptr),
m_metrics2(nullptr),
m_oldMetrics(nullptr),
m_newMetrics(nullptr),
m_decisions(nullptr),
m_dp(nullptr)
{
	m_metrics1  = new uint16_t[20U];
	m_metrics2  = new uint16_t[20U];
	m_decisions = new uint64_t[300U];
}

CM17Convolution::~CM17Convolution()
{
	delete[] m_metrics1;
	delete[] m_metrics2;
	delete[] m_decisions;
}

void CM17Convolution::encodeLinkSetup(const unsigned char* in, unsigned char* out) const
{
	assert(in != nullptr);
	assert(out != nullptr);

	unsigned char temp1[31U];
	::memset(temp1, 0x00U, 31U);
	::memcpy(temp1, in, 30U);

	unsigned char temp2[61U];
	encode(temp1, temp2, 244U);

	unsigned int n = 0U;
	unsigned int index = 0U;
	for (unsigned int i = 0U; i < 488U; i++) {
		if (i != PUNCTURE_LIST_LINK_SETUP[index]) {
			bool b = READ_BIT1(temp2, i);
			WRITE_BIT1(out, n, b);
			n++;
		} else {
			index++;
		}
	}
}

void CM17Convolution::encodeData(const unsigned char* in, unsigned char* out) const
{
	assert(in != nullptr);
	assert(out != nullptr);

	unsigned char temp1[19U];
	::memset(temp1, 0x00U, 19U);
	::memcpy(temp1, in, 18U);

	unsigned char temp2[37U];
	encode(temp1, temp2, 148U);

	unsigned int n = 0U;
	unsigned int index = 0U;
	for (unsigned int i = 0U; i < 296U; i++) {
		if (i != PUNCTURE_LIST_DATA[index]) {
			bool b = READ_BIT1(temp2, i);
			WRITE_BIT1(out, n, b);
			n++;
		} else {
			index++;
		}
	}
}

unsigned int CM17Convolution::decodeLinkSetup(const unsigned char* in, unsigned char* out)
{
	assert(in != nullptr);
	assert(out != nullptr);

	uint8_t temp[500U];
	::memset(temp, 0x00U, 500U);

	unsigned int n = 0U;
	unsigned int index = 0U;
	for (unsigned int i = 0U; i < 368U; i++) {
		if (n == PUNCTURE_LIST_LINK_SETUP[index]) {
			temp[n++] = 1U;
			index++;
		}

		bool b = READ_BIT1(in, i);
		temp[n++] = b ? 2U : 0U;
	}

	start();

	n = 0U;
	for (unsigned int i = 0U; i < 244U; i++) {
		uint8_t s0 = temp[n++];
		uint8_t s1 = temp[n++];

		decode(s0, s1);
	}

	return chainback(out, 240U) - PUNCTURE_LIST_LINK_SETUP_COUNT;
}

unsigned int CM17Convolution::decodeData(const unsigned char* in, unsigned char* out)
{
	assert(in != nullptr);
	assert(out != nullptr);

	uint8_t temp[300U];
	::memset(temp, 0x00U, 300U);

	unsigned int n = 0U;
	unsigned int index = 0U;
	for (unsigned int i = 0U; i < 272U; i++) {
		if (n == PUNCTURE_LIST_DATA[index]) {
			temp[n++] = 1U;
			index++;
		}

		bool b = READ_BIT1(in, i);
		temp[n++] = b ? 2U : 0U;
	}

	start();

	n = 0U;
	for (unsigned int i = 0U; i < 148U; i++) {
		uint8_t s0 = temp[n++];
		uint8_t s1 = temp[n++];

		decode(s0, s1);
	}

	return chainback(out, 144U) - PUNCTURE_LIST_DATA_COUNT;
}

void CM17Convolution::start()
{
	::memset(m_metrics1, 0x00U, NUM_OF_STATES * sizeof(uint16_t));
	::memset(m_metrics2, 0x00U, NUM_OF_STATES * sizeof(uint16_t));

	m_oldMetrics = m_metrics1;
	m_newMetrics = m_metrics2;
	m_dp = m_decisions;
}

void CM17Convolution::decode(uint8_t s0, uint8_t s1)
{
  *m_dp = 0U;

  for (uint8_t i = 0U; i < NUM_OF_STATES_D2; i++) {
    uint8_t j = i * 2U;

    uint16_t metric = std::abs(BRANCH_TABLE1[i] - s0) + std::abs(BRANCH_TABLE2[i] - s1);

    uint16_t m0 = m_oldMetrics[i] + metric;
    uint16_t m1 = m_oldMetrics[i + NUM_OF_STATES_D2] + (M - metric);
    uint8_t decision0 = (m0 >= m1) ? 1U : 0U;
    m_newMetrics[j + 0U] = decision0 != 0U ? m1 : m0;

    m0 = m_oldMetrics[i] + (M - metric);
    m1 = m_oldMetrics[i + NUM_OF_STATES_D2] + metric;
    uint8_t decision1 = (m0 >= m1) ? 1U : 0U;
    m_newMetrics[j + 1U] = decision1 != 0U ? m1 : m0;

    *m_dp |= (uint64_t(decision1) << (j + 1U)) | (uint64_t(decision0) << (j + 0U));
  }

  ++m_dp;

  assert((m_dp - m_decisions) <= 300);

  uint16_t* tmp = m_oldMetrics;
  m_oldMetrics = m_newMetrics;
  m_newMetrics = tmp;
}

unsigned int CM17Convolution::chainback(unsigned char* out, unsigned int nBits)
{
	assert(out != nullptr);

	uint32_t state = 0U;

	while (nBits-- > 0) {
		--m_dp;

		uint32_t  i = state >> (9 - K);
		uint8_t bit = uint8_t(*m_dp >> i) & 1;
		state = (bit << 7) | (state >> 1);

		WRITE_BIT1(out, nBits, bit != 0U);
	}

	unsigned int minCost = m_oldMetrics[0];

	for (unsigned int i = 0U; i < NUM_OF_STATES; i++) {
		if (m_oldMetrics[i] < minCost)
			minCost = m_oldMetrics[i];
	}

	return minCost / (M >> 1);
}

void CM17Convolution::encode(const unsigned char* in, unsigned char* out, unsigned int nBits) const
{
	assert(in != nullptr);
	assert(out != nullptr);
	assert(nBits > 0U);

	uint8_t d1 = 0U, d2 = 0U, d3 = 0U, d4 = 0U;
	uint32_t k = 0U;
	for (unsigned int i = 0U; i < nBits; i++) {
		uint8_t d = READ_BIT1(in, i) ? 1U : 0U;

		uint8_t g1 = (d + d3 + d4) & 1;
		uint8_t g2 = (d + d1 + d2 + d4) & 1;

		d4 = d3;
		d3 = d2;
		d2 = d1;
		d1 = d;

		WRITE_BIT1(out, k, g1 != 0U);
		k++;

		WRITE_BIT1(out, k, g2 != 0U);
		k++;
	}
}
