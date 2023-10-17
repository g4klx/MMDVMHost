/*
*   Copyright (C) 2021 by Jonathan Naylor G4KLX
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

#include "NullController.h"

#include <cstdio>
#include <cstdint>
#include <cassert>

const unsigned char MMDVM_FRAME_START = 0xE0U;

const unsigned char MMDVM_GET_VERSION = 0x00U;
const unsigned char MMDVM_GET_STATUS  = 0x01U;
const unsigned char MMDVM_SET_CONFIG  = 0x02U;
const unsigned char MMDVM_SET_MODE    = 0x03U;
const unsigned char MMDVM_SET_FREQ    = 0x04U;

const unsigned char MMDVM_FM_PARAMS1 = 0x60U;
const unsigned char MMDVM_FM_PARAMS2 = 0x61U;
const unsigned char MMDVM_FM_PARAMS3 = 0x62U;
const unsigned char MMDVM_FM_PARAMS4 = 0x63U;

const unsigned char MMDVM_ACK = 0x70U;
const unsigned char MMDVM_NAK = 0x7FU;

const unsigned char PROTOCOL_VERSION = 2U; 

const char* HARDWARE = "Null Modem Controller";

CNullController::CNullController() :
m_buffer(200U, "Null Controller Buffer")
{
}

CNullController::~CNullController()
{
}

bool CNullController::open()
{
	return true;
}

int CNullController::read(unsigned char* buffer, unsigned int length)
{
	unsigned int dataSize = m_buffer.dataSize();
	if (dataSize == 0U)
		return 0;

	if (length > dataSize)
		length = dataSize;

	m_buffer.getData(buffer, length);

	return int(length);
}

int CNullController::write(const unsigned char* buffer, unsigned int length)
{
	switch (buffer[2U]) {
	case MMDVM_GET_VERSION:
		writeVersion();
		break;
	case MMDVM_GET_STATUS:
		writeStatus();
		break;
	case MMDVM_SET_CONFIG:
	case MMDVM_SET_FREQ:
	case MMDVM_SET_MODE:
	case MMDVM_FM_PARAMS1:
	case MMDVM_FM_PARAMS2:
	case MMDVM_FM_PARAMS3:
	case MMDVM_FM_PARAMS4:
		writeAck(buffer[2U]);
		break;
	default:
		break;
	}

	return int(length);
}


void CNullController::close()
{
}

void CNullController::writeVersion()
{
	unsigned char reply[200U];

	reply[0U] = MMDVM_FRAME_START;
	reply[1U] = 0U;
	reply[2U] = MMDVM_GET_VERSION;

	reply[3U] = PROTOCOL_VERSION;

	// Return two bytes of mode capabilities
	reply[4U] = 0xFFU;
	reply[5U] = 0xFFU;

	// CPU type/manufacturer. 0=Atmel ARM, 1=NXP ARM, 2=St-Micro ARM
	reply[6U] = 2U;

	// Reserve 16 bytes for the UDID
	::memset(reply + 7U, 0x00U, 16U);

	uint8_t count = 23U;
	for (uint8_t i = 0U; HARDWARE[i] != 0x00U; i++, count++)
		reply[count] = HARDWARE[i];

	reply[1U] = count;

	m_buffer.addData(reply, count);
}

void CNullController::writeStatus()
{
	unsigned char reply[30U];

	// Send all sorts of interesting internal values
	reply[0U] = MMDVM_FRAME_START;
	reply[1U] = 20U;
	reply[2U] = MMDVM_GET_STATUS;

	reply[3U] = 0U;

	reply[4U] = 0x00U;

	reply[5U] = 0x00U;

	reply[6U] = 20U;

	reply[7U] = 20U;
	reply[8U] = 20U;

	reply[9U] = 20U;

	reply[10U] = 20U;

	reply[11U] = 20U;

	reply[12U] = 20U;

	reply[13U] = 20U;

	reply[14U] = 0x00U;
	reply[15U] = 0x00U;

	reply[16U] = 20U;

	reply[17U] = 20U;

	reply[18U] = 0x00U;
	reply[19U] = 0x00U;

	m_buffer.addData(reply, 20U);
}

void CNullController::writeAck(unsigned char type)
{
	unsigned char reply[4U];

	reply[0U] = MMDVM_FRAME_START;
	reply[1U] = 4U;
	reply[2U] = MMDVM_ACK;
	reply[3U] = type;

	m_buffer.addData(reply, 4U);
}
