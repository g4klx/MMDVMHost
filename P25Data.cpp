/*
*   Copyright (C) 2016,2017,2023,2024,2025 by Jonathan Naylor G4KLX
*   Copyright (C) 2018 by Bryan Biedenkapp <gatekeep@gmail.com> N2PLL
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
#include "CRC.h"
#include "Hamming.h"
#include "Golay24128.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>
	
const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CP25Data::CP25Data() :
m_mi(nullptr),
m_mfId(0U),
m_algId(P25_ALGO_UNENCRYPT),
m_kId(0U),
m_lcf(0x00U),
m_emergency(false),
m_srcId(0U),
m_dstId(0U),
m_rs(),
m_trellis()
{
	m_mi = new unsigned char[P25_MI_LENGTH_BYTES];
	::memset(m_mi, 0x00U, P25_MI_LENGTH_BYTES);
}

CP25Data::~CP25Data()
{
	delete[] m_mi;
}

CP25Data& CP25Data::operator=(const CP25Data& data)
{
	if (this != &data) {
		m_mfId = data.m_mfId;

		m_srcId = data.m_srcId;
		m_dstId = data.m_dstId;

		m_emergency = data.m_emergency;

		m_algId = data.m_algId;
		m_kId   = data.m_kId;

		::memcpy(m_mi, data.m_mi, P25_MI_LENGTH_BYTES);
	}

	return *this;
}

bool CP25Data::decodeHeader(const unsigned char* data)
{
	assert(data != nullptr);

	// deinterleave
	unsigned char rs[81U];
	unsigned char raw[81U];
	CP25Utils::decode(data, raw, 114U, 780U);

	// decode Golay (18,6,8) FEC
	decodeHeaderGolay(raw, rs);

	// decode RS (36,20,17) FEC
	try {
		bool ret = m_rs.decode362017(rs);
		if (!ret)
			return false;
	} catch (...) {
		CUtils::dump(2U, "P25, RS crashed with input data", rs, 81U);
		return false;
	}

	m_mfId  = rs[9U];						// Mfg Id.
/*
	m_algId = rs[10U];						// Algorithm ID

	if (m_algId != P25_ALGO_UNENCRYPT) {
		::memcpy(m_mi, rs, P25_MI_LENGTH_BYTES);		// Message Indicator

		m_kId = (rs[11U] << 8) | (rs[12U] << 0);		// Key ID
	} else {
		::memset(m_mi, 0x00U, P25_MI_LENGTH_BYTES);

		m_kId = 0x0000U;
	}
*/
	return true;
}

void CP25Data::encodeHeader(unsigned char* data)
{
	assert(data != nullptr);
	assert(m_mi != nullptr);

	unsigned char rs[81U];
	::memset(rs, 0x00U, 81U);

	for (unsigned int i = 0; i < P25_MI_LENGTH_BYTES; i++)
		rs[i] = m_mi[i];                                                        // Message Indicator

	rs[9U]  = m_mfId;                                                               // Mfg Id.
	rs[10U] = m_algId;                                                              // Algorithm ID
	rs[11U] = (m_kId >> 8) & 0xFFU;                                                 // Key ID MSB
	rs[12U] = (m_kId >> 0) & 0xFFU;                                                 // Key ID LSB
	rs[13U] = (m_dstId >> 8) & 0xFFU;                                               // Talkgroup Address MSB
	rs[14U] = (m_dstId >> 0) & 0xFFU;                                               // Talkgroup Address LSB

	// encode RS (36,20,17) FEC
	m_rs.encode362017(rs);

	unsigned char raw[81U];
	::memset(raw, 0x00U, 81U);

	// encode Golay (18,6,8) FEC
	encodeHeaderGolay(raw, rs);

	// interleave
	CP25Utils::encode(raw, data, 114U, 780U);
}

bool CP25Data::decodeLDU1(const unsigned char* data)
{
	assert(data != nullptr);

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

	try {
		bool ret = m_rs.decode241213(rs);
		if (!ret)
			return false;
	} catch (...) {
		CUtils::dump(2U, "P25, RS carshed with input data", rs, 18U);
		return false;
	}

	unsigned int srcId = (rs[6U] << 16) + (rs[7U] << 8) + rs[8U];

	switch (rs[0U]) {
		case P25_LCF_GROUP:
			m_emergency = (rs[2U] & 0x80U) == 0x80U;
			m_dstId = (rs[4U] << 8) + rs[5U];
			m_srcId = srcId;
			break;
		case P25_LCF_PRIVATE:
			m_emergency = false;
			m_dstId = (rs[3U] << 16) + (rs[4U] << 8) + rs[5U];
			m_srcId = srcId;
			break;
		default:
			return false;
	}

	m_lcf  = rs[0U];
	m_mfId = rs[1U];

	return true;
}

