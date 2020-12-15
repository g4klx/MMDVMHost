/*
*   Copyright (C) 2016 by Jonathan Naylor G4KLX
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

#include "Defines.h"
#include "Utils.h"
#include "Log.h"
#include "UMP.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned char UMP_FRAME_START  = 0xF0U;

const unsigned char UMP_HELLO        = 0x00U;

const unsigned char UMP_SET_MODE     = 0x01U;
const unsigned char UMP_SET_TX       = 0x02U;
const unsigned char UMP_SET_CD       = 0x03U;

const unsigned char UMP_WRITE_SERIAL = 0x10U;
const unsigned char UMP_READ_SERIAL  = 0x11U;

const unsigned char UMP_STATUS       = 0x50U;

const unsigned int BUFFER_LENGTH = 255U;

CUMP::CUMP(const std::string& port) :
m_serial(port, SERIAL_115200),
m_open(false),
m_buffer(NULL),
m_length(0U),
m_offset(0U),
m_lockout(false),
m_mode(MODE_IDLE),
m_tx(false),
m_cd(false)
{
	m_buffer = new unsigned char[BUFFER_LENGTH];
}

CUMP::~CUMP()
{
	delete[] m_buffer;
}

bool CUMP::open()
{
	if (m_open)
		return true;

	LogMessage("Opening the UMP");

	bool ret = m_serial.open();
	if (!ret)
		return false;

	unsigned char buffer[3U];

	buffer[0U] = UMP_FRAME_START;
	buffer[1U] = 3U;
	buffer[2U] = UMP_HELLO;

	// CUtils::dump(1U, "Transmitted", buffer, 3U);

	int n = m_serial.write(buffer, 3U);
	if (n != 3) {
		m_serial.close();
		return false;
	}

	m_open = true;

	return true;
}

bool CUMP::setMode(unsigned char mode)
{
	if (mode == m_mode)
		return true;

	m_mode = mode;

	unsigned char buffer[4U];

	buffer[0U] = UMP_FRAME_START;
	buffer[1U] = 4U;
	buffer[2U] = UMP_SET_MODE;
	buffer[3U] = mode;

	// CUtils::dump(1U, "Transmitted", buffer, 4U);

	return m_serial.write(buffer, 4U) == 4;
}

bool CUMP::setTX(bool on)
{
	if (on == m_tx)
		return true;

	m_tx = on;

	unsigned char buffer[4U];

	buffer[0U] = UMP_FRAME_START;
	buffer[1U] = 4U;
	buffer[2U] = UMP_SET_TX;
	buffer[3U] = on ? 0x01U : 0x00U;

	// CUtils::dump(1U, "Transmitted", buffer, 4U);

	return m_serial.write(buffer, 4U) == 4;
}

bool CUMP::setCD(bool on)
{
	if (on == m_cd)
		return true;

	m_cd = on;

	unsigned char buffer[4U];

	buffer[0U] = UMP_FRAME_START;
	buffer[1U] = 4U;
	buffer[2U] = UMP_SET_CD;
	buffer[3U] = on ? 0x01U : 0x00U;

	// CUtils::dump(1U, "Transmitted", buffer, 4U);

	return m_serial.write(buffer, 4U) == 4;
}

bool CUMP::getLockout() const
{
	return m_lockout;
}

int CUMP::write(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	unsigned char buffer[250U];

	buffer[0U] = UMP_FRAME_START;
	buffer[1U] = length + 3U;
	buffer[2U] = UMP_WRITE_SERIAL;

	::memcpy(buffer + 3U, data, length);

	// CUtils::dump(1U, "Transmitted", buffer, length + 3U);

	return m_serial.write(buffer, length + 3U);
}

// To be implemented later if needed
int CUMP::read(unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	return 0;
}

void CUMP::clock(unsigned int ms)
{
	if (m_offset == 0U) {
		// Get the start of the frame or nothing at all
		int ret = m_serial.read(m_buffer + 0U, 1U);
		if (ret < 0) {
			LogError("Error when reading from the UMP");
			return;
		}

		if (ret == 0)
			return;

		if (m_buffer[0U] != UMP_FRAME_START)
			return;

		m_offset = 1U;
	}

	if (m_offset == 1U) {
		// Get the length of the frame
		int ret = m_serial.read(m_buffer + 1U, 1U);
		if (ret < 0) {
			LogError("Error when reading from the UMP");
			m_offset = 0U;
			return;
		}

		if (ret == 0)
			return;

		if (m_buffer[1U] >= 250U) {
			LogError("Invalid length received from the UMP - %u", m_buffer[1U]);
			m_offset = 0U;
			return;
		}

		m_length = m_buffer[1U];
		m_offset = 2U;
	}

	if (m_offset == 2U) {
		// Get the frame type
		int ret = m_serial.read(m_buffer + 2U, 1U);
		if (ret < 0) {
			LogError("Error when reading from the UMP");
			m_offset = 0U;
			return;
		}

		if (ret == 0)
			return;

		switch (m_buffer[2U]) {
		case UMP_STATUS:
		case UMP_READ_SERIAL:
			break;

		default:
			LogError("Unknown message, type: %02X", m_buffer[2U]);
			m_offset = 0U;
			return;
		}

		m_offset = 3U;
	}

	if (m_offset >= 3U) {
		while (m_offset < m_length) {
			int ret = m_serial.read(m_buffer + m_offset, m_length - m_offset);
			if (ret < 0) {
				LogError("Error when reading from the UMP");
				m_offset = 0U;
				return;
			}

			if (ret == 0)
				return;

			if (ret > 0)
				m_offset += ret;
		}
	}

	m_offset = 0U;

	// CUtils::dump(1U, "Received", m_buffer, m_length);

	if (m_buffer[2U] == UMP_STATUS)
		m_lockout = (m_buffer[3U] & 0x01U) == 0x01U;
}

void CUMP::close()
{
	if (!m_open)
		return;

	LogMessage("Closing the UMP");

	m_serial.close();
	
	m_open = false;
}
