/*
 *   Copyright (C) 2002-2004,2007-2011,2013,2014-2017 by Jonathan Naylor G4KLX
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

#include "IICController.h"
#include "Log.h"

#include <cstring>
#include <cassert>

#include <sys/types.h>

#if defined(_WIN32) || defined(_WIN64)
#include <setupapi.h>
#include <winioctl.h>
#else
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#if !defined(__APPLE__)
#include <linux/i2c-dev.h>
#endif
#endif


#if defined(_WIN32) || defined(_WIN64)

CIICController::CSerialController(const std::string& device, SERIAL_SPEED speed, unsigned int address, bool assertRTS) :
CSerialController(device,speed,assertRTS),
m_address(address)
{
}

CIICController::~CIICController()
{
}

bool CIICController::open()
{
	return CSerialController::open();
}

int CIICController::read(unsigned char* buffer, unsigned int length)
{
	return CSerialController::read(buffer,length);
}

int CIICController::write(const unsigned char* buffer, unsigned int length)
{
	return CSerialController:;write(buffer,length);
}

#else

CIICController::CIICController(const std::string& device, SERIAL_SPEED speed, unsigned int address, bool assertRTS) :
CSerialController(device,speed,assertRTS),
m_address(address)
{
}

CIICController::~CIICController()
{
}

bool CIICController::open()
{
	assert(m_fd == -1);
#if !defined(__APPLE__)
	m_fd = ::open(m_device.c_str(), O_RDWR);
	if (m_fd < 0) {
		LogError("Cannot open device - %s", m_device.c_str());
		return false;
	}

	if (::ioctl(m_fd, I2C_TENBIT, 0) < 0) {
		LogError("CI2C: failed to set 7bitaddress");
		::close(m_fd);
		return false;
	}

	if (::ioctl(m_fd, I2C_SLAVE, m_address) < 0) {
		LogError("CI2C: Failed to acquire bus access/talk to slave 0x%02X", m_address);
		::close(m_fd);
		return false;
	}
#else
#warning "I2C controller does not support OSX"
#endif
	return true;
}

int CIICController::read(unsigned char* buffer, unsigned int length)
{
	assert(buffer != NULL);
	assert(m_fd != -1);

	if (length == 0U)
		return 0;

	unsigned int offset = 0U;

	while (offset < length) {
#if !defined(__APPLE__)
		ssize_t n = ::read(m_fd, buffer + offset, 1U);
		if (n < 0) {
			if (errno != EAGAIN) {
				LogError("Error returned from read(), errno=%d", errno);
				return -1;
			}
		}

		if (n > 0)
			offset += n;
#endif
	}

	return length;
}

int CIICController::write(const unsigned char* buffer, unsigned int length)
{
	assert(buffer != NULL);
	assert(m_fd != -1);

	if (length == 0U)
		return 0;

	unsigned int ptr = 0U;
	while (ptr < length) {
		ssize_t n = 0U;
#if !defined(__APPLE__)
		n = ::write(m_fd, buffer + ptr, 1U);
#endif
		if (n < 0) {
			if (errno != EAGAIN) {
				LogError("Error returned from write(), errno=%d", errno);
				return -1;
			}
		}

		if (n > 0)
			ptr += n;
	}

	return length;
}

#endif
