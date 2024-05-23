/*
 *   Copyright (C) 2002-2004,2007-2011,2013,2014-2017,2019,2020,2021 by Jonathan Naylor G4KLX
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

#include "UARTController.h"
#include "Log.h"

#include <cstring>
#include <cassert>

#if defined(_WIN32) || defined(_WIN64)
#include <setupapi.h>
#include <winioctl.h>
#else
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#endif


#if defined(_WIN32) || defined(_WIN64)

CUARTController::CUARTController(const std::string& device, unsigned int speed, bool assertRTS) :
m_device(device),
m_speed(speed),
m_assertRTS(assertRTS),
m_handle(INVALID_HANDLE_VALUE)
{
	assert(!device.empty());
}

CUARTController::CUARTController(unsigned int speed, bool assertRTS) :
m_device(),
m_speed(speed),
m_assertRTS(assertRTS),
m_handle(INVALID_HANDLE_VALUE)
{
}

CUARTController::~CUARTController()
{
}

bool CUARTController::open()
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

int CUARTController::read(unsigned char* buffer, unsigned int length)
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

int CUARTController::readNonblock(unsigned char* buffer, unsigned int length)
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

int CUARTController::write(const unsigned char* buffer, unsigned int length)
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

void CUARTController::close()
{
	assert(m_handle != INVALID_HANDLE_VALUE);

	::CloseHandle(m_handle);
	m_handle = INVALID_HANDLE_VALUE;
}

#else

CUARTController::CUARTController(const std::string& device, unsigned int speed, bool assertRTS) :
m_device(device),
m_speed(speed),
m_assertRTS(assertRTS),
m_fd(-1)
{
	assert(!device.empty());
}

CUARTController::CUARTController(unsigned int speed, bool assertRTS) :
m_device(),
m_speed(speed),
m_assertRTS(assertRTS),
m_fd(-1)
{
}

CUARTController::~CUARTController()
{
}

bool CUARTController::open()
{
	assert(m_fd == -1);

#if defined(__APPLE__)
	m_fd = ::open(m_device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK); /*open in block mode under OSX*/
#else
	m_fd = ::open(m_device.c_str(), O_RDWR | O_NOCTTY | O_NDELAY, 0);
#endif
	if (m_fd < 0) {
		LogError("Cannot open device - %s", m_device.c_str());
		return false;
	}

	if (::isatty(m_fd))
		return setRaw();
		
	return true;
}

