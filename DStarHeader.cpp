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

#include "DStarDefines.h"
#include "DStarHeader.h"
#include "CRC.h"

#include <cstdio>
#include <cassert>
#include <cstring>

CDStarHeader::CDStarHeader(const unsigned char* header) :
m_header(NULL)
{
	assert(header != NULL);

	m_header = new unsigned char[DSTAR_HEADER_LENGTH_BYTES];

	::memcpy(m_header, header, DSTAR_HEADER_LENGTH_BYTES);
}

CDStarHeader::CDStarHeader() :
m_header(NULL)
{
	m_header = new unsigned char[DSTAR_HEADER_LENGTH_BYTES];

	::memset(m_header, ' ', DSTAR_HEADER_LENGTH_BYTES);

	m_header[0U] = 0x00U;
	m_header[1U] = 0x00U;
	m_header[2U] = 0x00U;
}

CDStarHeader::~CDStarHeader()
{
	delete[] m_header;
}

CDStarHeader& CDStarHeader::operator=(const CDStarHeader& header)
{
	if (&header != this)
		::memcpy(m_header, header.m_header, DSTAR_HEADER_LENGTH_BYTES);

	return *this;
}

bool CDStarHeader::isRepeater() const
{
	return (m_header[0U] & DSTAR_REPEATER_MASK) == DSTAR_REPEATER_MASK;
}

void CDStarHeader::setRepeater(bool on)
{
	if (on)
		m_header[0U] |= DSTAR_REPEATER_MASK;
	else
		m_header[0U] &= ~DSTAR_REPEATER_MASK;
}

bool CDStarHeader::isDataPacket() const
{
	return (m_header[0U] & DSTAR_DATA_MASK) == DSTAR_DATA_MASK;
}

void CDStarHeader::setUnavailable(bool on)
{
	if (on)
		m_header[0U] |= DSTAR_RELAY_UNAVAILABLE;
	else
		m_header[0U] &= ~DSTAR_RELAY_UNAVAILABLE;
}

void CDStarHeader::getMyCall1(unsigned char* call1) const
{
	assert(call1 != NULL);

	::memcpy(call1, m_header + 27U, DSTAR_LONG_CALLSIGN_LENGTH);
}

void CDStarHeader::getMyCall2(unsigned char* call2) const
{
	assert(call2 != NULL);

	::memcpy(call2, m_header + 35U, DSTAR_SHORT_CALLSIGN_LENGTH);
}

void CDStarHeader::setMyCall1(const unsigned char* call1)
{
	assert(call1 != NULL);

	::memcpy(m_header + 27U, call1, DSTAR_LONG_CALLSIGN_LENGTH);
}

void CDStarHeader::setMyCall2(const unsigned char* call2)
{
	assert(call2 != NULL);

	::memcpy(m_header + 35U, call2, DSTAR_SHORT_CALLSIGN_LENGTH);
}

void CDStarHeader::getRPTCall1(unsigned char* call1) const
{
	assert(call1 != NULL);

	::memcpy(call1, m_header + 11U, DSTAR_LONG_CALLSIGN_LENGTH);
}

void CDStarHeader::getRPTCall2(unsigned char* call2) const
{
	assert(call2 != NULL);

	::memcpy(call2, m_header + 3U, DSTAR_LONG_CALLSIGN_LENGTH);
}

void CDStarHeader::setRPTCall1(const unsigned char* call1)
{
	assert(call1 != NULL);

	::memcpy(m_header + 11U, call1, DSTAR_LONG_CALLSIGN_LENGTH);
}

void CDStarHeader::setRPTCall2(const unsigned char* call2)
{
	assert(call2 != NULL);

	::memcpy(m_header + 3U, call2, DSTAR_LONG_CALLSIGN_LENGTH);
}

void CDStarHeader::getYourCall(unsigned char* call) const
{
	assert(call != NULL);

	::memcpy(call, m_header + 19U, DSTAR_LONG_CALLSIGN_LENGTH);
}

void CDStarHeader::setYourCall(const unsigned char* call)
{
	assert(call != NULL);

	::memcpy(m_header + 19U, call, DSTAR_LONG_CALLSIGN_LENGTH);
}

void CDStarHeader::get(unsigned char* header) const
{
	assert(header != NULL);

	::memcpy(header, m_header, DSTAR_HEADER_LENGTH_BYTES);

	CCRC::addCCITT161(header, DSTAR_HEADER_LENGTH_BYTES);
}
