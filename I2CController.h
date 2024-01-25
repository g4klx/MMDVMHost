/*
 *   Copyright (C) 2002-2004,2007-2009,2011-2013,2015-2017,2020,2021 by Jonathan Naylor G4KLX
 *   Copyright (C) 1999-2001 by Thomas Sailor HB9JNX
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

#ifndef I2CController_H
#define I2CController_H

#if defined(__linux__)

#include "ModemPort.h"
#include "SerialPort.h"

#include <string>

class CI2CController : public ISerialPort, public IModemPort {
public:
	CI2CController(const std::string& device, unsigned int address = 0x22U);
	virtual ~CI2CController();

	virtual bool open();

	virtual int read(unsigned char* buffer, unsigned int length);

	virtual int write(const unsigned char* buffer, unsigned int length);

	virtual void close();

private:
	std::string  m_device;
	unsigned int m_address;
	int          m_fd;
};

#endif

#endif