bool CUARTController::setRaw()
{
	termios termios;
	if (::tcgetattr(m_fd, &termios) < 0) {
		LogError("Cannot get the attributes for %s", m_device.c_str());
		::close(m_fd);
		return false;
	}

	termios.c_iflag &= ~(IGNBRK | BRKINT | IGNPAR | PARMRK | INPCK);
	termios.c_iflag &= ~(ISTRIP | INLCR | IGNCR | ICRNL);
	termios.c_iflag &= ~(IXON | IXOFF | IXANY);
	termios.c_oflag &= ~(OPOST);
	termios.c_cflag &= ~(CSIZE | CSTOPB | PARENB | CRTSCTS);
	termios.c_cflag |=  (CS8 | CLOCAL | CREAD);
	termios.c_lflag &= ~(ISIG | ICANON | IEXTEN);
	termios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
#if defined(__APPLE__)
	termios.c_cc[VMIN] = 1;
	termios.c_cc[VTIME] = 1;
	#define B460800 460800
#else
	termios.c_cc[VMIN]  = 0;
	termios.c_cc[VTIME] = 10;
#endif

	switch (m_speed) {
#if defined(B1200)
		case 1200U:
			::cfsetospeed(&termios, B1200);
			::cfsetispeed(&termios, B1200);
			break;
#endif /*B1200*/
#if defined(B2400)			
		case 2400U:
			::cfsetospeed(&termios, B2400);
			::cfsetispeed(&termios, B2400);
			break;
#endif /*B2400*/
#if defined(B4800)
		case 4800U:
			::cfsetospeed(&termios, B4800);
			::cfsetispeed(&termios, B4800);
			break;
#endif /*B4800*/
#if defined(B9600)
		case 9600U:
			::cfsetospeed(&termios, B9600);
			::cfsetispeed(&termios, B9600);
			break;
#endif /*B9600*/
#if defined(B19200)
		case 19200U:
			::cfsetospeed(&termios, B19200);
			::cfsetispeed(&termios, B19200);
			break;
#endif /*B19200*/
#if defined(B38400)
		case 38400U:
			::cfsetospeed(&termios, B38400);
			::cfsetispeed(&termios, B38400);
			break;
#endif /*B38400*/
#if defined(B57600)
		case 57600U:
			::cfsetospeed(&termios, B57600);
			::cfsetispeed(&termios, B57600);
			break;
#endif /*B57600*/
#if defined(B115200)
		case 115200U:
			::cfsetospeed(&termios, B115200);
			::cfsetispeed(&termios, B115200);
			break;
#endif /*B115200*/
#if defined(B230400)
		case 230400U:
			::cfsetospeed(&termios, B230400);
			::cfsetispeed(&termios, B230400);
			break;
#endif /*B230400*/
#if defined(B460800)		
		case 460800U:
 			::cfsetospeed(&termios, B460800);
 			::cfsetispeed(&termios, B460800);
			break;
#endif /*B460800*/
#if defined(B500000)
                case 500000U:
                        ::cfsetospeed(&termios, B500000);
                        ::cfsetispeed(&termios, B500000);
                        break;
#endif /*B500000*/
		default:
			LogError("Unsupported serial port speed - %u", m_speed);
			::close(m_fd);
			return false;
	}

	if (::tcsetattr(m_fd, TCSANOW, &termios) < 0) {
		LogError("Cannot set the attributes for %s", m_device.c_str());
		::close(m_fd);
		return false;
	}

	if (m_assertRTS) {
		unsigned int y;
		if (::ioctl(m_fd, TIOCMGET, &y) < 0) {
			LogError("Cannot get the control attributes for %s", m_device.c_str());
			::close(m_fd);
			return false;
		}

		y |= TIOCM_RTS;

		if (::ioctl(m_fd, TIOCMSET, &y) < 0) {
			LogError("Cannot set the control attributes for %s", m_device.c_str());
			::close(m_fd);
			return false;
		}
	}

#if defined(__APPLE__)
	setNonblock(false);
#endif

	return true;
}

#if defined(__APPLE__)
int CUARTController::setNonblock(bool nonblock)
{
	int flag = ::fcntl(m_fd, F_GETFL, 0);

	if (nonblock)
		flag |= O_NONBLOCK;
	else
		flag &= ~O_NONBLOCK;

	return ::fcntl(m_fd, F_SETFL, flag);
}
#endif

int CUARTController::read(unsigned char* buffer, unsigned int length)
{
	assert(buffer != NULL);
	assert(m_fd != -1);

	if (length == 0U)
		return 0;

	unsigned int offset = 0U;

	while (offset < length) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(m_fd, &fds);
		int n;
		if (offset == 0U) {
			struct timeval tv;
			tv.tv_sec  = 0;
			tv.tv_usec = 0;
			n = ::select(m_fd + 1, &fds, NULL, NULL, &tv);
			if (n == 0)
				return 0;
		} else {
			n = ::select(m_fd + 1, &fds, NULL, NULL, NULL);
		}

		if (n < 0) {
			LogError("Error from select(), errno=%d", errno);
			return -1;
		}

		if (n > 0) {
			ssize_t len = ::read(m_fd, buffer + offset, length - offset);
			if (len < 0) {
				if (errno != EAGAIN) {
					LogError("Error from read(), errno=%d", errno);
					return -1;
				}
			}

			if (len > 0)
				offset += len;
		}
	}

	return length;
}

bool CUARTController::canWrite(){
#if defined(__APPLE__)
	fd_set wset;
	FD_ZERO(&wset);
	FD_SET(m_fd, &wset);

	struct timeval timeo;
	timeo.tv_sec  = 0;
	timeo.tv_usec = 0;

	int rc = ::select(m_fd + 1, NULL, &wset, NULL, &timeo);
	if (rc > 0 && FD_ISSET(m_fd, &wset))
		return true;

	return false;
#else
	return true;
#endif
}

int CUARTController::write(const unsigned char* buffer, unsigned int length)
{
	assert(buffer != NULL);
	assert(m_fd != -1);

	if (length == 0U)
		return 0;

	unsigned int ptr = 0U;
	while (ptr < length) {
		ssize_t n = 0U;
		if (canWrite())
			n = ::write(m_fd, buffer + ptr, length - ptr);
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

void CUARTController::close()
{
	assert(m_fd != -1);

	::close(m_fd);
	m_fd = -1;
}

#endif

