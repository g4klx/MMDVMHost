/*
 *   Copyright (C) 2019 by Jonathan Naylor G4KLX
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

#include "RemoteControl.h"

#include <cstdio>
#include <cassert>

const unsigned int BUFFER_LENGTH = 100U;

CRemoteControl::CRemoteControl(unsigned int port) :
m_socket(port)
{
	assert(port > 0U);
}

CRemoteControl::~CRemoteControl()
{
}

bool CRemoteControl::open()
{
	return m_socket.open();
}

REMOTE_COMMAND CRemoteControl::getCommand()
{
	REMOTE_COMMAND command = RC_NONE;

	unsigned char buffer[BUFFER_LENGTH];
	in_addr address;
	unsigned int port;
	int ret = m_socket.read(buffer, BUFFER_LENGTH, address, port);
	if (ret > 0) {
		if (::memcmp(buffer, "0", 1U) == 0)
			command = RC_FORCE_IDLE;
	}
	
	return command;
}

void CRemoteControl::close()
{
	m_socket.close();
}
