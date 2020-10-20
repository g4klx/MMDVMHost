/*
 *   Copyright (C) 2009-2014,2016,2019,2020 by Jonathan Naylor G4KLX
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

#include "M17Network.h"
#include "M17Defines.h"
#include "M17Utils.h"
#include "Defines.h"
#include "M17CRC.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned int BUFFER_LENGTH = 200U;

CM17Network::CM17Network(const std::string& callsign, unsigned int port, bool debug) :
m_socket(port),
m_addr(),
m_addrLen(0U),
m_debug(debug),
m_enabled(false),
m_outId(0U),
m_inId(0U),
m_buffer(1000U, "M17 Network"),
m_random(),
m_state(M17N_NOTLINKED),
m_encoded(NULL),
m_module(' '),
m_timer(1000U, 5U)
{
	std::random_device rd;
	std::mt19937 mt(rd());
	m_random = mt;

	m_encoded = new unsigned char[6U];

	std::string call = callsign;
	call.resize(8U, ' ');
	call += "D";

	CM17Utils::encodeCallsign(call, m_encoded);
}

CM17Network::~CM17Network()
{
	delete[] m_encoded;
}

bool CM17Network::open()
{
	LogMessage("Opening M17 network connection");

	return m_socket.open(m_addr);
}

bool CM17Network::link(const std::string& address, unsigned int port, char module)
{
	if (CUDPSocket::lookup(address, port, m_addr, m_addrLen) != 0) {
		m_state = M17N_NOTLINKED;
		return false;
	}

	m_module = module;

	m_state = M17N_LINKING;

	sendConnect();

	m_timer.start();

	return true;
}

void CM17Network::unlink()
{
	if (m_state != M17N_LINKED)
		return;

	m_state = M17N_UNLINKING;

	sendConnect();

	m_timer.start();
}

bool CM17Network::write(const unsigned char* data)
{
	if (m_state != M17N_LINKED)
		return false;

	assert(data != NULL);

	unsigned char buffer[100U];

	buffer[0U] = 'M';
	buffer[1U] = '1';
	buffer[2U] = '7';
	buffer[3U] = ' ';

	// Create a random id for this transmission if needed
	if (m_outId == 0U) {
		std::uniform_int_distribution<uint16_t> dist(0x0001, 0xFFFE);
		m_outId = dist(m_random);
	}

	buffer[4U] = m_outId / 256U;	// Unique session id
	buffer[5U] = m_outId % 256U;

	::memcpy(buffer + 6U, data, 46U);

	// Add the CRC
	CM17CRC::encodeCRC(buffer + 6U, M17_LICH_LENGTH_BYTES + M17_FN_LENGTH_BYTES + M17_PAYLOAD_LENGTH_BYTES + M17_CRC_LENGTH_BYTES);

	if (m_debug)
		CUtils::dump(1U, "M17 data transmitted", buffer, 54U);

	return m_socket.write(buffer, 54U, m_addr, m_addrLen);
}

void CM17Network::clock(unsigned int ms)
{
	m_timer.clock(ms);
	if (m_timer.isRunning() && m_timer.hasExpired()) {
		switch (m_state) {
		case M17N_LINKING:
			sendConnect();
			m_timer.start();
			break;
		case M17N_UNLINKING:
			sendDisconnect();
			m_timer.start();
			break;
		default:
			m_timer.stop();
			break;
		}
	}

	unsigned char buffer[BUFFER_LENGTH];

	sockaddr_storage address;
	unsigned int addrLen;
	int length = m_socket.read(buffer, BUFFER_LENGTH, address, addrLen);
	if (length <= 0)
		return;

	if (!CUDPSocket::match(m_addr, address)) {
		LogMessage("M17, packet received from an invalid source");
		return;
	}

	if (m_debug)
		CUtils::dump(1U, "M17 Network Data Received", buffer, length);

	if (::memcmp(buffer + 0U, "ACKN", 4U) == 0) {
		m_timer.stop();
		m_state = M17N_LINKED;
		LogMessage("M17, linked to reflector");
		return;
	}

	if (::memcmp(buffer + 0U, "NACK", 4U) == 0) {
		m_timer.stop();
		m_state = M17N_NOTLINKED;
		LogMessage("M17, link refused by reflector");
		return;
	}

	if (::memcmp(buffer + 0U, "DISC", 4U) == 0) {
		m_timer.stop();
		m_state = M17N_NOTLINKED;
		LogMessage("M17, unlinked from reflector");
		return;
	}

	if (::memcmp(buffer + 0U, "PING", 4U) == 0) {
		if (m_state == M17N_LINKED)
			sendPong();
		return;
	}

	if (!m_enabled)
		return;

	if (::memcmp(buffer + 0U, "M17 ", 4U) != 0) {
		CUtils::dump(2U, "M17, received unknown packet", buffer, length);
		return;
	}

	uint16_t id = (buffer[4U] << 8) + (buffer[5U] << 0);
	if (m_inId == 0U) {
		m_inId = id;
	} else {
		if (id != m_inId)
			return;
	}

	// Check the CRC
	bool valid = CM17CRC::checkCRC(buffer + 6U, M17_LICH_LENGTH_BYTES + M17_FN_LENGTH_BYTES + M17_PAYLOAD_LENGTH_BYTES + M17_CRC_LENGTH_BYTES);
	if (!valid) {
		LogMessage("M17, network packet received with an invalid CRC");
		return;
	}

	unsigned char c = length - 6U;
	m_buffer.addData(&c, 1U);

	m_buffer.addData(buffer + 6U, length - 8U);
}

bool CM17Network::read(unsigned char* data)
{
	assert(data != NULL);

	if (m_buffer.isEmpty())
		return false;

	unsigned char c = 0U;
	m_buffer.getData(&c, 1U);

	m_buffer.getData(data, c);

	return true;
}

void CM17Network::close()
{
	m_socket.close();

	LogMessage("Closing M17 network connection");
}

void CM17Network::reset()
{
	m_outId = 0U;
	m_inId  = 0U;
}

void CM17Network::enable(bool enabled)
{
	if (!enabled && m_enabled)
		m_buffer.clear();

	m_enabled = enabled;
}

void CM17Network::sendConnect()
{
	unsigned char buffer[15U];

	buffer[0U] = 'C';
	buffer[1U] = 'O';
	buffer[2U] = 'N';
	buffer[3U] = 'N';

	buffer[4U] = m_encoded[0U];
	buffer[5U] = m_encoded[1U];
	buffer[6U] = m_encoded[2U];
	buffer[7U] = m_encoded[3U];
	buffer[8U] = m_encoded[4U];
	buffer[9U] = m_encoded[5U];

	buffer[10U] = m_module;

	if (m_debug)
		CUtils::dump(1U, "M17 data transmitted", buffer, 11U);

	m_socket.write(buffer, 11U, m_addr, m_addrLen);
}

void CM17Network::sendPong()
{
	unsigned char buffer[15U];

	buffer[0U] = 'P';
	buffer[1U] = 'O';
	buffer[2U] = 'N';
	buffer[3U] = 'G';

	buffer[4U] = m_encoded[0U];
	buffer[5U] = m_encoded[1U];
	buffer[6U] = m_encoded[2U];
	buffer[7U] = m_encoded[3U];
	buffer[8U] = m_encoded[4U];
	buffer[9U] = m_encoded[5U];

	if (m_debug)
		CUtils::dump(1U, "M17 data transmitted", buffer, 10U);

	m_socket.write(buffer, 10U, m_addr, m_addrLen);
}

void CM17Network::sendDisconnect()
{
	unsigned char buffer[15U];

	buffer[0U] = 'D';
	buffer[1U] = 'I';
	buffer[2U] = 'S';
	buffer[3U] = 'C';

	buffer[4U] = m_encoded[0U];
	buffer[5U] = m_encoded[1U];
	buffer[6U] = m_encoded[2U];
	buffer[7U] = m_encoded[3U];
	buffer[8U] = m_encoded[4U];
	buffer[9U] = m_encoded[5U];

	if (m_debug)
		CUtils::dump(1U, "M17 data transmitted", buffer, 10U);

	m_socket.write(buffer, 10U, m_addr, m_addrLen);
}
