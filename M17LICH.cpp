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
#include "M17Defines.h"
#include "M17CRC.h"

#include <cassert>
#include <cstring>

CM17LICH::CM17LICH() :
m_lich(NULL),
m_valid(false)
{
	m_lich = new unsigned char[M17_LICH_LENGTH_BYTES];
}

CM17LICH::~CM17LICH()
{
	delete[] m_lich;
}

void CM17LICH::getNetwork(unsigned char* data) const
{
	assert(data != NULL);

	::memcpy(data, m_lich, M17_LICH_LENGTH_BYTES);
}

void CM17LICH::setNetwork(const unsigned char* data)
{
	assert(data != NULL);

	::memcpy(m_lich, data, M17_LICH_LENGTH_BYTES);

	m_valid = true;
}

std::string CM17LICH::getSource() const
{
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
	return (m_lich[13U] >> 1) & 0x03U;
}

void CM17LICH::setDataType(unsigned char type)
{
	m_lich[13U] &= 0xF9U;
	m_lich[13U] |= (type << 1) & 0x06U;
}

bool CM17LICH::isNONCENull() const
{
	return ::memcmp(m_lich + 14U, M17_NULL_NONCE, M17_NONCE_LENGTH_BYTES) == 0;
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

	::memcpy(data, m_lich, M17_LICH_LENGTH_BYTES);

	CM17CRC::encodeCRC(data, M17_LICH_LENGTH_BYTES);
}

void CM17LICH::setLinkSetup(const unsigned char* data)
{
	assert(data != NULL);

	::memcpy(m_lich, data, M17_LICH_LENGTH_BYTES);

	m_valid = CM17CRC::checkCRC(m_lich, M17_LICH_LENGTH_BYTES);
}

void CM17LICH::getFragment(unsigned char* data, unsigned int n) const
{
	assert(data != NULL);

	CM17CRC::encodeCRC(m_lich, M17_LICH_LENGTH_BYTES);

	::memcpy(data, m_lich + (n * M17_LICH_FRAGMENT_LENGTH_BYTES), M17_LICH_FRAGMENT_LENGTH_BYTES);
}

void CM17LICH::setFragment(const unsigned char* data, unsigned int n)
{
	assert(data != NULL);

	::memcpy(m_lich + (n * M17_LICH_FRAGMENT_LENGTH_BYTES), data, M17_LICH_FRAGMENT_LENGTH_BYTES);

	m_valid = CM17CRC::checkCRC(m_lich, M17_LICH_LENGTH_BYTES);
}
