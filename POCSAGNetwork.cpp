/*
 *   Copyright (C) 2018 by Jonathan Naylor G4KLX
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

#include "POCSAGDefines.h"
#include "POCSAGNetwork.h"
#include "Defines.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned int BUFFER_LENGTH = 200U;

CPOCSAGNetwork::CPOCSAGNetwork(const std::string& myAddress, unsigned int myPort, const std::string& gatewayAddress, unsigned int gatewayPort, bool debug) :
m_socket(myAddress, myPort),
m_address(),
m_port(gatewayPort),
m_debug(debug),
m_enabled(false),
m_buffer(1000U, "POCSAG Network")
{
	m_address = CUDPSocket::lookup(gatewayAddress);
}

CPOCSAGNetwork::~CPOCSAGNetwork()
{
}

bool CPOCSAGNetwork::open()
{
	LogMessage("Opening POCSAG network connection");

	if (m_address.s_addr == INADDR_NONE)
		return false;

	return m_socket.open();
}

void CPOCSAGNetwork::clock(unsigned int ms)
{
	unsigned char buffer[BUFFER_LENGTH];

	in_addr address;
	unsigned int port;
	int length = m_socket.read(buffer, BUFFER_LENGTH, address, port);
	if (length <= 0)
		return;

	// Check if the data is for us
	if (m_address.s_addr != address.s_addr || m_port != port) {
		LogMessage("POCSAG packet received from an invalid source, %08X != %08X and/or %u != %u", m_address.s_addr, address.s_addr, m_port, port);
		return;
	}

	// Invalid packet type?
	if (::memcmp(buffer, "POCSAG", 6U) != 0)
		return;

	if (!m_enabled)
		return;

	if (m_debug)
		CUtils::dump(1U, "POCSAG Network Data Received", buffer, length);

	unsigned char len = length - 6U;
	m_buffer.addData(&len, 1U);
	m_buffer.addData(buffer + 6U, length - 6U);
}

unsigned int CPOCSAGNetwork::read(unsigned char* data)
{
	assert(data != NULL);

	if (m_buffer.isEmpty())
		return 0U;

	unsigned char len = 0U;
	m_buffer.getData(&len, 1U);
	m_buffer.getData(data, len);

	return len;
}

void CPOCSAGNetwork::reset()
{
}

void CPOCSAGNetwork::close()
{
	m_socket.close();

	LogMessage("Closing POCSAG network connection");
}

void CPOCSAGNetwork::enable(bool enabled)
{
	if (enabled && !m_enabled)
		reset();

	unsigned char c = enabled ? 0x00U : 0xFFU;
	
	m_socket.write(&c, 1U, m_address, m_port);
	
	m_enabled = enabled;
}
