/*
 *   Copyright (C) 2002-2004,2007-2011,2013,2014-2016 by Jonathan Naylor G4KLX
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

#include "SerialController.h"
#include "Log.h"

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
#endif


#if defined(_WIN32) || defined(_WIN64)

const unsigned int BUFFER_LENGTH = 1000U;

CSerialController::CSerialController(const std::string& device, SERIAL_SPEED speed, bool assertRTS) :
m_device(device),
m_speed(speed),
m_assertRTS(assertRTS),
m_handle(INVALID_HANDLE_VALUE),
m_readOverlapped(),
m_writeOverlapped(),
m_readBuffer(NULL),
m_readLength(0U),
m_readPending(false)
{
	assert(!device.empty());

	m_readBuffer = new unsigned char[BUFFER_LENGTH];
}

CSerialController::~CSerialController()
{
	delete[] m_readBuffer;
}

bool CSerialController::open()
{
	assert(m_handle == INVALID_HANDLE_VALUE);

	DWORD errCode;

	std::string baseName = m_device.substr(4U);		// Convert "\\.\COM10" to "COM10"

	m_handle = ::CreateFileA(m_device.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
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

	dcb.BaudRate     = DWORD(m_speed);
	dcb.ByteSize     = 8;
	dcb.Parity       = NOPARITY;
	dcb.fParity      = FALSE;
	dcb.StopBits     = ONESTOPBIT;
	dcb.fInX         = FALSE;
	dcb.fOutX        = FALSE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fDtrControl  = DTR_CONTROL_DISABLE;
	dcb.fRtsControl  = RTS_CONTROL_DISABLE;

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

	::memset(&m_readOverlapped, 0x00U, sizeof(OVERLAPPED));
	::memset(&m_writeOverlapped, 0x00U, sizeof(OVERLAPPED));

	m_readOverlapped.hEvent  = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	m_writeOverlapped.hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);

	m_readLength  = 0U;
	m_readPending = false;
	::memset(m_readBuffer, 0x00U, BUFFER_LENGTH);

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

	if (length > BUFFER_LENGTH)
		length = BUFFER_LENGTH;

	if (m_readPending && length != m_readLength) {
		::CancelIo(m_handle);
		m_readPending = false;
	}

	m_readLength = length;

	if (length == 0U)
		return 0;

	if (!m_readPending) {
		DWORD bytes = 0UL;
		BOOL res = ::ReadFile(m_handle, m_readBuffer, m_readLength, &bytes, &m_readOverlapped);
		if (res) {
			::memcpy(buffer, m_readBuffer, bytes);
			return int(bytes);
		}

		DWORD error = ::GetLastError();
		if (error != ERROR_IO_PENDING) {
			LogError("Error from ReadFile: %04lx", error);
			return -1;
		}

		m_readPending = true;
	}

	BOOL res = HasOverlappedIoCompleted(&m_readOverlapped);
	if (!res)
		return 0;

	DWORD bytes = 0UL;
	res = ::GetOverlappedResult(m_handle, &m_readOverlapped, &bytes, TRUE);
	if (!res) {
		LogError("Error from GetOverlappedResult (ReadFile): %04lx", ::GetLastError());
		return -1;
	}

	::memcpy(buffer, m_readBuffer, bytes);
	m_readPending = false;

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
		BOOL res = ::WriteFile(m_handle, buffer + ptr, length - ptr, &bytes, &m_writeOverlapped);
		if (!res) {
			DWORD error = ::GetLastError();
			if (error != ERROR_IO_PENDING) {
				LogError("Error from WriteFile: %04lx", error);
				return -1;
			}

			res = ::GetOverlappedResult(m_handle, &m_writeOverlapped, &bytes, TRUE);
			if (!res) {
				LogError("Error from GetOverlappedResult (WriteFile): %04lx", ::GetLastError());
				return -1;
			}
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

	::CloseHandle(m_readOverlapped.hEvent);
	::CloseHandle(m_writeOverlapped.hEvent);
}

#else

CSerialController::CSerialController(const std::string& device, SERIAL_SPEED speed, bool assertRTS) :
m_device(device),
m_speed(speed),
m_assertRTS(assertRTS),
m_fd(-1)
{
	assert(!device.empty());
}

CSerialController::~CSerialController()
{
}

bool CSerialController::open()
{
	assert(m_fd == -1);

	m_fd = ::open(m_device.c_str(), O_RDWR | O_NOCTTY | O_NDELAY, 0);
	if (m_fd < 0) {
		LogError("Cannot open device - %s", m_device.c_str());
		return false;
	}

	if (::isatty(m_fd) == 0) {
		LogError("%s is not a TTY device", m_device.c_str());
		::close(m_fd);
		return false;
	}

	termios termios;
	if (::tcgetattr(m_fd, &termios) < 0) {
		LogError("Cannot get the attributes for %s", m_device.c_str());
		::close(m_fd);
		return false;
	}

	termios.c_lflag    &= ~(ECHO | ECHOE | ICANON | IEXTEN | ISIG);
	termios.c_iflag    &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON | IXOFF | IXANY);
	termios.c_cflag    &= ~(CSIZE | CSTOPB | PARENB | CRTSCTS);
	termios.c_cflag    |= CS8;
	termios.c_oflag    &= ~(OPOST);
	termios.c_cc[VMIN]  = 0;
	termios.c_cc[VTIME] = 10;

	switch (m_speed) {
		case SERIAL_1200:
			::cfsetospeed(&termios, B1200);
			::cfsetispeed(&termios, B1200);
			break;
		case SERIAL_2400:
			::cfsetospeed(&termios, B2400);
			::cfsetispeed(&termios, B2400);
			break;
		case SERIAL_4800:
			::cfsetospeed(&termios, B4800);
			::cfsetispeed(&termios, B4800);
			break;
		case SERIAL_9600:
			::cfsetospeed(&termios, B9600);
			::cfsetispeed(&termios, B9600);
			break;
		case SERIAL_19200:
			::cfsetospeed(&termios, B19200);
			::cfsetispeed(&termios, B19200);
			break;
		case SERIAL_38400:
			::cfsetospeed(&termios, B38400);
			::cfsetispeed(&termios, B38400);
			break;
		case SERIAL_115200:
			::cfsetospeed(&termios, B115200);
			::cfsetispeed(&termios, B115200);
			break;
		case SERIAL_230400:
			::cfsetospeed(&termios, B230400);
			::cfsetispeed(&termios, B230400);
			break;
		default:
			LogError("Unsupported serial port speed - %d", int(m_speed));
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

	return true;
}

int CSerialController::read(unsigned char* buffer, unsigned int length)
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

int CSerialController::write(const unsigned char* buffer, unsigned int length)
{
	assert(buffer != NULL);
	assert(m_fd != -1);

	if (length == 0U)
		return 0;

	unsigned int ptr = 0U;

	while (ptr < length) {
		ssize_t n = ::write(m_fd, buffer + ptr, length - ptr);
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

void CSerialController::close()
{
	assert(m_fd != -1);

	::close(m_fd);
	m_fd = -1;
}

#endif
