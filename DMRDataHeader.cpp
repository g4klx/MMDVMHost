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
#include "CRC.h"
#include "Log.h"

#include <cstdio>
#include <cassert>

CDMRDataHeader::CDMRDataHeader(const unsigned char* data) :
m_bptc(),
m_valid(false),
m_gi(false),
m_srcId(0U),
m_dstId(0U),
m_blocks(0U)
{
	assert(data != NULL);

	unsigned char header[12U];
	m_bptc.decode(data, header);

	header[10U] ^= DATA_HEADER_CRC_MASK[0U];
	header[11U] ^= DATA_HEADER_CRC_MASK[1U];

	m_valid = CCRC::checkCCITT162(header, 12U);

	m_gi = (header[0U] & 0x80U) == 0x80U;

	m_dstId = data[2U] << 16 | data[3U] << 8 | data[4U];
	m_srcId = data[5U] << 16 | data[6U] << 8 | data[7U];

	m_blocks = data[8U] & 0x7FU;
}

CDMRDataHeader::~CDMRDataHeader()
{
}

bool CDMRDataHeader::isValid() const
{
	return m_valid;
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
