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


CPseudoTTYController::CPseudoTTYController(const std::string& symlink, unsigned int speed, bool assertRTS) :
CSerialController("", speed, assertRTS),
m_symlink(symlink)
{
}

CPseudoTTYController::~CPseudoTTYController()
{
}

bool CPseudoTTYController::open()
{
	assert(m_fd == -1);

	int slavefd;
	char slave[300];
	int result = ::openpty(&m_fd, &slavefd, slave, NULL, NULL);
	if (result < 0) {
		LogError("Cannot open the pseudo tty - errno : %d", errno);
		return false;
	}

	// Remove any previous stale symlink
	::unlink(m_symlink.c_str());

	int ret = ::symlink(slave, m_symlink.c_str());
	if (ret != 0) {
		LogError("Cannot make symlink to %s with %s", slave, m_symlink.c_str());
		close();
		return false;
	}

	LogMessage("Made symbolic link from %s to %s", slave, m_symlink.c_str());

	m_device = std::string(::ttyname(m_fd));

	return setRaw();
}

void CPseudoTTYController::close()
{
	CSerialController::close();

	::unlink(m_symlink.c_str());
}

#endif

