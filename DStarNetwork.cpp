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

#include "DStarDefines.h"
#include "DStarNetwork.h"
#include "StopWatch.h"
#include "Defines.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>

const unsigned int BUFFER_LENGTH = 100U;

CDStarNetwork::CDStarNetwork(const std::string& gatewayAddress, unsigned int gatewayPort, unsigned int localPort, bool debug) :
m_socket("", localPort),
m_address(),
m_port(gatewayPort),
m_debug(debug),
m_enabled(false),
m_outId(0U),
m_outSeq(0U),
m_inId(0U),
m_buffer(1000U)
{
	m_address = CUDPSocket::lookup(gatewayAddress);

	CStopWatch stopWatch;
	::srand(stopWatch.start());
}

CDStarNetwork::~CDStarNetwork()
{
}

bool CDStarNetwork::open()
{
	LogMessage("Opening D-Star network connection");

	if (m_address.s_addr == INADDR_NONE)
		return false;

	return m_socket.open();
}

bool CDStarNetwork::writeHeader(const unsigned char* header, unsigned int length)
{
	assert(header != NULL);

	unsigned char buffer[50U];

	buffer[0] = 'D';
	buffer[1] = 'S';
	buffer[2] = 'R';
	buffer[3] = 'P';

	buffer[4] = 0x20U;

	// Create a random id for this transmission
	m_outId = (::rand() % 65535U) + 1U;

	buffer[5] = m_outId / 256U;	// Unique session id
	buffer[6] = m_outId % 256U;

	buffer[7] = 0U;

	::memcpy(buffer + 8U, header, length);

	m_outSeq = 0U;

	if (m_debug)
		CUtils::dump(1U, "D-Star Transmitted", buffer, 49U);

	for (unsigned int i = 0U; i < 2U; i++) {
		bool ret = m_socket.write(buffer, 49U, m_address, m_port);
		if (!ret)
			return false;
	}

	return true;
}

bool CDStarNetwork::writeData(const unsigned char* data, unsigned int length, unsigned int errors, bool end)
{
	assert(data != NULL);

	unsigned char buffer[30U];

	buffer[0] = 'D';
	buffer[1] = 'S';
	buffer[2] = 'R';
	buffer[3] = 'P';

	buffer[4] = 0x21U;

	buffer[5] = m_outId / 256U;	// Unique session id
	buffer[6] = m_outId % 256U;

	// If this is a data sync, reset the sequence to zero
	if (data[9] == 0x55 && data[10] == 0x2D && data[11] == 0x16)
		m_outSeq = 0U;

	buffer[7] = m_outSeq;
	if (end)
		buffer[7] |= 0x40U;			// End of data marker

	buffer[8] = errors;

	m_outSeq++;
	if (m_outSeq > 0x14U)
		m_outSeq = 0U;

	::memcpy(buffer + 9U, data, length);

	if (m_debug)
		CUtils::dump(1U, "D-Star Transmitted", buffer, length + 9U);

	return m_socket.write(buffer, length + 9U, m_address, m_port);
}

bool CDStarNetwork::writeBusyHeader(const unsigned char* header, unsigned int length)
{
	assert(header != NULL);

	unsigned char buffer[50U];

	buffer[0] = 'D';
	buffer[1] = 'S';
	buffer[2] = 'R';
	buffer[3] = 'P';

	buffer[4] = 0x22U;

	// Create a random id for this header
	m_outId = (::rand() % 65535U) + 1U;

	buffer[5] = m_outId / 256U;		// Unique session id
	buffer[6] = m_outId % 256U;

	buffer[7] = 0U;

	::memcpy(buffer + 8U, header, length);

	m_outSeq = 0U;

	if (m_debug)
		CUtils::dump(1U, "D-Star Transmitted", buffer, 49U);

	return m_socket.write(buffer, 49U, m_address, m_port);
}

