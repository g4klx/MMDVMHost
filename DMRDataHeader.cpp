/*
 *   Copyright (C) 2012 by Ian Wraith
 *   Copyright (C) 2015,2016,2017 by Jonathan Naylor G4KLX
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

#include "DMRDataHeader.h"
#include "DMRDefines.h"
#include "BPTC19696.h"
#include "RS129.h"
#include "Utils.h"
#include "CRC.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned char UDTF_NMEA = 0x05U;

CDMRDataHeader::CDMRDataHeader() :
m_data(NULL),
m_GI(false),
m_A(false),
m_srcId(0U),
m_dstId(0U),
m_blocks(0U),
m_F(false),
m_S(false),
m_Ns(0U)
{
	m_data = new unsigned char[12U];
}

CDMRDataHeader::~CDMRDataHeader()
{
	delete[] m_data;
}

bool CDMRDataHeader::put(const unsigned char* bytes)
{
	assert(bytes != NULL);

	CBPTC19696 bptc;
	bptc.decode(bytes, m_data);

	m_data[10U] ^= DATA_HEADER_CRC_MASK[0U];
	m_data[11U] ^= DATA_HEADER_CRC_MASK[1U];

	bool valid = CCRC::checkCCITT162(m_data, 12U);
	if (!valid)
		return false;

	// Restore the checksum
	m_data[10U] ^= DATA_HEADER_CRC_MASK[0U];
	m_data[11U] ^= DATA_HEADER_CRC_MASK[1U];

	m_GI = (m_data[0U] & 0x80U) == 0x80U;
	m_A  = (m_data[0U] & 0x40U) == 0x40U;

	unsigned char dpf = m_data[0U] & 0x0FU;
	if (dpf == DPF_PROPRIETARY)
		return true;

	m_dstId = m_data[2U] << 16 | m_data[3U] << 8 | m_data[4U];
	m_srcId = m_data[5U] << 16 | m_data[6U] << 8 | m_data[7U];

	switch (dpf) {
	case DPF_UNCONFIRMED_DATA:
		CUtils::dump(1U, "DMR, Unconfirmed Data Header", m_data, 12U);
		m_F = (m_data[8U] & 0x80U) == 0x80U;
		m_blocks = m_data[8U] & 0x7FU;
		break;

	case DPF_CONFIRMED_DATA:
		CUtils::dump(1U, "DMR, Confirmed Data Header", m_data, 12U);
		m_F = (m_data[8U] & 0x80U) == 0x80U;
		m_blocks = m_data[8U] & 0x7FU;
		m_S = (m_data[9U] & 0x80U) == 0x80U;
		m_Ns = (m_data[9U] >> 4) & 0x07U;
		break;

	case DPF_RESPONSE:
		CUtils::dump(1U, "DMR, Response Data Header", m_data, 12U);
		m_blocks = m_data[8U] & 0x7FU;
		break;

	case DPF_PROPRIETARY:
		CUtils::dump(1U, "DMR, Proprietary Data Header", m_data, 12U);
		break;

	case DPF_DEFINED_RAW:
		CUtils::dump(1U, "DMR, Raw or Status/Precoded Short Data Header", m_data, 12U);
		m_blocks = (m_data[0U] & 0x30U) + (m_data[1U] & 0x0FU);
		m_F = (m_data[8U] & 0x01U) == 0x01U;
		m_S = (m_data[8U] & 0x02U) == 0x02U;
		break;

	case DPF_DEFINED_SHORT:
		CUtils::dump(1U, "DMR, Defined Short Data Header", m_data, 12U);
		m_blocks = (m_data[0U] & 0x30U) + (m_data[1U] & 0x0FU);
		m_F = (m_data[8U] & 0x01U) == 0x01U;
		m_S = (m_data[8U] & 0x02U) == 0x02U;
		break;

	case DPF_UDT:
		CUtils::dump(1U, "DMR, Unified Data Transport Header", m_data, 12U);
		m_blocks = (m_data[8U] & 0x03U) + 1U;
		break;

	default:
		CUtils::dump("DMR, Unknown Data Header", m_data, 12U);
		break;
	}

	return true;
}

void CDMRDataHeader::get(unsigned char* bytes) const
{
	assert(bytes != NULL);

	CBPTC19696 bptc;
	bptc.encode(m_data, bytes);
}

bool CDMRDataHeader::getGI() const
{
	return m_GI;
}

unsigned int CDMRDataHeader::getSrcId() const
{
	return m_srcId;
}

unsigned int CDMRDataHeader::getDstId() const
{
	return m_dstId;
}

unsigned int CDMRDataHeader::getBlocks() const
{
	return m_blocks;
}

CDMRDataHeader& CDMRDataHeader::operator=(const CDMRDataHeader& header)
{
	if (&header != this) {
		::memcpy(m_data, header.m_data, 12U);
		m_GI     = header.m_GI;
		m_A      = header.m_A;
		m_srcId  = header.m_srcId;
		m_dstId  = header.m_dstId;
		m_blocks = header.m_blocks;
		m_F      = header.m_F;
		m_S      = header.m_S;
		m_Ns     = header.m_Ns;
	}

	return *this;
}
