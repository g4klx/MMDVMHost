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
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

CDStarSlowData::CDStarSlowData() :
m_header(NULL),
m_ptr(0U),
m_buffer(NULL),
m_text(NULL),
m_textPtr(0U),
m_state(SDD_FIRST)
{
	m_header = new unsigned char[50U];		// DSTAR_HEADER_LENGTH_BYTES
	m_buffer = new unsigned char[DSTAR_DATA_FRAME_LENGTH_BYTES * 2U];
	m_text   = new unsigned char[24U];
}

CDStarSlowData::~CDStarSlowData()
{
	delete[] m_header;
	delete[] m_buffer;
	delete[] m_text;
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

	if (m_ptr >= 45U)
		return NULL;

	::memcpy(m_header + m_ptr, m_buffer + 1U, 5U);
	m_ptr += 5U;

	// Clean up the data
	m_header[0U] &= (DSTAR_INTERRUPTED_MASK | DSTAR_URGENT_MASK | DSTAR_REPEATER_MASK);
	m_header[1U] = 0x00U;
	m_header[2U] = 0x00U;

	for (unsigned int i = 3U; i < 39U; i++)
		m_header[i] &= 0x7FU;

	// Check the CRC
	bool ret = CCRC::checkCCITT161(m_header, DSTAR_HEADER_LENGTH_BYTES);
	if (!ret) {
		if (m_ptr == 45U)
			LogMessage("D-Star, invalid slow data header");
		return NULL;
	}

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

void CDStarSlowData::setText(const char* text)
{
	assert(text != NULL);

	m_text[0U] = DSTAR_SLOW_DATA_TYPE_TEXT | 0U;
	m_text[1U] = text[0U];
	m_text[2U] = text[1U];
	m_text[3U] = text[2U];
	m_text[4U] = text[3U];
	m_text[5U] = text[4U];

	m_text[6U] = DSTAR_SLOW_DATA_TYPE_TEXT | 1U;
	m_text[7U] = text[5U];
	m_text[8U] = text[6U];
	m_text[9U] = text[7U];
	m_text[10U] = text[8U];
	m_text[11U] = text[9U];

	m_text[12U] = DSTAR_SLOW_DATA_TYPE_TEXT | 2U;
	m_text[13U] = text[10U];
	m_text[14U] = text[11U];
	m_text[15U] = text[12U];
	m_text[16U] = text[13U];
	m_text[17U] = text[14U];

	m_text[18U] = DSTAR_SLOW_DATA_TYPE_TEXT | 3U;
	m_text[19U] = text[15U];
	m_text[20U] = text[16U];
	m_text[21U] = text[17U];
	m_text[22U] = text[18U];
	m_text[23U] = text[19U];

	m_textPtr = 0U;
}

void CDStarSlowData::get(unsigned char* data)
{
	assert(data != NULL);

	if (m_textPtr < 24U) {
		data[0U] = m_text[m_textPtr++] ^ DSTAR_SCRAMBLER_BYTES[0U];
		data[1U] = m_text[m_textPtr++] ^ DSTAR_SCRAMBLER_BYTES[1U];
		data[2U] = m_text[m_textPtr++] ^ DSTAR_SCRAMBLER_BYTES[2U];
	} else {
		data[0U] = 'f' ^ DSTAR_SCRAMBLER_BYTES[0U];
		data[1U] = 'f' ^ DSTAR_SCRAMBLER_BYTES[1U];
		data[2U] = 'f' ^ DSTAR_SCRAMBLER_BYTES[2U];
	}
}
