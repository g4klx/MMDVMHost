/*
 *   Copyright (C) 2021 by Jonathan Naylor G4KLX
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

#include "UDPController.h"
#include "Log.h"

#include <cstring>
#include <cassert>

const unsigned int BUFFER_LENGTH = 600U;

CUDPController::CUDPController(const std::string& modemAddress, unsigned int modemPort, unsigned int localPort) :
m_socket(localPort),
m_addr(),
m_addrLen(0U),
m_buffer(NULL),
m_length(0U),
m_offset(0U)
{
	assert(!modemAddress.empty());
	assert(modemPort > 0U);
	assert(localPort > 0U);

	if (CUDPSocket::lookup(modemAddress, modemPort, m_addr, m_addrLen) != 0)
		m_addrLen = 0U;

	m_buffer = new unsigned char[BUFFER_LENGTH];
}

CUDPController::~CUDPController()
{
	delete[] m_buffer;
}

bool CUDPController::open()
{
	if (m_addrLen == 0U) {
		LogError("Unable to resolve the address of the modem");
		return false;
	}

	return m_socket.open(m_addr);
}

int CUDPController::read(unsigned char* buffer, unsigned int length)
{
	assert(buffer != NULL);
	assert(length > 0U);

	return 0;
}

int CUDPController::write(const unsigned char* buffer, unsigned int length)
{
	assert(buffer != NULL);
	assert(length > 0U);

	return m_socket.write(buffer, length, m_addr, m_addrLen) ? int(length) : -1;
}

void CUDPController::close()
{
	m_socket.close();
}
