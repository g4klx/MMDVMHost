/*
 *   Copyright (C) 2020 by Jonathan Naylor G4KLX
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

#include "M17LICH.h"
#include "M17Utils.h"
#include "M17CRC.h"

#include <cassert>
#include <cstring>

CM17LICH::CM17LICH() :
m_lich(NULL),
m_valid(false)
{
	m_lich = new unsigned char[30U];
}

CM17LICH::~CM17LICH()
{
	delete[] m_lich;
}

void CM17LICH::getNetwork(unsigned char* data) const
{
	assert(data != NULL);

	::memcpy(data, m_lich, 28U);
}

void CM17LICH::setNetwork(const unsigned char* data)
{
	assert(data != NULL);

	::memcpy(m_lich, data, 28U);

	m_valid = true;
}

std::string CM17LICH::getSource() const
{
	if (!m_valid)
		return "";

	std::string callsign;
	CM17Utils::decodeCallsign(m_lich + 6U, callsign);

	return callsign;
}

void CM17LICH::setSource(const std::string& callsign)
{
	CM17Utils::encodeCallsign(callsign, m_lich + 6U);
}

std::string CM17LICH::getDest() const
{
	if (!m_valid)
		return "";

	std::string callsign;
	CM17Utils::decodeCallsign(m_lich + 0U, callsign);

	return callsign;
}

void CM17LICH::setDest(const std::string& callsign)
{
	CM17Utils::encodeCallsign(callsign, m_lich + 0U);
}

unsigned char CM17LICH::getDataType() const
{
	if (!m_valid)
		return 0U;

	return (m_lich[12U] >> 1) & 0x03U;
}

void CM17LICH::setDataType(unsigned char type)
{
	m_lich[12U] &= 0xF9U;
	m_lich[12U] |= (type << 1) & 0x06U;
}

void CM17LICH::reset()
{
	::memset(m_lich, 0x00U, 30U);

	m_valid = false;
}

bool CM17LICH::isValid() const
{
	return m_valid;
}

void CM17LICH::getLinkSetup(unsigned char* data) const
{
	assert(data != NULL);

	::memcpy(data, m_lich, 30U);

	CM17CRC::encodeCRC(data, 30U);
}

void CM17LICH::setLinkSetup(const unsigned char* data)
{
	assert(data != NULL);

	::memcpy(m_lich, data, 30U);

	m_valid = CM17CRC::checkCRC(m_lich, 30U);
}

void CM17LICH::getFragment(unsigned char* data, unsigned short fn) const
{
	assert(data != NULL);

	CM17CRC::encodeCRC(m_lich, 30U);

	unsigned int n = (fn & 0x7FFFU) % 5U;

	::memcpy(data, m_lich + (n * 6U), 6U);
}

void CM17LICH::setFragment(const unsigned char* data, unsigned short fn)
{
	assert(data != NULL);

	unsigned int n = (fn & 0x7FFFU) % 5U;

	::memcpy(m_lich + (n * 6U), data, 6U);

	m_valid = CM17CRC::checkCRC(m_lich, 30U);
}
