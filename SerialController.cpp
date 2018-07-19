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

#include "SerialController.h"
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
#include <linux/i2c-dev.h>
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

CSerialController::CSerialController(const std::string& device, SERIAL_SPEED speed, const std::string& protocol, unsigned int address, bool assertRTS) :
m_device(device),
m_speed(speed),
m_protocol(protocol),
m_address(address),
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

	if (m_protocol == "i2c"){
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
	} else {
#if defined(__APPLE__)
		m_fd = ::open(m_device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK); /*open in block mode under OSX*/
#else
		m_fd = ::open(m_device.c_str(), O_RDWR | O_NOCTTY | O_NDELAY, 0);
#endif
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

#if defined(__APPLE__)
		termios.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
		termios.c_cflag &= ~CSIZE;
		termios.c_cflag |= CS8;         /* 8-bit characters */
		termios.c_cflag &= ~PARENB;     /* no parity bit */
		termios.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
		termios.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

	    /* setup for non-canonical mode */
		termios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
		termios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
		termios.c_oflag &= ~OPOST;

	    /* fetch bytes as they become available */
		termios.c_cc[VMIN] = 1;
		termios.c_cc[VTIME] = 1;
#else
		termios.c_lflag    &= ~(ECHO | ECHOE | ICANON | IEXTEN | ISIG);
		termios.c_iflag    &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON | IXOFF | IXANY);
		termios.c_cflag    &= ~(CSIZE | CSTOPB | PARENB | CRTSCTS);
		termios.c_cflag    |= CS8;
		termios.c_oflag    &= ~(OPOST);
		termios.c_cc[VMIN]  = 0;
		termios.c_cc[VTIME] = 10;
#endif

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

#if defined(__APPLE__)
		setNonblock(false);
#endif
    }

	return true;
}

#if defined(__APPLE__)
int CSerialController::setNonblock(bool nonblock)
{
	int flag = ::fcntl(m_fd, F_GETFD, 0);

	if (nonblock)
		flag |= O_NONBLOCK;
	else
		flag &= ~O_NONBLOCK;

	return ::fcntl(m_fd, F_SETFL, flag);
}
#endif

int CSerialController::read(unsigned char* buffer, unsigned int length)
{
	assert(buffer != NULL);
	assert(m_fd != -1);

	if (length == 0U)
		return 0;

	unsigned int offset = 0U;

	while (offset < length) {
        if (m_protocol == "i2c"){
			ssize_t n = ::read(m_fd, buffer + offset, 1U);
			if (n < 0) {
				if (errno != EAGAIN) {
					LogError("Error returned from read(), errno=%d", errno);
					return -1;
				}
			}

			if (n > 0)
				offset += n;
		} else {

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
	}

	return length;
}

bool CSerialController::canWrite(){
#if defined(__APPLE__)
	fd_set wset;
	FD_ZERO(&wset);
	FD_SET(m_fd, &wset);

	struct timeval timeo;
	timeo.tv_sec  = 0;
	timeo.tv_usec = 0;

	int rc = select(m_fd + 1, NULL, &wset, NULL, &timeo);
	if (rc >0 && FD_ISSET(m_fd, &wset))
		return true;

	return false;
#else
	return true;
#endif
}

int CSerialController::write(const unsigned char* buffer, unsigned int length)
{
	assert(buffer != NULL);
	assert(m_fd != -1);

	if (length == 0U)
		return 0;

	unsigned int ptr = 0U;
	while (ptr < length) {
		ssize_t n = 0U;
		if (m_protocol == "i2c"){
            n = ::write(m_fd, buffer + ptr, 1U);
		} else {
		if (canWrite())
			n = ::write(m_fd, buffer + ptr, length - ptr);
        }
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
