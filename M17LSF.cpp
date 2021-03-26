/*
 *   Copyright (C) 2020,2021 by Jonathan Naylor G4KLX
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

#include "M17LSF.h"
#include "M17Utils.h"
#include "M17Defines.h"
#include "M17CRC.h"

#include <cassert>
#include <cstring>

CM17LSF::CM17LSF() :
m_lsf(NULL),
m_valid(false)
{
	m_lsf = new unsigned char[M17_LSF_LENGTH_BYTES];
}

CM17LSF::~CM17LSF()
{
	delete[] m_lsf;
}

void CM17LSF::getNetwork(unsigned char* data) const
{
	assert(data != NULL);

	::memcpy(data, m_lsf, M17_LSF_LENGTH_BYTES);
}

void CM17LSF::setNetwork(const unsigned char* data)
{
	assert(data != NULL);

	::memcpy(m_lsf, data, M17_LSF_LENGTH_BYTES);

	m_valid = true;
}

std::string CM17LSF::getSource() const
{
	if (m_lsf[6U] == 0xFFU && m_lsf[7U]  == 0xFFU && m_lsf[8U]  == 0xFFU &&
		m_lsf[9U] == 0xFFU && m_lsf[10U] == 0xFFU && m_lsf[11U] == 0xFFU)
		return "******";

	std::string callsign;
	CM17Utils::decodeCallsign(m_lsf + 6U, callsign);

	return callsign;
}

void CM17LSF::setSource(const std::string& callsign)
{
	CM17Utils::encodeCallsign(callsign, m_lsf + 6U);
}

std::string CM17LSF::getDest() const
{
	if (m_lsf[0U] == 0xFFU && m_lsf[1U] == 0xFFU && m_lsf[2U] == 0xFFU &&
		m_lsf[3U] == 0xFFU && m_lsf[4U] == 0xFFU && m_lsf[5U] == 0xFFU)
		return "******";

	std::string callsign;
	CM17Utils::decodeCallsign(m_lsf + 0U, callsign);

	return callsign;
}

void CM17LSF::setDest(const std::string& callsign)
{
	CM17Utils::encodeCallsign(callsign, m_lsf + 0U);
}

unsigned char CM17LSF::getPacketStream() const
{
	return m_lsf[13U] & 0x01U;
}

void CM17LSF::setPacketStream(unsigned char ps)
{
	m_lsf[13U] &= 0xF7U;
	m_lsf[13U] |= ps & 0x01U;
}

unsigned char CM17LSF::getDataType() const
{
	return (m_lsf[13U] >> 1) & 0x03U;
}

void CM17LSF::setDataType(unsigned char type)
{
	m_lsf[13U] &= 0xF9U;
	m_lsf[13U] |= (type << 1) & 0x06U;
}

unsigned char CM17LSF::getEncryptionType() const
{
	return (m_lsf[13U] >> 3) & 0x03U;
}

void CM17LSF::setEncryptionType(unsigned char type)
{
	m_lsf[13U] &= 0xE7U;
	m_lsf[13U] |= (type << 3) & 0x18U;
}

unsigned char CM17LSF::getEncryptionSubType() const
{
	return (m_lsf[13U] >> 5) & 0x03U;
}

void CM17LSF::setEncryptionSubType(unsigned char type)
{
	m_lsf[13U] &= 0x9FU;
	m_lsf[13U] |= (type << 5) & 0x60U;
}

unsigned char CM17LSF::getCAN() const
{
	return ((m_lsf[12U] << 1) & 0x0EU) | ((m_lsf[13U] >> 7) & 0x01U);
}

void CM17LSF::setCAN(unsigned char can)
{
	m_lsf[13U] &= 0x7FU;
	m_lsf[13U] |= (can << 7) & 0x80U;

	m_lsf[12U] &= 0xF8U;
	m_lsf[12U] |= (can >> 1) & 0x07U;
}

void CM17LSF::reset()
{
	::memset(m_lsf, 0x00U, 30U);

	m_valid = false;
}

bool CM17LSF::isValid() const
{
	return m_valid;
}

void CM17LSF::getLinkSetup(unsigned char* data) const
{
	assert(data != NULL);

	::memcpy(data, m_lsf, M17_LSF_LENGTH_BYTES);

	CM17CRC::encodeCRC16(data, M17_LSF_LENGTH_BYTES);
}

void CM17LSF::setLinkSetup(const unsigned char* data)
{
	assert(data != NULL);

	::memcpy(m_lsf, data, M17_LSF_LENGTH_BYTES);

	m_valid = CM17CRC::checkCRC16(m_lsf, M17_LSF_LENGTH_BYTES);
}

void CM17LSF::getFragment(unsigned char* data, unsigned int n) const
{
	assert(data != NULL);

	CM17CRC::encodeCRC16(m_lsf, M17_LSF_LENGTH_BYTES);

	::memcpy(data, m_lsf + (n * M17_LSF_FRAGMENT_LENGTH_BYTES), M17_LSF_FRAGMENT_LENGTH_BYTES);
}

void CM17LSF::setFragment(const unsigned char* data, unsigned int n)
{
	assert(data != NULL);

	::memcpy(m_lsf + (n * M17_LSF_FRAGMENT_LENGTH_BYTES), data, M17_LSF_FRAGMENT_LENGTH_BYTES);

	m_valid = CM17CRC::checkCRC16(m_lsf, M17_LSF_LENGTH_BYTES);
}
