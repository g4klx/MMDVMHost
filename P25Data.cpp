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

#include "P25Data.h"
#include "P25Defines.h"
#include "P25Utils.h"
#include "Hamming.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>

const unsigned char DUMMY_HEADER[] = {
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x08U, 0xDCU, 0x60U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U, 0x93U, 0xE7U, 0x73U, 0x77U, 0x57U, 0xD6U, 0xD3U, 0xCFU, 0x77U,
	0xEEU, 0x82U, 0x93U, 0xE2U, 0x2FU, 0xF3U, 0xD5U, 0xF5U, 0xBEU, 0xBCU, 0x54U, 0x0DU, 0x9CU, 0x29U, 0x3EU, 0x46U,
	0xE3U, 0x28U, 0xB0U, 0xB7U, 0x73U, 0x76U, 0x1EU, 0x26U, 0x0CU, 0x75U, 0x5BU, 0xF7U, 0x4DU, 0x5FU, 0x5AU, 0x37U,
	0x18U};

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CP25Data::CP25Data() :
m_mi(NULL),
m_mfId(0U),
m_algId(0U),
m_kId(0U),
m_lcf(0x00U),
m_emergency(false),
m_srcId(0U),
m_dstId(0U),
m_rs241213(),
m_rs24169()
{
	m_mi = new unsigned char[P25_MI_LENGTH_BYTES];
}

CP25Data::~CP25Data()
{
	delete[] m_mi;
}

void CP25Data::processHeader(unsigned char* data)
{
	assert(data != NULL);

	unsigned char raw[81U];
	CP25Utils::decode(data, raw, 114U, 780U);

	// CUtils::dump(1U, "P25, raw header", raw, 81U);

	// XXX Need to add FEC code

	CP25Utils::encode(DUMMY_HEADER, data, 114U, 780U);
}

void CP25Data::encodeHeader(unsigned char* data)
{
	assert(data != NULL);

	CP25Utils::encode(DUMMY_HEADER, data, 114U, 780U);
}

void CP25Data::processLDU1(unsigned char* data)
{
	assert(data != NULL);

	unsigned char rs[18U];

	unsigned char raw[5U];
	CP25Utils::decode(data, raw, 410U, 452U);
	decodeLDUHamming(raw, rs + 0U);

	CP25Utils::decode(data, raw, 600U, 640U);
	decodeLDUHamming(raw, rs + 3U);

	CP25Utils::decode(data, raw, 788U, 830U);
	decodeLDUHamming(raw, rs + 6U);

	CP25Utils::decode(data, raw, 978U, 1020U);
	decodeLDUHamming(raw, rs + 9U);

	CP25Utils::decode(data, raw, 1168U, 1208U);
	decodeLDUHamming(raw, rs + 12U);

	CP25Utils::decode(data, raw, 1356U, 1398U);
	decodeLDUHamming(raw, rs + 15U);

	// CUtils::dump(1U, "P25, LDU1 Data before", rs, 18U);

	m_rs241213.decode(rs);

	m_lcf  = rs[0U];
	m_mfId = rs[1U];

	switch (m_lcf) {
	case P25_LCF_GROUP:
		m_emergency = (rs[2U] & 0x80U) == 0x80U;
		m_dstId = (rs[4U] << 8) + rs[5U];
		m_srcId = (rs[6U] << 16) + (rs[7U] << 8) + rs[8U];
		break;
	case P25_LCF_PRIVATE:
		m_emergency = false;
		m_dstId = (rs[3U] << 16) + (rs[4U] << 8) + rs[5U];
		m_srcId = (rs[6U] << 16) + (rs[7U] << 8) + rs[8U];
		break;
	default:
		LogMessage("P25, unknown LCF value in LDU1 - $%02X", m_lcf);
		break;
	}

	// m_rs241213.encode(rs);

	// CUtils::dump(1U, "P25, LDU1 Data after", rs, 18U);

	encodeLDUHamming(raw, rs + 0U);
	CP25Utils::encode(raw, data, 410U, 452U);

	encodeLDUHamming(raw, rs + 3U);
	CP25Utils::encode(raw, data, 600U, 640U);

	encodeLDUHamming(raw, rs + 6U);
	CP25Utils::encode(raw, data, 788U, 830U);

	encodeLDUHamming(raw, rs + 9U);
	CP25Utils::encode(raw, data, 978U, 1020U);

	encodeLDUHamming(raw, rs + 12U);
	CP25Utils::encode(raw, data, 1168U, 1208U);

	encodeLDUHamming(raw, rs + 15U);
	CP25Utils::encode(raw, data, 1356U, 1398U);
}

