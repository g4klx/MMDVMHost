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

#include "DStarSlowData.h"
#include "DStarDefines.h"
#include "CRC.h"

#include <cstdio>
#include <cassert>
#include <cstring>

CDStarSlowData::CDStarSlowData() :
m_header(NULL),
m_ptr(0U),
m_buffer(NULL),
m_state(SDD_FIRST)
{
	m_header = new unsigned char[50U];		// DSTAR_HEADER_LENGTH_BYTES
	m_buffer = new unsigned char[DSTAR_DATA_FRAME_LENGTH_BYTES * 2U];
}

CDStarSlowData::~CDStarSlowData()
{
	delete[] m_header;
	delete[] m_buffer;
}

CDStarHeader* CDStarSlowData::add(const unsigned char* data)
{
	assert(data != NULL);

	switch (m_state) {
	case SDD_FIRST:
		m_buffer[0U] = data[9U]  ^ DSTAR_SCRAMBLER_BYTES[0U];
		m_buffer[1U] = data[10U] ^ DSTAR_SCRAMBLER_BYTES[1U];
		m_buffer[2U] = data[11U] ^ DSTAR_SCRAMBLER_BYTES[2U];
		m_state = SDD_SECOND;
		return NULL;

	case SDD_SECOND:
		m_buffer[3U] = data[9U]  ^ DSTAR_SCRAMBLER_BYTES[0U];
		m_buffer[4U] = data[10U] ^ DSTAR_SCRAMBLER_BYTES[1U];
		m_buffer[5U] = data[11U] ^ DSTAR_SCRAMBLER_BYTES[2U];
		m_state = SDD_FIRST;
		break;
	}

	if ((m_buffer[0U] & DSTAR_SLOW_DATA_TYPE_MASK) != DSTAR_SLOW_DATA_TYPE_HEADER)
		return NULL;

	::memcpy(m_header + m_ptr, m_buffer + 1U, 5U);
	m_ptr += 5U;

	if (m_ptr < DSTAR_HEADER_LENGTH_BYTES)
		return NULL;

	// Clean up the data
	m_header[0U] &= (DSTAR_INTERRUPTED_MASK | DSTAR_URGENT_MASK | DSTAR_REPEATER_MASK);
	m_header[1U] = 0x00U;
	m_header[2U] = 0x00U;

	for (unsigned int i = 3U; i < 39U; i++)
		m_header[i] &= 0x7FU;

	// Save the CRC for later comparison
	unsigned char crc[2U];
	::memcpy(crc, m_header + 39U, 2U);

	// Add the new CRC
	CCRC::addCCITT16(m_header, DSTAR_HEADER_LENGTH_BYTES);

	m_ptr = 0U;

	// Compare them
	if (crc[0U] != m_header[39U] || crc[1U] != m_header[40U])
		return NULL;

	return new CDStarHeader(m_header);
}

void CDStarSlowData::start()
{
	::memset(m_header, 0x00U, DSTAR_HEADER_LENGTH_BYTES);

	m_ptr   = 0U;
	m_state = SDD_FIRST;
}

void CDStarSlowData::reset()
{
	m_ptr = 0U;
	m_state = SDD_FIRST;
}
