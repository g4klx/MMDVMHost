/*
 *   Copyright (C) 2020,2021,2023,2025 by Jonathan Naylor G4KLX
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

#include "M17Network.h"
#include "M17Defines.h"
#include "M17Utils.h"
#include "Defines.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned int BUFFER_LENGTH = 200U;

CM17Network::CM17Network(const std::string& localAddress, unsigned short localPort, const std::string& gatewayAddress, unsigned short gatewayPort, bool debug) :
m_socket(localAddress, localPort),
m_addr(),
m_addrLen(0U),
m_debug(debug),
m_enabled(false),
m_outId(0U),
m_inId(0U),
m_buffer(1000U, "M17 Network"),
m_random(),
m_timer(1000U, 5U)
{
	if (CUDPSocket::lookup(gatewayAddress, gatewayPort, m_addr, m_addrLen) != 0) {
		m_addrLen = 0U;
		return;
	}

	std::random_device rd;
	std::mt19937 mt(rd());
	m_random = mt;
}

CM17Network::~CM17Network()
{
}

bool CM17Network::open()
{
	if (m_addrLen == 0U) {
		LogError("M17, unable to resolve the gateway address");
		return false;
	}

	LogMessage("Opening M17 network connection");

	bool ret = m_socket.open(m_addr);

	if (ret) {
		m_timer.start();
		return true;
	} else {
		return false;
	}
}

bool CM17Network::write(const unsigned char* data)
{
	if (m_addrLen == 0U)
		return false;

	assert(data != nullptr);

	unsigned char buffer[100U];

	buffer[0U] = 'M';
	buffer[1U] = '1';
	buffer[2U] = '7';
	buffer[3U] = ' ';

	// Create a random id for this transmission if needed
	if (m_outId == 0U) {
		std::uniform_int_distribution<uint16_t> dist(0x0001, 0xFFFE);
		m_outId = dist(m_random);
	}

	buffer[4U] = m_outId / 256U;	// Unique session id
	buffer[5U] = m_outId % 256U;

	::memcpy(buffer + 6U, data, 46U);

	// Dummy CRC
	buffer[52U] = 0x00U;
	buffer[53U] = 0x00U;

	if (m_debug)
		CUtils::dump(1U, "M17 Network Transmitted", buffer, 54U);

	return m_socket.write(buffer, 54U, m_addr, m_addrLen);
}

void CM17Network::clock(unsigned int ms)
{
	m_timer.clock(ms);
	if (m_timer.isRunning() && m_timer.hasExpired()) {
		sendPing();
		m_timer.start();
	}

	unsigned char buffer[BUFFER_LENGTH];

	sockaddr_storage address;
	unsigned int addrLen;
	int length = m_socket.read(buffer, BUFFER_LENGTH, address, addrLen);
	if (length <= 0)
		return;

	if (!CUDPSocket::match(m_addr, address)) {
		LogMessage("M17, packet received from an invalid source");
		return;
	}

	if (m_debug)
		CUtils::dump(1U, "M17 Network Received", buffer, length);

	if (!m_enabled)
		return;

	if (::memcmp(buffer + 0U, "PING", 4U) == 0)
		return;

	if (::memcmp(buffer + 0U, "M17 ", 4U) != 0) {
		CUtils::dump(2U, "M17, received unknown packet", buffer, length);
		return;
	}

	uint16_t id = (buffer[4U] << 8) + (buffer[5U] << 0);
	if (m_inId == 0U) {
		m_inId = id;
	} else {
		if (id != m_inId)
			return;
	}

	unsigned char c = length - 6U;
	m_buffer.addData(&c, 1U);

	m_buffer.addData(buffer + 6U, length - 6U);
}

bool CM17Network::read(unsigned char* data)
{
	assert(data != nullptr);

	if (m_buffer.isEmpty())
		return false;

	unsigned char c = 0U;
	m_buffer.getData(&c, 1U);

	m_buffer.getData(data, c);

	return true;
}

void CM17Network::close()
{
	m_socket.close();

	LogMessage("Closing M17 network connection");
}

void CM17Network::reset()
{
	m_outId = 0U;
	m_inId  = 0U;
}

void CM17Network::enable(bool enabled)
{
	if (enabled && !m_enabled)
		reset();
	else if (!enabled && m_enabled)
		m_buffer.clear();

	m_enabled = enabled;
}

bool CM17Network::isConnected() const
{
	return (m_addrLen != 0);
}

void CM17Network::sendPing()
{
	unsigned char buffer[5U];

	buffer[0U] = 'P';
	buffer[1U] = 'I';
	buffer[2U] = 'N';
	buffer[3U] = 'G';

	if (m_debug)
		CUtils::dump(1U, "M17 Network Transmitted", buffer, 4U);

	m_socket.write(buffer, 4U, m_addr, m_addrLen);
}
