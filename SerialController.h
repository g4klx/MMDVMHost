/*
 *   Copyright (C) 2002-2004,2007-2009,2011-2013,2015,2016 by Jonathan Naylor G4KLX
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

#ifndef SerialController_H
#define SerialController_H

#include <string>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

enum SERIAL_SPEED {
	SERIAL_1200   = 1200,
	SERIAL_2400   = 2400,
	SERIAL_4800   = 4800,
	SERIAL_9600   = 9600,
	SERIAL_19200  = 19200,
	SERIAL_38400  = 38400,
	SERIAL_76800  = 76800,
	SERIAL_115200 = 115200,
	SERIAL_230400 = 230400
};

class CSerialController {
public:
	CSerialController(const std::string& device, SERIAL_SPEED speed, bool assertRTS = false);
	~CSerialController();

	bool open();

	int  read(unsigned char* buffer, unsigned int length);
	int  write(const unsigned char* buffer, unsigned int length);

	void close();

private:
	std::string    m_device;
	SERIAL_SPEED   m_speed;
	bool           m_assertRTS;
#if defined(_WIN32) || defined(_WIN64)
	HANDLE         m_handle;
	OVERLAPPED     m_readOverlapped;
	OVERLAPPED     m_writeOverlapped;
	unsigned char* m_readBuffer;
	unsigned int   m_readLength;
	bool           m_readPending;
#else
	int            m_fd;
#endif

#if defined(_WIN32) || defined(_WIN64)
	int readNonblock(unsigned char* buffer, unsigned int length);
#endif
};

#endif
