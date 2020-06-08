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

#include "AX25Network.h"
#include "Defines.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned int BUFFER_LENGTH = 200U;

CAX25Network::CAX25Network(const std::string& localAddress, unsigned int localPort, const std::string& gatewayAddress, unsigned int gatewayPort, bool debug) :
m_socket(localAddress, localPort),
m_address(),
m_port(gatewayPort),
m_debug(debug),
m_enabled(false)
{
	assert(gatewayPort > 0U);
	assert(!gatewayAddress.empty());

	m_address = CUDPSocket::lookup(gatewayAddress);
}

CAX25Network::~CAX25Network()
{
}

bool CAX25Network::open()
{
	LogMessage("Opening AX25 network connection");

	if (m_address.s_addr == INADDR_NONE)
		return false;

	return m_socket.open();
}

bool CAX25Network::writeAX25(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);

	unsigned char buffer[110U];
	::memset(buffer, 0x00U, 110U);

	buffer[0U] = 'A';
	buffer[1U] = 'X';
	buffer[2U] = '2';
	buffer[3U] = '5';

	::memcpy(buffer + 4U, data, length);

	if (m_debug)
		CUtils::dump(1U, "AX25 Network Data Sent", buffer, length + 4U);

	return m_socket.write(buffer, length + 4U, m_address, m_port);
}

bool CAX25Network::writeMICE(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);

	unsigned char buffer[110U];
	::memset(buffer, 0x00U, 110U);

	buffer[0U] = 'M';
	buffer[1U] = 'I';
	buffer[2U] = 'C';
	buffer[3U] = 'E';

	::memcpy(buffer + 4U, data, length);

	if (m_debug)
		CUtils::dump(1U, "AX25 Network Data Sent", buffer, length + 4U);

	return m_socket.write(buffer, length + 4U, m_address, m_port);
}

void CAX25Network::reset()
{
}

void CAX25Network::close()
{
	m_socket.close();

	LogMessage("Closing AX25 network connection");
}

void CAX25Network::enable(bool enabled)
{
	m_enabled = enabled;
}