bool CDStarNetwork::writeBusyData(const unsigned char* data, unsigned int length, unsigned int errors, bool end)
{
	assert(data != NULL);

	unsigned char buffer[30U];

	buffer[0] = 'D';
	buffer[1] = 'S';
	buffer[2] = 'R';
	buffer[3] = 'P';

	buffer[4] = 0x23U;

	buffer[5] = m_outId / 256U;	// Unique session id
	buffer[6] = m_outId % 256U;

	// If this is a data sync, reset the sequence to zero
	if (data[9] == 0x55 && data[10] == 0x2D && data[11] == 0x16)
		m_outSeq = 0U;

	buffer[7] = m_outSeq;
	if (end)
		buffer[7] |= 0x40U;			// End of data marker

	buffer[8] = errors;

	m_outSeq++;
	if (m_outSeq > 0x14U)
		m_outSeq = 0U;

	::memcpy(buffer + 9U, data, length);

	if (m_debug)
		CUtils::dump(1U, "D-Star Transmitted", buffer, length + 9U);

	return m_socket.write(buffer, length + 9U, m_address, m_port);
}

bool CDStarNetwork::writePoll(const std::string& text)
{
	unsigned char buffer[40U];

	buffer[0] = 'D';
	buffer[1] = 'S';
	buffer[2] = 'R';
	buffer[3] = 'P';

	buffer[4] = 0x0A;				// Poll with text

	unsigned int length = text.length();

	for (unsigned int i = 0U; i < length; i++)
		buffer[5U + i] = text.at(i);

	buffer[5U + length] = 0x00;

	if (m_debug)
		CUtils::dump(1U, "D-Star Transmitted", buffer, 6U + length);

	return m_socket.write(buffer, 6U + length, m_address, m_port);
}

void CDStarNetwork::clock(unsigned int ms)
{
	unsigned char buffer[BUFFER_LENGTH];

	in_addr address;
	unsigned int port;
	int length = m_socket.read(buffer, BUFFER_LENGTH, address, port);
	if (length <= 0)
		return;

	if (m_debug)
		CUtils::dump(1U, "D-Star Received", buffer, length);

	// Check if the data is for us
	if (m_address.s_addr != address.s_addr || m_port != port) {
		LogMessage("D-Star packet received from an invalid source, %08X != %08X and/or %u != %u", m_address.s_addr, address.s_addr, m_port, port);
		return;
	}

	// Invalid packet type?
	if (::memcmp(buffer, "DSRP", 4U) != 0)
		return;

	switch (buffer[4]) {
	case 0x00U:			// NETWORK_TEXT;
	case 0x01U:			// NETWORK_TEMPTEXT;
	case 0x04U:			// NETWORK_STATUS1..5
	case 0x24U:			// NETWORK_DD_DATA
		return;

	case 0x20U:			// NETWORK_HEADER
		if (m_inId == 0U && m_enabled) {
			m_inId = buffer[5] * 256U + buffer[6];

			unsigned char c = length - 6U;
			m_buffer.addData(&c, 1U);

			c = TAG_HEADER;
			m_buffer.addData(&c, 1U);

			m_buffer.addData(buffer + 7U, length - 7U);
		}
		break;

	case 0x21U:			// NETWORK_DATA
		if (m_enabled) {
			uint16_t id = buffer[5] * 256U + buffer[6];

			// Check that the stream id matches the valid header, reject otherwise
			if (id == m_inId && m_enabled) {
				unsigned char c = length - 8U;
				m_buffer.addData(&c, 1U);

				// Is this the last packet in the stream?
				if ((buffer[7] & 0x40) == 0x40) {
					m_inId = 0U;
					c = TAG_EOT;
				} else {
					c = TAG_DATA;
				}
				m_buffer.addData(&c, 1U);

				m_buffer.addData(buffer + 9U, length - 9U);
			}
		}
		break;

	default:
		CUtils::dump("Unknown D-Star packet from the Gateway", buffer, length);
		break;
	}
}

unsigned int CDStarNetwork::read(unsigned char* data, unsigned int length)
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

void CDStarNetwork::reset()
{
	m_inId = 0U;
}

void CDStarNetwork::close()
{
	m_socket.close();

	LogMessage("Closing D-Star network connection");
}

void CDStarNetwork::enable(bool enabled)
{
	m_enabled = enabled;
}
