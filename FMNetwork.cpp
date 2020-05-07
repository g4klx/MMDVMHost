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

#include "FMNetwork.h"
#include "Defines.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned int BUFFER_LENGTH = 500U;

CFMNetwork::CFMNetwork(const std::string& localAddress, unsigned int localPort, const std::string& gatewayAddress, unsigned int gatewayPort, bool debug) :
m_socket(localAddress, localPort),
m_address(),
m_port(gatewayPort),
m_debug(debug),
m_enabled(false),
m_buffer(2000U, "FM Network")
{
	assert(gatewayPort > 0U);
	assert(!gatewayAddress.empty());

	m_address = CUDPSocket::lookup(gatewayAddress);
}

CFMNetwork::~CFMNetwork()
{
}

bool CFMNetwork::open()
{
	LogMessage("Opening FM network connection");

	if (m_address.s_addr == INADDR_NONE)
		return false;

	return m_socket.open();
}

bool CFMNetwork::write(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);

	unsigned char buffer[500U];
	::memset(buffer, 0x00U, 500U);

	buffer[0U] = 'F';
	buffer[1U] = 'M';
	buffer[2U] = 'D';

	::memcpy(buffer + 3U, data, length);

	if (m_debug)
		CUtils::dump(1U, "FM Network Data Sent", buffer, length + 3U);

	return m_socket.write(buffer, length + 3U, m_address, m_port);
}

void CFMNetwork::clock(unsigned int ms)
{
	unsigned char buffer[BUFFER_LENGTH];

	in_addr address;
	unsigned int port;
	int length = m_socket.read(buffer, BUFFER_LENGTH, address, port);
	if (length <= 0)
		return;

	// Check if the data is for us
	if (m_address.s_addr != address.s_addr || port != m_port) {
		LogMessage("FM packet received from an invalid source, %08X != %08X and/or %u != %u", m_address.s_addr, address.s_addr, m_port, port);
		return;
	}

	// Invalid packet type?
	if (::memcmp(buffer, "FMD", 3U) != 0)
		return;

	if (!m_enabled)
		return;

	if (m_debug)
		CUtils::dump(1U, "FM Network Data Received", buffer, length);

	m_buffer.addData(buffer + 3U, length - 3U);
}

unsigned int CFMNetwork::read(unsigned char* data, unsigned int space)
{
	assert(data != NULL);

	unsigned int bytes = m_buffer.dataSize();
	if (bytes == 0U)
		return 0U;

	if (bytes < space)
		space = bytes;

	m_buffer.getData(data, space);

	return space;
}

void CFMNetwork::reset()
{
}

void CFMNetwork::close()
{
	m_socket.close();

	LogMessage("Closing FM network connection");
}

void CFMNetwork::enable(bool enabled)
{
	if (enabled && !m_enabled)
		reset();
	else if (!enabled && m_enabled)
		m_buffer.clear();

	m_enabled = enabled;
}
