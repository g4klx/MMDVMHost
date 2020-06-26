/*
 *   Copyright (C) 2002-2004,2007-2011,2013,2014-2017,2019,2020 by Jonathan Naylor G4KLX
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

#if !defined(_WIN32) && !defined(_WIN64)

#include "PseudoTTYController.h"
#include "Log.h"

#include <cstring>
#include <cassert>
#include <cstdlib>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <pty.h>


CPseudoTTYController::CPseudoTTYController(const std::string& device, unsigned int speed, bool assertRTS) :
CSerialController(device, speed, assertRTS)
{
}

CPseudoTTYController::~CPseudoTTYController()
{
}

bool CPseudoTTYController::open()
{
	assert(m_fd == -1);

	int slavefd;
	char buf[300];
	int result = ::openpty(&m_fd, &slavefd, buf, NULL,NULL);
	if (result < 0) {
		LogError("Cannot open device - %s - Errno : %d", m_device.c_str(), errno);
		return false;
	}

	std::string slave = std::string(::ptsname(m_fd));

	// Remove any previous stale symlink
	::unlink(m_device.c_str());

	int ret = ::symlink(slave.c_str(), m_device.c_str());
	if (ret != 0) {
		LogError("Cannot make symlink to %s with %s", slave.c_str(), m_device.c_str());
		close();
		return false;
	}

	m_device = std::string(::ttyname(m_fd));

	if (::isatty(m_fd)) {
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
		termios.c_cflag |= (CS8 | CLOCAL | CREAD);
		termios.c_lflag &= ~(ISIG | ICANON | IEXTEN);
		termios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
		termios.c_cc[VMIN] = 0;
		termios.c_cc[VTIME] = 10;

		switch (m_speed) {
		case 1200U:
			::cfsetospeed(&termios, B1200);
			::cfsetispeed(&termios, B1200);
			break;
		case 2400U:
			::cfsetospeed(&termios, B2400);
			::cfsetispeed(&termios, B2400);
			break;
		case 4800U:
			::cfsetospeed(&termios, B4800);
			::cfsetispeed(&termios, B4800);
			break;
		case 9600U:
			::cfsetospeed(&termios, B9600);
			::cfsetispeed(&termios, B9600);
			break;
		case 19200U:
			::cfsetospeed(&termios, B19200);
			::cfsetispeed(&termios, B19200);
			break;
		case 38400U:
			::cfsetospeed(&termios, B38400);
			::cfsetispeed(&termios, B38400);
			break;
		case 115200U:
			::cfsetospeed(&termios, B115200);
			::cfsetispeed(&termios, B115200);
			break;
		case 230400U:
			::cfsetospeed(&termios, B230400);
			::cfsetispeed(&termios, B230400);
			break;
		case 460800U:
			::cfsetospeed(&termios, B460800);
			::cfsetispeed(&termios, B460800);
			break;
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
	}

	return true;
}

#endif

