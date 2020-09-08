/*
 *   Copyright (C) 2009-2014,2016,2019 by Jonathan Naylor G4KLX
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

#include "YSFDefines.h"
#include "YSFNetwork.h"
#include "Defines.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned int BUFFER_LENGTH = 200U;

CYSFNetwork::CYSFNetwork(const std::string& myAddress, unsigned int myPort, const std::string& gatewayAddress, unsigned int gatewayPort, const std::string& callsign, bool debug) :
m_socket(myAddress, myPort),
m_address(),
m_port(gatewayPort),
m_callsign(),
m_debug(debug),
m_enabled(false),
m_buffer(1000U, "YSF Network"),
m_pollTimer(1000U, 5U),
m_tag(NULL)
{
	m_callsign = callsign;
	m_callsign.resize(YSF_CALLSIGN_LENGTH, ' ');

	m_address = CUDPSocket::lookup(gatewayAddress);

	m_tag = new unsigned char[YSF_CALLSIGN_LENGTH];
	::memset(m_tag, ' ', YSF_CALLSIGN_LENGTH);
}

CYSFNetwork::~CYSFNetwork()
{
	delete[] m_tag;
}

bool CYSFNetwork::open()
{
	LogMessage("Opening YSF network connection");

	if (m_address.s_addr == INADDR_NONE)
		return false;

	m_pollTimer.start();

	return m_socket.open();
}

bool CYSFNetwork::write(const unsigned char* src, const unsigned char* dest, const unsigned char* data, unsigned int count, bool end)
{
	assert(data != NULL);

	unsigned char buffer[200U];

	buffer[0] = 'Y';
	buffer[1] = 'S';
	buffer[2] = 'F';
	buffer[3] = 'D';

	for (unsigned int i = 0U; i < YSF_CALLSIGN_LENGTH; i++)
		buffer[i + 4U] = m_callsign.at(i);

	if (src != NULL)
		::memcpy(buffer + 14U, src, YSF_CALLSIGN_LENGTH);
	else
		::memset(buffer + 14U, ' ', YSF_CALLSIGN_LENGTH);

	if (dest != NULL)
		::memcpy(buffer + 24U, dest, YSF_CALLSIGN_LENGTH);
	else
		::memset(buffer + 24U, ' ', YSF_CALLSIGN_LENGTH);

	buffer[34U] = end ? 0x01U : 0x00U;
	buffer[34U] |= (count & 0x7FU) << 1;

	::memcpy(buffer + 35U, data, YSF_FRAME_LENGTH_BYTES);

	if (m_debug)
		CUtils::dump(1U, "YSF Network Data Sent", buffer, 155U);

	return m_socket.write(buffer, 155U, m_address, m_port);
}

bool CYSFNetwork::writePoll()
{
	unsigned char buffer[20U];

	buffer[0] = 'Y';
	buffer[1] = 'S';
	buffer[2] = 'F';
	buffer[3] = 'P';

	for (unsigned int i = 0U; i < YSF_CALLSIGN_LENGTH; i++)
		buffer[i + 4U] = m_callsign.at(i);

	if (m_debug)
		CUtils::dump(1U, "YSF Network Poll Sent", buffer, 14U);

	return m_socket.write(buffer, 14U, m_address, m_port);
}

void CYSFNetwork::clock(unsigned int ms)
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
		LogMessage("YSF packet received from an invalid source, %08X != %08X and/or %u != %u", m_address.s_addr, address.s_addr, m_port, port);
		return;
	}

	if (m_debug)
		CUtils::dump(1U, "YSF Network Data Received", buffer, length);

	// Invalid packet type?
	if (::memcmp(buffer, "YSFD", 4U) != 0)
		return;

	if (!m_enabled)
		return;

	if (::memcmp(m_tag, "          ", YSF_CALLSIGN_LENGTH) == 0) {
		::memcpy(m_tag, buffer + 4U, YSF_CALLSIGN_LENGTH);
	} else {
		if (::memcmp(m_tag, buffer + 4U, YSF_CALLSIGN_LENGTH) != 0)
			return;
	}

	bool end = (buffer[34U] & 0x01U) == 0x01U;
	if (end)
		::memset(m_tag, ' ', YSF_CALLSIGN_LENGTH);

	m_buffer.addData(buffer, 155U);
}

unsigned int CYSFNetwork::read(unsigned char* data)
{
	assert(data != NULL);

	if (m_buffer.isEmpty())
		return 0U;

	m_buffer.getData(data, 155U);

	return 155U;
}

void CYSFNetwork::reset()
{
	::memset(m_tag, ' ', YSF_CALLSIGN_LENGTH);
}

void CYSFNetwork::close()
{
	m_socket.close();

	LogMessage("Closing YSF network connection");
}

void CYSFNetwork::enable(bool enabled)
{
	if (enabled && !m_enabled)
		reset();
	else if (!enabled && m_enabled)
		m_buffer.clear();

	m_enabled = enabled;
}
