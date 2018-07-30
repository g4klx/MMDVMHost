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

CSerialController::CSerialController(const std::string& device, SERIAL_SPEED speed, const std::string& protocol, unsigned int address, bool assertRTS) :
m_device(device),
m_speed(speed),
m_protocol(protocol),
m_address(address),
m_assertRTS(assertRTS),
m_handle(INVALID_HANDLE_VALUE)
{
	assert(!device.empty());
}

CSerialController::~CSerialController()
{
}

bool CSerialController::open()
{
	assert(m_handle == INVALID_HANDLE_VALUE);

	DWORD errCode;

	std::string baseName = m_device.substr(4U);		// Convert "\\.\COM10" to "COM10"

	m_handle = ::CreateFileA(m_device.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_handle == INVALID_HANDLE_VALUE) {
		LogError("Cannot open device - %s, err=%04lx", m_device.c_str(), ::GetLastError());
		return false;
	}

	DCB dcb;
	if (::GetCommState(m_handle, &dcb) == 0) {
		LogError("Cannot get the attributes for %s, err=%04lx", m_device.c_str(), ::GetLastError());
		::ClearCommError(m_handle, &errCode, NULL);
		::CloseHandle(m_handle);
		return false;
	}

	dcb.BaudRate        = DWORD(m_speed);
	dcb.ByteSize        = 8;
	dcb.Parity          = NOPARITY;
	dcb.fParity         = FALSE;
	dcb.StopBits        = ONESTOPBIT;
	dcb.fInX            = FALSE;
	dcb.fOutX           = FALSE;
	dcb.fOutxCtsFlow    = FALSE;
	dcb.fOutxDsrFlow    = FALSE;
	dcb.fDsrSensitivity = FALSE;
	dcb.fDtrControl     = DTR_CONTROL_DISABLE;
	dcb.fRtsControl     = RTS_CONTROL_DISABLE;

	if (::SetCommState(m_handle, &dcb) == 0) {
		LogError("Cannot set the attributes for %s, err=%04lx", m_device.c_str(), ::GetLastError());
		::ClearCommError(m_handle, &errCode, NULL);
		::CloseHandle(m_handle);
		return false;
	}

	COMMTIMEOUTS timeouts;
	if (!::GetCommTimeouts(m_handle, &timeouts)) {
		LogError("Cannot get the timeouts for %s, err=%04lx", m_device.c_str(), ::GetLastError());
		::ClearCommError(m_handle, &errCode, NULL);
		::CloseHandle(m_handle);
		return false;
	}

	timeouts.ReadIntervalTimeout        = MAXDWORD;
	timeouts.ReadTotalTimeoutMultiplier = 0UL;
	timeouts.ReadTotalTimeoutConstant   = 0UL;

	if (!::SetCommTimeouts(m_handle, &timeouts)) {
		LogError("Cannot set the timeouts for %s, err=%04lx", m_device.c_str(), ::GetLastError());
		::ClearCommError(m_handle, &errCode, NULL);
		::CloseHandle(m_handle);
		return false;
	}

	if (::EscapeCommFunction(m_handle, CLRDTR) == 0) {
		LogError("Cannot clear DTR for %s, err=%04lx", m_device.c_str(), ::GetLastError());
		::ClearCommError(m_handle, &errCode, NULL);
		::CloseHandle(m_handle);
		return false;
	}

	if (::EscapeCommFunction(m_handle, m_assertRTS ? SETRTS : CLRRTS) == 0) {
		LogError("Cannot set/clear RTS for %s, err=%04lx", m_device.c_str(), ::GetLastError());
		::ClearCommError(m_handle, &errCode, NULL);
		::CloseHandle(m_handle);
		return false;
	}

	::ClearCommError(m_handle, &errCode, NULL);

	return true;
}

int CSerialController::read(unsigned char* buffer, unsigned int length)
{
	assert(m_handle != INVALID_HANDLE_VALUE);
	assert(buffer != NULL);

	unsigned int ptr = 0U;

	while (ptr < length) {
		int ret = readNonblock(buffer + ptr, length - ptr);
		if (ret < 0) {
			return ret;
		} else if (ret == 0) {
			if (ptr == 0U)
				return 0;
		} else {
			ptr += ret;
		}
	}

	return int(length);
}

int CSerialController::readNonblock(unsigned char* buffer, unsigned int length)
{
	assert(m_handle != INVALID_HANDLE_VALUE);
	assert(buffer != NULL);

	if (length == 0U)
		return 0;

	DWORD errors;
	COMSTAT status;
	if (::ClearCommError(m_handle, &errors, &status) == 0) {
		LogError("Error from ClearCommError for %s, err=%04lx", m_device.c_str(), ::GetLastError());
		return -1;
	}

	if (status.cbInQue == 0UL)
		return 0;

	DWORD readLength = status.cbInQue;
	if (length < readLength)
		readLength = length;

	DWORD bytes = 0UL;
	BOOL ret = ::ReadFile(m_handle, buffer, readLength, &bytes, NULL);
	if (!ret) {
		LogError("Error from ReadFile for %s: %04lx", m_device.c_str(), ::GetLastError());
		return -1;
	}

	return int(bytes);
}

int CSerialController::write(const unsigned char* buffer, unsigned int length)
{
	assert(m_handle != INVALID_HANDLE_VALUE);
	assert(buffer != NULL);

	if (length == 0U)
		return 0;

	unsigned int ptr = 0U;

	while (ptr < length) {
		DWORD bytes = 0UL;
		BOOL ret = ::WriteFile(m_handle, buffer + ptr, length - ptr, &bytes, NULL);
		if (!ret) {
			LogError("Error from WriteFile for %s: %04lx", m_device.c_str(), ::GetLastError());
			return -1;
		}

		ptr += bytes;
	}

	return int(length);
}

void CSerialController::close()
{
	assert(m_handle != INVALID_HANDLE_VALUE);

	::CloseHandle(m_handle);
	m_handle = INVALID_HANDLE_VALUE;
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
