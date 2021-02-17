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

#ifndef UARTController_H
#define UARTController_H

#include "ModemPort.h"
#include "SerialPort.h"

#include <string>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

class CUARTController : public ISerialPort, public IModemPort {
public:
	CUARTController(const std::string& device, unsigned int speed, bool assertRTS = false);
	virtual ~CUARTController();

	virtual bool open();

	virtual int read(unsigned char* buffer, unsigned int length);

	virtual int write(const unsigned char* buffer, unsigned int length);

	virtual void close();

#if defined(__APPLE__)
	virtual int setNonblock(bool nonblock);
#endif

protected:
	CUARTController(unsigned int speed, bool assertRTS = false);

	std::string    m_device;
	unsigned int   m_speed;
	bool           m_assertRTS;
#if defined(_WIN32) || defined(_WIN64)
	HANDLE         m_handle;
#else
	int            m_fd;
#endif

#if defined(_WIN32) || defined(_WIN64)
	int readNonblock(unsigned char* buffer, unsigned int length);
#else
	bool canWrite();
	bool setRaw();
#endif
};

#endif
