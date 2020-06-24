/*
*   Copyright (C) 2018 by Jonathan Naylor G4KLX
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

#include "NXDNUDCH.h"

#include "NXDNConvolution.h"
#include "NXDNDefines.h"
#include "NXDNCRC.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned int INTERLEAVE_TABLE[] = {
	0U,  29U, 58U,  87U, 116U, 145U, 174U, 203U, 232U, 261U, 290U, 319U,
	1U,  30U, 59U,  88U, 117U, 146U, 175U, 204U, 233U, 262U, 291U, 320U,
	2U,  31U, 60U,  89U, 118U, 147U, 176U, 205U, 234U, 263U, 292U, 321U,
	3U,  32U, 61U,  90U, 119U, 148U, 177U, 206U, 235U, 264U, 293U, 322U,
	4U,  33U, 62U,  91U, 120U, 149U, 178U, 207U, 236U, 265U, 294U, 323U,
	5U,  34U, 63U,  92U, 121U, 150U, 179U, 208U, 237U, 266U, 295U, 324U,
	6U,  35U, 64U,  93U, 122U, 151U, 180U, 209U, 238U, 267U, 296U, 325U,
	7U,  36U, 65U,  94U, 123U, 152U, 181U, 210U, 239U, 268U, 297U, 326U,
	8U,  37U, 66U,  95U, 124U, 153U, 182U, 211U, 240U, 269U, 298U, 327U,
	9U,  38U, 67U,  96U, 125U, 154U, 183U, 212U, 241U, 270U, 299U, 328U,
	10U, 39U, 68U,  97U, 126U, 155U, 184U, 213U, 242U, 271U, 300U, 329U,
	11U, 40U, 69U,  98U, 127U, 156U, 185U, 214U, 243U, 272U, 301U, 330U,
	12U, 41U, 70U,  99U, 128U, 157U, 186U, 215U, 244U, 273U, 302U, 331U,
	13U, 42U, 71U, 100U, 129U, 158U, 187U, 216U, 245U, 274U, 303U, 332U,
	14U, 43U, 72U, 101U, 130U, 159U, 188U, 217U, 246U, 275U, 304U, 333U,
	15U, 44U, 73U, 102U, 131U, 160U, 189U, 218U, 247U, 276U, 305U, 334U,
	16U, 45U, 74U, 103U, 132U, 161U, 190U, 219U, 248U, 277U, 306U, 335U,
	17U, 46U, 75U, 104U, 133U, 162U, 191U, 220U, 249U, 278U, 307U, 336U,
	18U, 47U, 76U, 105U, 134U, 163U, 192U, 221U, 250U, 279U, 308U, 337U,
	19U, 48U, 77U, 106U, 135U, 164U, 193U, 222U, 251U, 280U, 309U, 338U,
	20U, 49U, 78U, 107U, 136U, 165U, 194U, 223U, 252U, 281U, 310U, 339U,
	21U, 50U, 79U, 108U, 137U, 166U, 195U, 224U, 253U, 282U, 311U, 340U,
	22U, 51U, 80U, 109U, 138U, 167U, 196U, 225U, 254U, 283U, 312U, 341U,
	23U, 52U, 81U, 110U, 139U, 168U, 197U, 226U, 255U, 284U, 313U, 342U,
	24U, 53U, 82U, 111U, 140U, 169U, 198U, 227U, 256U, 285U, 314U, 343U,
	25U, 54U, 83U, 112U, 141U, 170U, 199U, 228U, 257U, 286U, 315U, 344U,
	26U, 55U, 84U, 113U, 142U, 171U, 200U, 229U, 258U, 287U, 316U, 345U,
	27U, 56U, 85U, 114U, 143U, 172U, 201U, 230U, 259U, 288U, 317U, 346U,
	28U, 57U, 86U, 115U, 144U, 173U, 202U, 231U, 260U, 289U, 318U, 347U };

const unsigned int PUNCTURE_LIST[] = { 3U,  11U,  17U,  25U,  31U,  39U,  45U,  53U,  59U,  67U,
									  73U,  81U,  87U,  95U, 101U, 109U, 115U, 123U, 129U, 137U,
									 143U, 151U, 157U, 165U, 171U, 179U, 185U, 193U, 199U, 207U,
									 213U, 221U, 227U, 235U, 241U, 249U, 255U, 263U, 269U, 277U,
									 283U, 291U, 297U, 305U, 311U, 319U, 325U, 333U, 339U, 347U,
									 353U, 361U, 367U, 375U, 381U, 389U, 395U, 403U };

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CNXDNUDCH::CNXDNUDCH(const CNXDNUDCH& udch) :
m_data(NULL)
{
	m_data = new unsigned char[23U + 3U];
	::memcpy(m_data, udch.m_data, 23U + 3U);
}

CNXDNUDCH::CNXDNUDCH() :
m_data(NULL)
{
	m_data = new unsigned char[23U + 3U];
}

CNXDNUDCH::~CNXDNUDCH()
{
	delete[] m_data;
}

bool CNXDNUDCH::decode(const unsigned char* data)
{
	assert(data != NULL);

	unsigned char temp1[44U];

	for (unsigned int i = 0U; i < NXDN_FACCH2_LENGTH_BITS; i++) {
		unsigned int n = INTERLEAVE_TABLE[i] + NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS;
		bool b = READ_BIT1(data, n);
		WRITE_BIT1(temp1, i, b);
	}

	uint8_t temp2[420U];

	unsigned int n = 0U;
	unsigned int index = 0U;
	for (unsigned int i = 0U; i < NXDN_FACCH2_LENGTH_BITS; i++) {
		if (n == PUNCTURE_LIST[index]) {
			temp2[n++] = 1U;
			index++;
		}

		bool b = READ_BIT1(temp1, i);
		temp2[n++] = b ? 2U : 0U;
	}

	for (unsigned int i = 0U; i < 8U; i++) {
		temp2[n++] = 0U;
	}

	CNXDNConvolution conv;
	conv.start();

	n = 0U;
	for (unsigned int i = 0U; i < 207U; i++) {
		uint8_t s0 = temp2[n++];
		uint8_t s1 = temp2[n++];

		conv.decode(s0, s1);
	}

	conv.chainback(m_data, 203U);

	return CNXDNCRC::checkCRC15(m_data, 184U);
}

void CNXDNUDCH::encode(unsigned char* data) const
{
	assert(data != NULL);

	unsigned char temp1[25U];
	::memset(temp1, 0x00U, 25U);
	::memcpy(temp1, m_data, 23U);

	CNXDNCRC::encodeCRC15(temp1, 184U);

	unsigned char temp2[51U];

	CNXDNConvolution conv;
	conv.encode(temp1, temp2, 203U);

	unsigned char temp3[44U];

	unsigned int n = 0U;
	unsigned int index = 0U;
	for (unsigned int i = 0U; i < 406U; i++) {
		if (i != PUNCTURE_LIST[index]) {
			bool b = READ_BIT1(temp2, i);
			WRITE_BIT1(temp3, n, b);
			n++;
		} else {
			index++;
		}
	}

	for (unsigned int i = 0U; i < NXDN_FACCH2_LENGTH_BITS; i++) {
		unsigned int n = INTERLEAVE_TABLE[i] + NXDN_FSW_LENGTH_BITS + NXDN_LICH_LENGTH_BITS;
		bool b = READ_BIT1(temp3, i);
		WRITE_BIT1(data, n, b);
	}
}

unsigned char CNXDNUDCH::getRAN() const
{
	return m_data[0U] & 0x3FU;
}

void CNXDNUDCH::getData(unsigned char* data) const
{
	assert(data != NULL);

	::memcpy(data, m_data + 1U, 22U);
}

void CNXDNUDCH::getRaw(unsigned char* data) const
{
	assert(data != NULL);

	::memset(data, 0x00U, 25U);
	::memcpy(data, m_data, 23U);
}

void CNXDNUDCH::setRAN(unsigned char ran)
{
	m_data[0U] = ran;
}

void CNXDNUDCH::setData(const unsigned char* data)
{
	assert(data != NULL);

	::memcpy(m_data + 1U, data, 22U);
}

void CNXDNUDCH::setRaw(const unsigned char* data)
{
	assert(data != NULL);

	::memcpy(m_data, data, 25U);
}

CNXDNUDCH& CNXDNUDCH::operator=(const CNXDNUDCH& udch)
{
	if (&udch != this)
		::memcpy(m_data, udch.m_data, 23U + 2U);

	return *this;
}
