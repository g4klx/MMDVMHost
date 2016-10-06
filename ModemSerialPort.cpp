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

#include "ModemSerialPort.h"

#include <cstdio>
#include <cassert>

CModemSerialPort::CModemSerialPort(CModem* modem) :
m_modem(modem)
{
	assert(modem != NULL);
}

CModemSerialPort::~CModemSerialPort()
{
}

bool CModemSerialPort::open()
{
	return true;
}

int CModemSerialPort::write(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	bool ret = m_modem->writeSerial(data, length);

	return ret ? int(length) : -1;
}

int CModemSerialPort::read(unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	return m_modem->readSerial(data, length);
}

void CModemSerialPort::close()
{
}
