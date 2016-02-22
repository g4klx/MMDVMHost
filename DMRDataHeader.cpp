/*
 *	 Copyright (C) 2012 by Ian Wraith
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

#include "DMRDataHeader.h"
#include "DMRDefines.h"
#include "BPTC19696.h"
#include "Utils.h"
#include "CRC.h"
#include "Log.h"

#include <cstdio>
#include <cassert>

CDMRDataHeader::CDMRDataHeader() :
m_data(NULL),
m_gi(false),
m_srcId(0U),
m_dstId(0U),
m_blocks(0U)
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

	m_gi = (m_data[0U] & 0x80U) == 0x80U;

	m_dstId = m_data[2U] << 16 | m_data[3U] << 8 | m_data[4U];
	m_srcId = m_data[5U] << 16 | m_data[6U] << 8 | m_data[7U];

	m_blocks = m_data[8U] & 0x7FU;

	CUtils::dump("Data Header", m_data, 12U);

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
	return m_gi;
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
