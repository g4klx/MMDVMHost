/*
 *   Copyright (C) 2009-2014,2016,2018 by Jonathan Naylor G4KLX
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

#include "NXDNDefines.h"
#include "NXDNNetwork.h"
#include "Defines.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned int BUFFER_LENGTH = 200U;

CNXDNNetwork::CNXDNNetwork(const std::string& myAddress, unsigned int myPort, const std::string& gatewayAddress, unsigned int gatewayPort, const std::string& callsign, bool debug) :
m_socket(myAddress, myPort),
m_address(),
m_port(gatewayPort),
m_debug(debug),
m_enabled(false),
m_buffer(1000U, "NXDN Network"),
m_pollTimer(1000U, 5U)
{
	m_address = CUDPSocket::lookup(gatewayAddress);
}

CNXDNNetwork::~CNXDNNetwork()
{
}

bool CNXDNNetwork::open()
{
	LogMessage("Opening NXDN network connection");

	if (m_address.s_addr == INADDR_NONE)
		return false;

	m_pollTimer.start();

	return m_socket.open();
}

bool CNXDNNetwork::write(const unsigned char* data, unsigned short src, bool grp, unsigned short dst, unsigned char cnt, bool end)
{
	assert(data != NULL);

	unsigned char buffer[100U];

	buffer[0U] = 'N';
	buffer[1U] = 'X';
	buffer[2U] = 'D';
	buffer[3U] = 'N';
	buffer[4U] = 'D';

	buffer[5U] = (src >> 8) & 0xFFU;
	buffer[6U] = (src >> 8) & 0xFFU;

	buffer[7U]  = grp ? 0x01U : 0x00U;
	buffer[7U] |= end ? 0x80U : 0x00U;

	buffer[8U] = (dst >> 8) & 0xFFU;
	buffer[9U] = (dst >> 8) & 0xFFU;

	buffer[10U] = cnt;

	::memcpy(buffer + 11U, data, NXDN_FRAME_LENGTH_BYTES);

	if (m_debug)
		CUtils::dump(1U, "NXDN Network Data Sent", buffer, 59U);

	return m_socket.write(buffer, 59U, m_address, m_port);
}

bool CNXDNNetwork::writePoll()
{
	unsigned char buffer[20U];

	buffer[0U] = 'N';
	buffer[1U] = 'X';
	buffer[2U] = 'D';
	buffer[3U] = 'N';
	buffer[4U] = 'P';

	if (m_debug)
		CUtils::dump(1U, "NXDN Network Poll Sent", buffer, 5U);

	return m_socket.write(buffer, 5U, m_address, m_port);
}

void CNXDNNetwork::clock(unsigned int ms)
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
		LogMessage("NXDN packet received from an invalid source, %08X != %08X and/or %u != %u", m_address.s_addr, address.s_addr, m_port, port);
		return;
	}

	// Ignore incoming polls
	if (::memcmp(buffer, "NXDNP", 5U) == 0)
		return;

	// Invalid packet type?
	if (::memcmp(buffer, "NXDND", 5U) != 0)
		return;

	if (!m_enabled)
		return;

	if (m_debug)
		CUtils::dump(1U, "NXDN Network Data Received", buffer, length);

	m_buffer.addData(buffer, 59U);
}

unsigned int CNXDNNetwork::read(unsigned char* data, unsigned short& src, bool& grp, unsigned short& dst, unsigned char& cnt, bool& end)
{
	assert(data != NULL);

	if (m_buffer.isEmpty())
		return 0U;

	unsigned char buffer[100U];
	m_buffer.getData(buffer, 59U);

	src = (buffer[5U] << 8) + buffer[6U];
	grp = (buffer[7U] & 0x01U) == 0x01U;
	end = (buffer[7U] & 0x80U) == 0x80U;
	dst = (buffer[8U] << 8) + buffer[9U];

	cnt = buffer[10U];

	::memcpy(data, buffer + 11U, NXDN_FRAME_LENGTH_BYTES);

	return NXDN_FRAME_LENGTH_BYTES;
}

void CNXDNNetwork::reset()
{
}

void CNXDNNetwork::close()
{
	m_socket.close();

	LogMessage("Closing NXDN network connection");
}

void CNXDNNetwork::enable(bool enabled)
{
	if (enabled && !m_enabled)
		reset();

	m_enabled = enabled;
}
