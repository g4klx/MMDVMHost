/*
 *   Copyright (C) 2020,2021 by Jonathan Naylor G4KLX
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

#include "I2CModem.h"

#include <cassert>


CI2CModem::CI2CModem(CModem* modem) :
m_modem(modem),
m_address(0x00U)
{
	assert(modem != NULL);
}

CI2CModem::~CI2CModem()
{
}

bool CI2CModem::open(unsigned char displayType)
{
	return true;
}

void CI2CModem::setAddress(unsigned char address)
{
	m_address = address;
}

void CI2CModem::setDataMode()
{
}

void CI2CModem::sendCommand(uint8_t c0, uint8_t c1, uint8_t c2)
{
	uint8_t buff[3U];

	buff[0U] = c0;
	buff[1U] = c1;
	buff[2U] = c2;

	// Write Data on I2C
	m_modem->writeI2CCommand(m_address, buff, 3U);
}

void CI2CModem::sendCommand(uint8_t c0, uint8_t c1)
{
	uint8_t buff[2U];

	buff[0U] = c0;
	buff[1U] = c1;

	// Write Data on I2C
	m_modem->writeI2CCommand(m_address, buff, 2U);
}

void CI2CModem::sendCommand(uint8_t c)
{
	// Write Data on I2C
	m_modem->writeI2CCommand(m_address, &c, 1U);
}

void CI2CModem::writeData(const uint8_t* data, uint16_t length)
{
	m_modem->writeI2CData(m_address, data, length);
}

void CI2CModem::writeData(uint8_t c)
{
	m_modem->writeI2CData(m_address, &c, 1U);
}

void CI2CModem::close()
{
}