void CP25Data::encodeLDU1(unsigned char* data)
{
	assert(data != NULL);

	unsigned char rs[18U];
	::memset(rs, 0x00U, 18U);

	rs[0U] = m_lcf;
	rs[1U] = m_mfId;

	switch (m_lcf) {
	case P25_LCF_GROUP:
		rs[2U] = m_emergency ? 0x80U : 0x00U;
		rs[4U] = (m_dstId >> 8) & 0xFFU;
		rs[5U] = (m_dstId >> 0) & 0xFFU;
		rs[6U] = (m_srcId >> 16) & 0xFFU;
		rs[7U] = (m_srcId >> 8) & 0xFFU;
		rs[8U] = (m_srcId >> 0) & 0xFFU;
		break;
	case P25_LCF_PRIVATE:
		rs[3U] = (m_dstId >> 16) & 0xFFU;
		rs[4U] = (m_dstId >> 8) & 0xFFU;
		rs[5U] = (m_dstId >> 0) & 0xFFU;
		rs[6U] = (m_srcId >> 16) & 0xFFU;
		rs[7U] = (m_srcId >> 8) & 0xFFU;
		rs[8U] = (m_srcId >> 0) & 0xFFU;
		break;
	default:
		LogMessage("P25, unknown LCF value in LDU1 - $%02X", m_lcf);
		break;
	}

	// m_rs241213.encode(rs);

	// CUtils::dump(1U, "P25, LDU1 Data", rs, 18U);

	unsigned char raw[5U];
	encodeLDUHamming(raw, rs + 0U);
	CP25Utils::encode(raw, data, 410U, 452U);

	encodeLDUHamming(raw, rs + 3U);
	CP25Utils::encode(raw, data, 600U, 640U);

	encodeLDUHamming(raw, rs + 6U);
	CP25Utils::encode(raw, data, 788U, 830U);

	encodeLDUHamming(raw, rs + 9U);
	CP25Utils::encode(raw, data, 978U, 1020U);

	encodeLDUHamming(raw, rs + 12U);
	CP25Utils::encode(raw, data, 1168U, 1208U);

	encodeLDUHamming(raw, rs + 15U);
	CP25Utils::encode(raw, data, 1356U, 1398U);
}

void CP25Data::processLDU2(unsigned char* data)
{
	assert(data != NULL);

	unsigned char rs[18U];

	unsigned char raw[5U];
	CP25Utils::decode(data, raw, 410U, 452U);
	decodeLDUHamming(raw, rs + 0U);

	CP25Utils::decode(data, raw, 600U, 640U);
	decodeLDUHamming(raw, rs + 3U);

	CP25Utils::decode(data, raw, 788U, 830U);
	decodeLDUHamming(raw, rs + 6U);

	CP25Utils::decode(data, raw, 978U, 1020U);
	decodeLDUHamming(raw, rs + 9U);

	CP25Utils::decode(data, raw, 1168U, 1208U);
	decodeLDUHamming(raw, rs + 12U);

	CP25Utils::decode(data, raw, 1356U, 1398U);
	decodeLDUHamming(raw, rs + 15U);

	// CUtils::dump(1U, "P25, LDU2 Data before", rs, 18U);

	m_rs24169.decode(rs);

	::memcpy(m_mi, rs + 0U, P25_MI_LENGTH_BYTES);
	m_algId = rs[9U];
	m_kId   = (rs[10U] << 8) + rs[11U];

	// m_rs24169.encode(rs);

	// CUtils::dump(1U, "P25, LDU2 Data after", rs, 18U);

	encodeLDUHamming(raw, rs + 0U);
	CP25Utils::encode(raw, data, 410U, 452U);

	encodeLDUHamming(raw, rs + 3U);
	CP25Utils::encode(raw, data, 600U, 640U);

	encodeLDUHamming(raw, rs + 6U);
	CP25Utils::encode(raw, data, 788U, 830U);

	encodeLDUHamming(raw, rs + 9U);
	CP25Utils::encode(raw, data, 978U, 1020U);

	encodeLDUHamming(raw, rs + 12U);
	CP25Utils::encode(raw, data, 1168U, 1208U);

	encodeLDUHamming(raw, rs + 15U);
	CP25Utils::encode(raw, data, 1356U, 1398U);
}

