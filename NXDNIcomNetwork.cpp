/*
 *   Copyright (C) 2009-2014,2016,2018-2020 by Jonathan Naylor G4KLX
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

#include "NXDNIcomNetwork.h"
#include "NXDNDefines.h"
#include "Defines.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned int BUFFER_LENGTH = 200U;

CNXDNIcomNetwork::CNXDNIcomNetwork(const std::string& localAddress, unsigned int localPort, const std::string& gatewayAddress, unsigned int gatewayPort, bool debug) :
m_socket(localAddress, localPort),
m_addr(),
m_addrLen(0U),
m_debug(debug),
m_enabled(false),
m_buffer(1000U, "NXDN Network")
{
	assert(gatewayPort > 0U);
	assert(!gatewayAddress.empty());

	if (CUDPSocket::lookup(gatewayAddress, gatewayPort, m_addr, m_addrLen) != 0)
		m_addrLen = 0U;
}

CNXDNIcomNetwork::~CNXDNIcomNetwork()
{
}

bool CNXDNIcomNetwork::open()
{
	if (m_addrLen == 0U) {
		LogError("Unable to resolve the address of the NXDN Gateway");
		return false;
	}

	LogMessage("Opening NXDN network connection");

	return m_socket.open(m_addr);
}

bool CNXDNIcomNetwork::write(const unsigned char* data, NXDN_NETWORK_MESSAGE_TYPE type)
{
	assert(data != NULL);

	unsigned char buffer[110U];
	::memset(buffer, 0x00U, 110U);

	buffer[0U] = 'I';
	buffer[1U] = 'C';
	buffer[2U] = 'O';
	buffer[3U] = 'M';
	buffer[4U] = 0x01U;
	buffer[5U] = 0x01U;
	buffer[6U] = 0x08U;
	buffer[7U] = 0xE0U;

	switch (type) {
	case NNMT_VOICE_HEADER:
	case NNMT_VOICE_TRAILER:
		buffer[37U] = 0x23U;
		buffer[38U] = 0x1CU;
		buffer[39U] = 0x21U;
		break;
	case NNMT_VOICE_BODY:
		buffer[37U] = 0x23U;
		buffer[38U] = 0x10U;
		buffer[39U] = 0x21U;
		break;
	case NNMT_DATA_HEADER:
	case NNMT_DATA_BODY:
	case NNMT_DATA_TRAILER:
		buffer[37U] = 0x23U;
		buffer[38U] = 0x02U;
		buffer[39U] = 0x18U;
		break;
	default:
		return false;
	}

	::memcpy(buffer + 40U, data, 33U);

	if (m_debug)
		CUtils::dump(1U, "NXDN Network Data Sent", buffer, 102U);

	return m_socket.write(buffer, 102U, m_addr, m_addrLen);
}

void CNXDNIcomNetwork::clock(unsigned int ms)
{
	unsigned char buffer[BUFFER_LENGTH];

	sockaddr_storage address;
	unsigned int addrLen;
	int length = m_socket.read(buffer, BUFFER_LENGTH, address, addrLen);
	if (length <= 0)
		return;

	if (!CUDPSocket::match(m_addr, address)) {
		LogMessage("NXDN, packet received from an invalid source");
		return;
	}

	if (m_debug)
		CUtils::dump(1U, "NXDN Network Data Received", buffer, length);

	// Invalid packet type?
	if (::memcmp(buffer, "ICOM", 4U) != 0)
		return;

	if (length != 102)
		return;

	if (!m_enabled)
		return;

	m_buffer.addData(buffer + 40U, 33U);
}

bool CNXDNIcomNetwork::read(unsigned char* data)
{
	assert(data != NULL);

	if (m_buffer.isEmpty())
		return false;

	m_buffer.getData(data, 33U);

	return true;
}

void CNXDNIcomNetwork::reset()
{
}

void CNXDNIcomNetwork::close()
{
	m_socket.close();

	LogMessage("Closing NXDN network connection");
}

void CNXDNIcomNetwork::enable(bool enabled)
{
	if (enabled && !m_enabled)
		reset();
	else if (!enabled && m_enabled)
		m_buffer.clear();

	m_enabled = enabled;
}
