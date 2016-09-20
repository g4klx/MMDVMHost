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
m_buffer(1000U, "P25 Network")
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

	return m_socket.open();
}

bool CP25Network::writeHeader(unsigned int tgid)
{
	unsigned char buffer[30U];

	// The '00' record
	::memset(buffer, 0x00U, 10U);
	buffer[0U] = 0x00U;
	buffer[1U] = 0x02U;
	buffer[2U] = 0x02U;			// RT mode enabled
	buffer[3U] = 0x0CU;			// Start
	buffer[4U] = 0x0BU;			// Voice

	if (m_debug)
		CUtils::dump(1U, "P25 Network ICW Sent", buffer, 10U);

#ifdef notdef
	bool ret = m_socket.write(buffer, 10U, m_address, m_port);
	if (!ret)
		return false;
#endif

	// The '60' record
	::memset(buffer, 0x00U, 30U);
	buffer[0U] = 0x60U;
	buffer[1U] = 0x02U;
	buffer[2U] = 0x02U;			// RT mode enabled
	buffer[3U] = 0x0CU;			// Start
	buffer[4U] = 0x0BU;			// Voice
	buffer[5U] = 0x1BU;			// Quantar
	buffer[6U] = 0x00U;			// LDU1 RSSI
	buffer[7U] = 0x00U;			// 1A Flag, no RSSI or MM
	buffer[8U] = 0x00U;			// LDU1 RSSI
	buffer[23U] = 0x08U;
	buffer[28U] = 0x02U;
	buffer[29U] = 0x36U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network VHDR1 Sent", buffer, 30U);

#ifdef notdef
	bool ret = m_socket.write(buffer, 30U, m_address, m_port);
	if (!ret)
		return false;
#endif

	// The '61' record
	::memset(buffer, 0x00U, 22U);
	buffer[0U] = 0x61U;
	buffer[1U] = (tgid << 8) & 0xFFU;
	buffer[2U] = (tgid << 0) & 0xFFU;
	buffer[21U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network VHDR2 Sent", buffer, 22U);

#ifdef notdef
	bool ret = m_socket.write(buffer, 22U, m_address, m_port);
	if (!ret)
		return false;
#endif

	return true;
}

bool CP25Network::writeLDU1(const unsigned char* ldu1)
{
	assert(ldu1 != NULL);

	unsigned char buffer[22U];

	// The '62' record
	::memset(buffer, 0x00U, 10U);
	buffer[0U] = 0x62U;
	buffer[1U] = 0x02U;
	buffer[2U] = 0x02U;			// RT mode enabled
	buffer[3U] = 0x0CU;			// Start
	buffer[4U] = 0x0BU;			// Voice
	buffer[5U] = 0x1BU;			// Quantar
	buffer[6U] = 0x00U;			// LDU1 RSSI
	buffer[7U] = 0x00U;			// 1A Flag, no RSSI or MM
	buffer[8U] = 0x00U;			// LDU1 RSSI
	buffer[9U] = 0x00U;			// Adj MM
	buffer[21U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU1 Sent", buffer, 22U);

#ifdef notdef
	bool ret = m_socket.write(buffer, 22U, m_address, m_port);
	if (!ret)
		return false;
#endif

	return true;
}

bool CP25Network::writeLDU2(const unsigned char* ldu2)
{
	assert(ldu2 != NULL);

	unsigned char buffer[22U];

	// The '6B' record
	::memset(buffer, 0x00U, 10U);
	buffer[0U] = 0x6BU;
	buffer[1U] = 0x02U;
	buffer[2U] = 0x02U;			// RT mode enabled
	buffer[3U] = 0x0CU;			// Start
	buffer[4U] = 0x0BU;			// Voice
	buffer[5U] = 0x1BU;			// Quantar
	buffer[6U] = 0x00U;			// LDU1 RSSI
	buffer[7U] = 0x00U;			// 1A Flag, no RSSI or MM
	buffer[8U] = 0x00U;			// LDU1 RSSI
	buffer[9U] = 0x00U;			// Adj MM
	buffer[21U] = 0x02U;

	if (m_debug)
		CUtils::dump(1U, "P25 Network LDU2 Sent", buffer, 22U);

#ifdef notdef
	bool ret = m_socket.write(buffer, 22U, m_address, m_port);
	if (!ret)
		return false;
#endif

	return true;
}

bool CP25Network::writeTerminator(const unsigned char* term)
{
	assert(term != NULL);

	unsigned char buffer[30U];

	// The '00' record
	::memset(buffer, 0x00U, 10U);
	buffer[0U] = 0x00U;
	buffer[1U] = 0x02U;
	buffer[2U] = 0x02U;			// RT mode enabled
	buffer[3U] = 0x25U;			// End
	buffer[4U] = 0x0BU;			// Voice

	if (m_debug)
		CUtils::dump(1U, "P25 Network ICW Sent", buffer, 10U);

#ifdef notdef
	bool ret = m_socket.write(buffer, 10U, m_address, m_port);
	if (!ret)
		return false;

	ret = m_socket.write(buffer, 10U, m_address, m_port);
	if (!ret)
		return false;
#endif

	return true;
}

void CP25Network::clock(unsigned int ms)
{
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

	unsigned char c = length;
	m_buffer.addData(&c, 1U);

	m_buffer.addData(buffer, length);
}

unsigned int CP25Network::read(unsigned char* data, unsigned int length)
{
	assert(data != NULL);

	if (m_buffer.isEmpty())
		return 0U;

	unsigned char c = 0U;
	m_buffer.getData(&c, 1U);

	assert(c <= length);

	m_buffer.getData(data, c);

	return c;
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
