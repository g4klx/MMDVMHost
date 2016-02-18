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

#include "DMRCSBK.h"
#include "BPTC19696.h"
#include "Utils.h"
#include "CRC.h"

#include <cstdio>
#include <cassert>

CDMRCSBK::CDMRCSBK(const unsigned char* bytes) :
m_CSBKO(CSBKO_NONE),
m_FID(0x00U),
m_bsId(0U),
m_srcId(0U),
m_dstId(0U),
m_valid(false)
{
	assert(bytes != NULL);

	CBPTC19696 bptc;

	unsigned char data[12U];
	bptc.decode(bytes, data);

	data[10U] ^= CSBK_CRC_MASK[0U];
	data[11U] ^= CSBK_CRC_MASK[1U];

	m_valid = CCRC::checkCCITT162(data, 12U);
	if (!m_valid)
		return;

	m_CSBKO = CSBKO(data[0U] & 0x3FU);
	m_FID   = data[1U];

	switch (m_CSBKO) {
	case CSBKO_BSDWNACT:
		m_bsId  = data[4U] << 16 | data[5U] << 8 | data[6U];
		m_srcId = data[7U] << 16 | data[8U] << 8 | data[9U]; 
		CUtils::dump("Download Activate CSBK", data, 12U);
		break;

	case CSBKO_UUVREQ:
		m_dstId = data[4U] << 16 | data[5U] << 8 | data[6U];
		m_srcId = data[7U] << 16 | data[8U] << 8 | data[9U];
		CUtils::dump("Unit to Unit Service Request CSBK", data, 12U);
		break;

	case CSBKO_UUANSRSP:
		m_dstId = data[4U] << 16 | data[5U] << 8 | data[6U];
		m_srcId = data[7U] << 16 | data[8U] << 8 | data[9U];
		CUtils::dump("Unit to Unit Service Answer Response CSBK", data, 12U);
		break;

	case CSBKO_PRECCSBK:
		m_dstId = data[4U] << 16 | data[5U] << 8 | data[6U];
		m_srcId = data[7U] << 16 | data[8U] << 8 | data[9U];
		CUtils::dump("Preamble CSBK", data, 12U);
		break;

	case CSBKO_NACKRSP:
		m_srcId = data[4U] << 16 | data[5U] << 8 | data[6U];
		m_dstId = data[7U] << 16 | data[8U] << 8 | data[9U];
		CUtils::dump("Negative Acknowledge Response CSBK", data, 12U);
		break;

	default:
		CUtils::dump("Unhandled CSBK type", data, 12U);
		break;
	}
}

CDMRCSBK::~CDMRCSBK()
{
}

bool CDMRCSBK::isValid() const
{
	return m_valid;
}

CSBKO CDMRCSBK::getCSBKO() const
{
	return m_CSBKO;
}

unsigned char CDMRCSBK::getFID() const
{
	return m_FID;
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
