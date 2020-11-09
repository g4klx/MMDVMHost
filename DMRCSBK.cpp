/*
 *   Copyright (C) 2015,2016,2020 by Jonathan Naylor G4KLX
 *   Copyright (C) 2019 by Patrick Maier DK5MP
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

#include "DMRCSBK.h"
#include "BPTC19696.h"
#include "Utils.h"
#include "CRC.h"

#include <cstdio>
#include <cassert>

CDMRCSBK::CDMRCSBK() :
m_data(NULL),
m_CSBKO(CSBKO_NONE),
m_FID(0x00U),
m_GI(false),
m_bsId(0U),
m_srcId(0U),
m_dstId(0U),
m_dataContent(false),
m_CBF(0U)
{
	m_data = new unsigned char[12U];
}

CDMRCSBK::~CDMRCSBK()
{
	delete[] m_data;
}

bool CDMRCSBK::put(const unsigned char* bytes)
{
	assert(bytes != NULL);

	CBPTC19696 bptc;
	bptc.decode(bytes, m_data);

	m_data[10U] ^= CSBK_CRC_MASK[0U];
	m_data[11U] ^= CSBK_CRC_MASK[1U];

	bool valid = CCRC::checkCCITT162(m_data, 12U);
	if (!valid)
		return false;

	// Restore the checksum
	m_data[10U] ^= CSBK_CRC_MASK[0U];
	m_data[11U] ^= CSBK_CRC_MASK[1U];

	m_CSBKO = CSBKO(m_data[0U] & 0x3FU);
	m_FID   = m_data[1U];

	switch (m_CSBKO) {
	case CSBKO_BSDWNACT:
		m_GI    = false;
		m_bsId  = m_data[4U] << 16 | m_data[5U] << 8 | m_data[6U];
		m_srcId = m_data[7U] << 16 | m_data[8U] << 8 | m_data[9U]; 
		m_dataContent = false;
		m_CBF   = 0U;
		CUtils::dump(1U, "Downlink Activate CSBK", m_data, 12U);
		break;

	case CSBKO_UUVREQ:
		m_GI    = false;
		m_dstId = m_data[4U] << 16 | m_data[5U] << 8 | m_data[6U];
		m_srcId = m_data[7U] << 16 | m_data[8U] << 8 | m_data[9U];
		m_dataContent = false;
		m_CBF   = 0U;
		CUtils::dump(1U, "Unit to Unit Service Request CSBK", m_data, 12U);
		break;

	case CSBKO_UUANSRSP:
		m_GI    = false;
		m_dstId = m_data[4U] << 16 | m_data[5U] << 8 | m_data[6U];
		m_srcId = m_data[7U] << 16 | m_data[8U] << 8 | m_data[9U];
		m_dataContent = false;
		m_CBF   = 0U;
		CUtils::dump(1U, "Unit to Unit Service Answer Response CSBK", m_data, 12U);
		break;

	case CSBKO_PRECCSBK:
		m_GI    = (m_data[2U] & 0x40U) == 0x40U;
		m_dstId = m_data[4U] << 16 | m_data[5U] << 8 | m_data[6U];
		m_srcId = m_data[7U] << 16 | m_data[8U] << 8 | m_data[9U];
		m_dataContent = (m_data[2U] & 0x80U) == 0x80U;
		m_CBF   = m_data[3U];
		CUtils::dump(1U, "Preamble CSBK", m_data, 12U);
		break;

	case CSBKO_NACKRSP:
		m_GI    = false;
		m_srcId = m_data[4U] << 16 | m_data[5U] << 8 | m_data[6U];
		m_dstId = m_data[7U] << 16 | m_data[8U] << 8 | m_data[9U];
		m_dataContent = false;
		m_CBF   = 0U;
		CUtils::dump(1U, "Negative Acknowledge Response CSBK", m_data, 12U);
		break;

	case CSBKO_CALL_ALERT:
		m_GI    = false;
		m_dstId = m_data[4U] << 16 | m_data[5U] << 8 | m_data[6U];
		m_srcId = m_data[7U] << 16 | m_data[8U] << 8 | m_data[9U];
		m_dataContent = false;
		m_CBF   = 0U;
		CUtils::dump(1U, "Call Alert CSBK", m_data, 12U);
		break;

	case CSBKO_CALL_ALERT_ACK:
		m_GI    = false;
		m_dstId = m_data[4U] << 16 | m_data[5U] << 8 | m_data[6U];
		m_srcId = m_data[7U] << 16 | m_data[8U] << 8 | m_data[9U];
		m_dataContent = false;
		m_CBF   = 0U;
		CUtils::dump(1U, "Call Alert Ack CSBK", m_data, 12U);
		break;

	default:
		m_GI    = false;
		m_srcId = 0U;
		m_dstId = 0U;
		m_dataContent = false;
		m_CBF = 0U;
		CUtils::dump("Unhandled CSBK type", m_data, 12U);
		return true;
	}

	return true;
}

void CDMRCSBK::get(unsigned char* bytes) const
{
	assert(bytes != NULL);

	m_data[10U] ^= CSBK_CRC_MASK[0U];
	m_data[11U] ^= CSBK_CRC_MASK[1U];

	CCRC::addCCITT162(m_data, 12U);

	m_data[10U] ^= CSBK_CRC_MASK[0U];
	m_data[11U] ^= CSBK_CRC_MASK[1U];

	CBPTC19696 bptc;
	bptc.encode(m_data, bytes);
}

CSBKO CDMRCSBK::getCSBKO() const
{
	return m_CSBKO;
}

unsigned char CDMRCSBK::getFID() const
{
	return m_FID;
}

bool CDMRCSBK::getOVCM() const
{
	bool bOVCM = false;
	// Service options informations are only available in
	// "Unit to Unit Voice Service Request CSBK" and
	// "Unit to Unit Voice Service Answer Response CSBK"
	if ((m_CSBKO == CSBKO_UUVREQ) || (m_CSBKO == CSBKO_UUANSRSP))
		bOVCM = (m_data[2U] & 0x04U) == 0x04U;

	return bOVCM;
}

void CDMRCSBK::setOVCM(bool ovcm)
{
	// Set OVCM only in CSBKs having the service options information
	if ((m_CSBKO == CSBKO_UUVREQ) || (m_CSBKO == CSBKO_UUANSRSP)) {
		if (ovcm)
			m_data[2U] |= 0x04U;
	}
}

bool CDMRCSBK::getGI() const
{
	return m_GI;
}

unsigned int CDMRCSBK::getBSId() const
{
	return m_bsId;
}

unsigned int CDMRCSBK::getSrcId() const
{
	return m_srcId;
}

unsigned int CDMRCSBK::getDstId() const
{
	return m_dstId;
}

bool CDMRCSBK::getDataContent() const
{
	return m_dataContent;
}

unsigned char CDMRCSBK::getCBF() const
{
	return m_CBF;
}

void CDMRCSBK::setCBF(unsigned char cbf)
{
	m_CBF = m_data[3U] = cbf;
}
