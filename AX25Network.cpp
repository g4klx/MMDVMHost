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
#include "AX25Defines.h"
#include "Defines.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned int BUFFER_LENGTH = 500U;


CAX25Network::CAX25Network(const std::string& port, unsigned int speed, bool debug) :
m_serial(port, speed, false),
m_txData(NULL),
m_rxData(NULL),
m_rxLength(0U),
m_rxLastChar(0U),
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

	if (!m_enabled)
		return true;

	unsigned int txLength = 0U;

	m_txData[txLength++] = AX25_FEND;
	m_txData[txLength++] = AX25_KISS_DATA;

	for (unsigned int i = 0U; i < length; i++) {
		unsigned char c = data[i];

		switch (c) {
		case AX25_FEND:
			m_txData[txLength++] = AX25_FESC;
			m_txData[txLength++] = AX25_TFEND;
			break;
		case AX25_FESC:
			m_txData[txLength++] = AX25_FESC;
			m_txData[txLength++] = AX25_TFESC;
			break;
		default:
			m_txData[txLength++] = c;
			break;
		}
	}

	m_txData[txLength++] = AX25_FEND;

	if (m_debug)
		CUtils::dump(1U, "AX25 Network Data Sent", m_txData, txLength);

	return m_serial.write(m_txData, txLength);
}

unsigned int CAX25Network::read(unsigned char* data, unsigned int length)
{
	assert(data != NULL);

	bool complete = false;

	unsigned char c;
	while (m_serial.read(&c, 1U) > 0) {
		if (m_rxLength == 0U && c == AX25_FEND)
			m_rxData[m_rxLength++] = c;
		else if (m_rxLength > 0U)
			m_rxData[m_rxLength++] = c;

		if (m_rxLength > 1U && c == AX25_FEND) {
			complete = true;
			break;
		}
	}

	if (!m_enabled)
		return 0U;

	if (!complete)
		return 0U;

	if (m_rxLength == 0U)
		return 0U;

	if (m_rxData[1U] != AX25_KISS_DATA) {
		m_rxLength = 0U;
		return 0U;
	}

	complete = false;

	unsigned int dataLen = 0U;
	for (unsigned int i = 2U; i < m_rxLength; i++) {
		unsigned char c = m_rxData[i];

		if (c == AX25_FEND) {
			complete = true;
			break;
		} else if (c == AX25_TFEND && m_rxLastChar == AX25_FESC) {
			data[dataLen++] = AX25_FEND;
		} else if (c == AX25_TFESC && m_rxLastChar == AX25_FESC) {
			data[dataLen++] = AX25_FESC;
		} else {
			data[dataLen++] = c;
		}

		m_rxLastChar = c;
	}

	if (!complete)
		return 0U;

	if (m_debug)
		CUtils::dump(1U, "AX25 Network Data Received", m_rxData, m_rxLength);

	m_rxLength   = 0U;
	m_rxLastChar = 0U;

	return dataLen;
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

	if (enabled != m_enabled) {
		m_rxLastChar = 0U;
		m_rxLength   = 0U;
	}
}
