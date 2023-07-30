/*
 *   Copyright (C) 2020,2023 by Jonathan Naylor G4KLX
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
#include "MQTTConnection.h"
#include "Utils.h"
#include "Log.h"

#if defined(USE_AX25)

#include <cstdio>
#include <cassert>
#include <cstring>

// In Log.cpp
extern CMQTTConnection* m_mqtt;

CAX25Network::CAX25Network(bool debug) :
m_buffer(1000U, "AX.25 buffer"),
m_mutex(),
m_debug(debug),
m_enabled(false)
{
}

CAX25Network::~CAX25Network()
{
}

bool CAX25Network::open()
{
	LogMessage("Opening AX.25 network connection");

	return true;
}

bool CAX25Network::write(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);
	assert(m_mqtt != NULL);

	if (!m_enabled)
		return true;

	unsigned char txData[500U];
	unsigned int txLength = 0U;

	txData[txLength++] = AX25_FEND;
	txData[txLength++] = AX25_KISS_DATA;

	for (unsigned int i = 0U; i < length; i++) {
		unsigned char c = data[i];

		switch (c) {
		case AX25_FEND:
			txData[txLength++] = AX25_FESC;
			txData[txLength++] = AX25_TFEND;
			break;
		case AX25_FESC:
			txData[txLength++] = AX25_FESC;
			txData[txLength++] = AX25_TFESC;
			break;
		default:
			txData[txLength++] = c;
			break;
		}
	}

	txData[txLength++] = AX25_FEND;

	if (m_debug)
		CUtils::dump(1U, "AX.25 network KISS packet sent", txData, txLength);

	m_mqtt->publish("ax25-out", txData, txLength);

	return true;
}

unsigned int CAX25Network::read(unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	if (m_buffer.isEmpty())
		return 0U;

	unsigned char rxData[500U];
	unsigned int rxLength = 0U;

	m_mutex.lock();

	m_buffer.getData((unsigned char*)&rxLength, sizeof(unsigned int));
	m_buffer.getData(rxData, rxLength);

	m_mutex.unlock();

	if (m_debug)
		CUtils::dump(1U, "AX.25 network KISS packet received", rxData, rxLength);

	if (rxData[0U] != AX25_FEND) {
		LogWarning("Missing FEND at start of a KISS frame - 0x%02X", rxData[0U]);
		return 0U;
	}
	
	if (rxData[1U] != AX25_KISS_DATA) {
		LogWarning("Invalid KISS type byte received - 0x%02X", rxData[1U]);
		return 0U;
	}

	bool complete = false;

	length = 0U;
	unsigned char lastChar = 0x00U;
	for (unsigned int i = 2U; i < rxLength; i++) {
		unsigned char c = rxData[i];

		if (c == AX25_FEND) {
			complete = true;
			break;
		} else if (c == AX25_TFEND && lastChar == AX25_FESC) {
			data[length++] = AX25_FEND;
		} else if (c == AX25_TFESC && lastChar == AX25_FESC) {
			data[length++] = AX25_FESC;
		} else {
			data[length++] = c;
		}

		lastChar = c;
	}

	if (!complete)
		return 0U;

	return length;
}

void CAX25Network::setData(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	m_mutex.lock();

	m_buffer.addData((unsigned char*)&length, sizeof(unsigned int));
	m_buffer.addData(data, length);

	m_mutex.unlock();
}

void CAX25Network::reset()
{
}

void CAX25Network::close()
{
	LogMessage("Closing AX.25 network connection");
}

void CAX25Network::enable(bool enabled)
{
	m_enabled = enabled;

	if (enabled != m_enabled)
		m_buffer.clear();
}

#endif

