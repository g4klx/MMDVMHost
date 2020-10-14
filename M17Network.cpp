/*
 *   Copyright (C) 2009-2014,2016,2019,2020 by Jonathan Naylor G4KLX
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
#include "Defines.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned int BUFFER_LENGTH = 200U;

CM17Network::CM17Network(unsigned int localPort, const std::string& gatewayAddress, unsigned int gatewayPort, bool debug) :
m_socket(localPort),
m_addr(),
m_addrLen(0U),
m_debug(debug),
m_enabled(false),
m_buffer(1000U, "M17 Network")
{
	if (CUDPSocket::lookup(gatewayAddress, gatewayPort, m_addr, m_addrLen) != 0)
		m_addrLen = 0U;
}

CM17Network::~CM17Network()
{
}

bool CM17Network::open()
{
	if (m_addrLen == 0U) {
		LogError("Unable to resolve the address of the M17 Gateway");
		return false;
	}

	LogMessage("Opening M17 network connection");

	return m_socket.open(m_addr);
}

bool CM17Network::write(const unsigned char* data)
{
	assert(data != NULL);

	unsigned char buffer[100U];

	buffer[0U] = 'M';
	buffer[1U] = '1';
	buffer[2U] = '7';

	if (m_debug)
		CUtils::dump(1U, "M17 data transmitted", buffer, 36U);

	return m_socket.write(buffer, 36U, m_addr, m_addrLen);
}

void CM17Network::clock(unsigned int ms)
{
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

	if (!m_enabled)
		return;

	if (m_debug)
		CUtils::dump(1U, "M17 Network Data Received", buffer, length);

	if (::memcmp(buffer + 0U, "M17", 3U) != 0)
		return;

	unsigned char c = length;
	m_buffer.addData(&c, 1U);

	m_buffer.addData(buffer, length);
}

bool CM17Network::read(unsigned char* data)
{
	assert(data != NULL);

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

void CM17Network::enable(bool enabled)
{
	if (!enabled && m_enabled)
		m_buffer.clear();

	m_enabled = enabled;
}
