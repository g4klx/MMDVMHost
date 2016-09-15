/*
 *   Copyright (C) 2009-2014,2016 by Jonathan Naylor G4KLX
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

#include "P25Network.h"
#include "StopWatch.h"
#include "Defines.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned int BUFFER_LENGTH = 100U;

CP25Network::CP25Network(const std::string& gatewayAddress, unsigned int gatewayPort, unsigned int localPort, bool debug) :
m_socket(localPort),
m_address(),
m_port(gatewayPort),
m_debug(debug),
m_enabled(false),
m_buffer(1000U, "P25 Network"),
m_pollTimer(1000U, 60U)
{
	m_address = CUDPSocket::lookup(gatewayAddress);

	CStopWatch stopWatch;
	::srand(stopWatch.start());
}

CP25Network::~CP25Network()
{
}

bool CP25Network::open()
{
	LogMessage("Opening P25 network connection");

	if (m_address.s_addr == INADDR_NONE)
		return false;

	m_pollTimer.start();

	return m_socket.open();
}

bool CP25Network::writeStart()
{
	return true;
}

bool CP25Network::writeHeader(const unsigned char* header)
{
	assert(header != NULL);
#ifdef notdef
	if (m_debug)
		CUtils::dump(1U, "P25 Network Header Sent", buffer, 49U);

	for (unsigned int i = 0U; i < 2U; i++) {
		bool ret = m_socket.write(buffer, 49U, m_address, m_port);
		if (!ret)
			return false;
	}
#endif
	return true;
}

bool CP25Network::writeLDU1(const unsigned char* ldu1)
{
	assert(ldu1 != NULL);
#ifdef notdef
	unsigned char buffer[30U];

	if (m_debug)
		CUtils::dump(1U, "P25 Network Data Sent", buffer, length + 9U);

	return m_socket.write(buffer, length + 9U, m_address, m_port);
#endif
	return true;
}

bool CP25Network::writeLDU2(const unsigned char* ldu2)
{
	assert(ldu2 != NULL);
#ifdef notdef
	unsigned char buffer[30U];

	if (m_debug)
		CUtils::dump(1U, "P25 Network Data Sent", buffer, length + 9U);

	return m_socket.write(buffer, length + 9U, m_address, m_port);
#endif
	return true;
}

bool CP25Network::writeTerminator(const unsigned char* term)
{
	assert(term != NULL);
#ifdef notdef
	unsigned char buffer[30U];

	if (m_debug)
		CUtils::dump(1U, "P25 Network Data Sent", buffer, length + 9U);

	return m_socket.write(buffer, length + 9U, m_address, m_port);
#endif
	return true;
}

bool CP25Network::writeEnd()
{
	return true;
}

bool CP25Network::writePoll()
{
#ifdef notdef
	// if (m_debug)
	//	CUtils::dump(1U, "P25 Network Poll Sent", buffer, 6U + length);

	return m_socket.write(buffer, 6U + length, m_address, m_port);
#endif
	return true;
}

void CP25Network::clock(unsigned int ms)
{
	m_pollTimer.clock(ms);
	if (m_pollTimer.hasExpired()) {
		writePoll();
		m_pollTimer.start();
	}

	unsigned char buffer[BUFFER_LENGTH];

	in_addr address;
	unsigned int port;
	int length = m_socket.read(buffer, BUFFER_LENGTH, address, port);
	if (length <= 0)
		return;

	// Check if the data is for us
	if (m_address.s_addr != address.s_addr || m_port != port) {
		LogMessage("P25 packet received from an invalid source, %08X != %08X and/or %u != %u", m_address.s_addr, address.s_addr, m_port, port);
		return;
	}

	if (!m_enabled)
		return;

	// Invalid packet type?
	if (::memcmp(buffer, "DSRP", 4U) != 0)
		return;
}

unsigned int CP25Network::read(unsigned char* data, unsigned int length)
{
	assert(data != NULL);

	if (m_buffer.isEmpty())
		return 0U;

	unsigned char c = 0U;
	m_buffer.getData(&c, 1U);

	assert(c <= 100U);
	assert(c <= length);

	unsigned char buffer[100U];
	m_buffer.getData(buffer, c);

	switch (buffer[0U]) {
	case TAG_HEADER:
	case TAG_DATA:
	case TAG_EOT:
		::memcpy(data, buffer, c);
		return c;

	default:
		return 0U;
	}
}

void CP25Network::reset()
{
}

void CP25Network::close()
{
	m_socket.close();

	LogMessage("Closing P25 network connection");
}

void CP25Network::enable(bool enabled)
{
	if (enabled && !m_enabled)
		reset();

	m_enabled = enabled;
}
