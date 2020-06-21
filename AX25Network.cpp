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

CAX25Network::CAX25Network(const std::string& port, unsigned int speed, bool debug) :
m_serial(port, SERIAL_SPEED(speed), false),		// XXX
m_txData(NULL),
m_txLength(0U),
m_txOffset(0U),
m_rxData(NULL),
m_rxLength(0U),
m_rxComplete(false),
m_debug(debug),
m_enabled(false)
{
	assert(!port.empty());
	assert(speed > 0U);

	m_txData = new unsigned char[BUFFER_LENGTH];
	m_rxData = new unsigned char[BUFFER_LENGTH];
}

CAX25Network::~CAX25Network()
{
	delete[] m_txData;
	delete[] m_rxData;
}

bool CAX25Network::open()
{
	LogMessage("Opening AX25 network connection");

	return m_serial.open();
}

bool CAX25Network::write(const unsigned char* data, unsigned int length)
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

void CAX25Network::reset()
{
}

void CAX25Network::close()
{
	m_serial.close();

	LogMessage("Closing AX25 network connection");
}

void CAX25Network::enable(bool enabled)
{
	m_enabled = enabled;
}