void CP25Data::encodeLDU1(unsigned char* data)
{
	assert(data != nullptr);

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

	m_rs.encode241213(rs);

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

bool CP25Data::decodeLDU2(const unsigned char* data)
{
	assert(data != nullptr);

	unsigned char rs[18U];

	// deinterleave and decode Hamming (10,6,3) for LC data
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

	// decode RS (24,16,9) FEC
	try {
		bool ret = m_rs.decode24169(rs);
		if (!ret)
			return false;
	} catch (...) {
		CUtils::dump(2U, "P25, RS crashed with input data", rs, 18U);
		return false;
	}
/*
	m_algId = rs[9U];                                                    // Algorithm ID

	if (m_algId != P25_ALGO_UNENCRYPT) {
		::memcpy(m_mi, rs, P25_MI_LENGTH_BYTES);                     // Message Indicator

		m_kId = (rs[10U] << 8) + rs[11U];                            // Key ID
	}
	else {
		::memset(m_mi, 0x00U, P25_MI_LENGTH_BYTES);

		m_kId = 0x0000U;
	}
*/
	return true;
}

void CP25Data::encodeLDU2(unsigned char* data)
{
	assert(data != nullptr);
	assert(m_mi != nullptr);

	unsigned char rs[18U];
	::memset(rs, 0x00U, 18U);

	for (unsigned int i = 0; i < P25_MI_LENGTH_BYTES; i++)
		rs[i] = m_mi[i];                                             // Message Indicator

	rs[9U]  = m_algId;                                                   // Algorithm ID
	rs[10U] = (m_kId >> 8) & 0xFFU;                                      // Key ID MSB
	rs[11U] = (m_kId >> 0) & 0xFFU;                                      // Key ID LSB

	// encode RS (24,16,9) FEC
	m_rs.encode24169(rs);

	// encode Hamming (10,6,3) FEC and interleave for LC data
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

bool CP25Data::decodeTSDU(const unsigned char* data)
{
    assert(data != nullptr);

    // deinterleave
    unsigned char tsbk[12U];
    unsigned char raw[25U];
    CP25Utils::decode(data, raw, 114U, 318U);

    // decode 1/2 rate Trellis & check CRC-CCITT 16
    try {
        bool ret = m_trellis.decode12(raw, tsbk);
        if (ret)
            ret = CCRC::checkCCITT162(tsbk, 12U);
        if (!ret)
            return false;
    }   
    catch (...) {
        CUtils::dump(2U, "P25, CRC failed with input data", tsbk, 12U);
        return false;
    }   

    m_lcf  = tsbk[0U] & 0x3F;
    m_mfId = tsbk[1U];

    unsigned long long tsbkValue = 0U; 

    // combine bytes into rs value
    tsbkValue = tsbk[2U];
    tsbkValue = (tsbkValue << 8) + tsbk[3U];
    tsbkValue = (tsbkValue << 8) + tsbk[4U];
    tsbkValue = (tsbkValue << 8) + tsbk[5U];
    tsbkValue = (tsbkValue << 8) + tsbk[6U];
    tsbkValue = (tsbkValue << 8) + tsbk[7U];
    tsbkValue = (tsbkValue << 8) + tsbk[8U];
    tsbkValue = (tsbkValue << 8) + tsbk[9U];

    switch (m_lcf) {
	    case P25_LCF_TSBK_CALL_ALERT:
		m_dstId = (unsigned int)((tsbkValue >> 24) & 0xFFFFFFU);    // Target Radio Address
		m_srcId = (unsigned int)(tsbkValue & 0xFFFFFFU);            // Source Radio Address
		break;
	    case P25_LCF_TSBK_ACK_RSP_FNE:
		m_serviceType = (unsigned char)((tsbkValue >> 56) & 0xFFU); // Service Type
		m_dstId = (unsigned int)((tsbkValue >> 24) & 0xFFFFFFU);    // Target Radio Address
		m_srcId = (unsigned int)(tsbkValue & 0xFFFFFFU);            // Source Radio Address
		break;
	    default:
		LogMessage("P25, unknown LCF value in TSDU - $%02X", m_lcf);
		break;
    }

    return true;
}

void CP25Data::encodeTSDU(unsigned char* data)
{
    assert(data != nullptr);

    unsigned char tsbk[12U];
    ::memset(tsbk, 0x00U, 12U);

    unsigned long long tsbkValue = 0U;
    tsbk[0U] = m_lcf;
    tsbk[0U] |= 0x80;

    tsbk[1U] = m_mfId;

    switch (m_lcf) {
	    case P25_LCF_TSBK_CALL_ALERT:
		tsbkValue = 0U;
		tsbkValue = (tsbkValue << 16) + 0U;
		tsbkValue = (tsbkValue << 24) + m_dstId;                    // Target Radio Address
		tsbkValue = (tsbkValue << 24) + m_srcId;                    // Source Radio Address
		break;
	    case P25_LCF_TSBK_ACK_RSP_FNE:
		tsbkValue = 0U;                                             // Additional Info. Flag
		tsbkValue = (tsbkValue << 1) + 0U;                          // Extended Address Flag
		tsbkValue = (tsbkValue << 16) + (m_serviceType & 0xFF);     // Service Type
		tsbkValue = (tsbkValue << 32) + m_dstId;                    // Target Radio Address
		tsbkValue = (tsbkValue << 24) + m_srcId;                    // Source Radio Address
		break;
	    default:
		LogMessage("P25, unknown LCF value in TSDU - $%02X", m_lcf);
		break;
    }

    // split rs value into bytes
    tsbk[2U] = (unsigned char)((tsbkValue >> 56) & 0xFFU);
    tsbk[3U] = (unsigned char)((tsbkValue >> 48) & 0xFFU);
    tsbk[4U] = (unsigned char)((tsbkValue >> 40) & 0xFFU);
    tsbk[5U] = (unsigned char)((tsbkValue >> 32) & 0xFFU);
    tsbk[6U] = (unsigned char)((tsbkValue >> 24) & 0xFFU);
    tsbk[7U] = (unsigned char)((tsbkValue >> 16) & 0xFFU);
    tsbk[8U] = (unsigned char)((tsbkValue >> 8) & 0xFFU);
    tsbk[9U] = (unsigned char)((tsbkValue >> 0) & 0xFFU);

    // compute CRC-CCITT 16
    CCRC::addCCITT162(tsbk, 12U);

    unsigned char raw[25U];
    ::memset(raw, 0x00U, 25U);

    // encode 1/2 rate Trellis
    m_trellis.encode12(tsbk, raw);

    // interleave
    CP25Utils::encode(raw, data, 114U, 318U);
}

void CP25Data::setMI(const unsigned char* mi)
{
	assert(mi != nullptr);

	::memcpy(m_mi, mi, P25_MI_LENGTH_BYTES);
}

void CP25Data::getMI(unsigned char* mi) const
{
	assert(mi != nullptr);

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

void CP25Data::setServiceType(unsigned char type)
{
    m_serviceType = type;
}

unsigned char CP25Data::getServiceType() const
{
    return m_serviceType;
}

void CP25Data::reset()
{
	::memset(m_mi, 0x00U, P25_MI_LENGTH_BYTES);

	m_algId = P25_ALGO_UNENCRYPT;
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

void CP25Data::decodeHeaderGolay(const unsigned char* data, unsigned char* raw)
{
	// shortened Golay (18,6,8) decode
	unsigned int n = 0U;
	unsigned int m = 0U;
	for (unsigned int i = 0U; i < 36U; i++) {
		bool golay[18U];
		for (unsigned int j = 0U; j < 18U; j++) {
			golay[j] = READ_BIT(data, n);
			n++;
		}

		unsigned int g0 = 0U;
		for (unsigned int j = 0U; j < 18U; j++)
			g0 = (g0 << 1) | (golay[j] ? 0x01U : 0x00U);

		unsigned int c0data = 0U;
		CGolay24128::decode24128(g0, c0data);

		for (int j = 5; j >= 0; j--) {
			golay[j] = (c0data & 0x01U) == 0x01U;
			c0data >>= 1;
		}

		for (unsigned int j = 0U; j < 6U; j++) {
			WRITE_BIT(raw, m, golay[j]);
			m++;
		}
	}
}

void CP25Data::encodeHeaderGolay(unsigned char* data, const unsigned char* raw)
{
	// shortened Golay (18,6,8) encode
	unsigned int n = 0U;
	unsigned int m = 0U;
	for (unsigned int i = 0U; i < 36U; i++) {
		bool golay[18U];
		for (unsigned int j = 0U; j < 6U; j++) {
			golay[j] = READ_BIT(raw, m);
			m++;
		}

		unsigned int c0data = 0U;
		for (unsigned int j = 0U; j < 6U; j++)
			c0data = (c0data << 1) | (golay[j] ? 0x01U : 0x00U);
		unsigned int g0 = CGolay24128::encode24128(c0data);
		for (int j = 17; j >= 0; j--) {
			golay[j] = (g0 & 0x01U) == 0x01U;
			g0 >>= 1;
		}

		for (unsigned int j = 0U; j < 18U; j++) {
			WRITE_BIT(data, n, golay[j]);
			n++;
		}
	}
}