void CP25Data::encodeLDU2(unsigned char* data)
{
	assert(data != NULL);

	unsigned char rs[18U];
	::memset(rs, 0x00U, 18U);

	::memcpy(rs + 0U, m_mi, P25_MI_LENGTH_BYTES);

	rs[9U] = m_algId;

	rs[10U] = (m_kId >> 8) & 0xFFU;
	rs[11U] = (m_kId >> 0) & 0xFFU;

	// m_rs24169.encode(rs);

	// CUtils::dump(1U, "P25, LDU2 Data", rs, 18U);

	unsigned char raw[5U];
	encodeLDUHamming(raw, rs + 0U);
	CP25Utils::encode(raw, data, 410U, 452U);

	encodeLDUHamming(raw, rs + 3U);
	CP25Utils::encode(raw, data, 600U, 640U);

	encodeLDUHamming(raw, rs + 6U);
	CP25Utils::encode(raw, data, 788U, 830U);

	encodeLDUHamming(raw, rs + 9U);
	CP25Utils::encode(raw, data, 978U, 1020U);

	encodeLDUHamming(raw, rs + 12U);
	CP25Utils::encode(raw, data, 1168U, 1208U);

	encodeLDUHamming(raw, rs + 15U);
	CP25Utils::encode(raw, data, 1356U, 1398U);
}

void CP25Data::setMI(const unsigned char* mi)
{
	assert(mi != NULL);

	::memcpy(m_mi, mi, P25_MI_LENGTH_BYTES);
}

void CP25Data::getMI(unsigned char* mi) const
{
	assert(mi != NULL);

	::memcpy(mi, m_mi, P25_MI_LENGTH_BYTES);
}

void CP25Data::setMFId(unsigned char id)
{
	m_mfId = id;
}

unsigned char CP25Data::getMFId() const
{
	return m_mfId;
}

void CP25Data::setAlgId(unsigned char id)
{
	m_algId = id;
}

unsigned char CP25Data::getAlgId() const
{
	return m_algId;
}

void CP25Data::setKId(unsigned int id)
{
	m_kId = id;
}

unsigned int CP25Data::getKId() const
{
	return m_kId;
}

void CP25Data::setSrcId(unsigned int id)
{
	m_srcId = id;
}

unsigned int CP25Data::getSrcId() const
{
	return m_srcId;
}

void CP25Data::setEmergency(bool on)
{
	m_emergency = on;
}

bool CP25Data::getEmergency() const
{
	return m_emergency;
}

void CP25Data::setLCF(unsigned char lcf)
{
	m_lcf = lcf;
}

unsigned char CP25Data::getLCF() const
{
	return m_lcf;
}

void CP25Data::setDstId(unsigned int id)
{
	m_dstId = id;
}

unsigned int CP25Data::getDstId() const
{
	return m_dstId;
}

void CP25Data::reset()
{
	::memset(m_mi, 0x00U, P25_MI_LENGTH_BYTES);

	m_algId = 0x80U;
	m_kId   = 0x0000U;
	m_lcf   = P25_LCF_GROUP;
	m_mfId  = 0x00U;
	m_srcId = 0U;
	m_dstId = 0U;
	m_emergency = false;
}

void CP25Data::decodeLDUHamming(const unsigned char* data, unsigned char* raw)
{
	unsigned int n = 0U;
	unsigned int m = 0U;
	for (unsigned int i = 0U; i < 4U; i++) {
		bool hamming[10U];

		for (unsigned int j = 0U; j < 10U; j++) {
			hamming[j] = READ_BIT(data, n);
			n++;
		}

		CHamming::decode1063(hamming);

		for (unsigned int j = 0U; j < 6U; j++) {
			WRITE_BIT(raw, m, hamming[j]);
			m++;
		}
	}
}

void CP25Data::encodeLDUHamming(unsigned char* data, const unsigned char* raw)
{
	unsigned int n = 0U;
	unsigned int m = 0U;
	for (unsigned int i = 0U; i < 4U; i++) {
		bool hamming[10U];

		for (unsigned int j = 0U; j < 6U; j++) {
			hamming[j] = READ_BIT(raw, m);
			m++;
		}

		CHamming::encode1063(hamming);

		for (unsigned int j = 0U; j < 10U; j++) {
			WRITE_BIT(data, n, hamming[j]);
			n++;
		}
	}
}
