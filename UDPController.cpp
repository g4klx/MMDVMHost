/*
 *   Copyright (C) 2021,2025 by Jonathan Naylor G4KLX
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

CUDPController::CUDPController(const std::string& modemAddress, unsigned int modemPort, const std::string& localAddress, unsigned int localPort) :
m_socket(localAddress, localPort),
m_addr(),
m_addrLen(0U),
m_buffer(2000U, "UDP Controller Ring Buffer")
{
	assert(!modemAddress.empty());
	assert(modemPort > 0U);
	assert(localPort > 0U);

	if (CUDPSocket::lookup(modemAddress, modemPort, m_addr, m_addrLen) != 0)
		m_addrLen = 0U;
}

CUDPController::~CUDPController()
{
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
    assert(buffer != nullptr);
    assert(length > 0U);

    unsigned char data[BUFFER_LENGTH];
    sockaddr_storage addr;
    unsigned int addrLen;
    int ret = m_socket.read(data, BUFFER_LENGTH, addr, addrLen);

    // An error occurred on the socket
    if (ret < 0)
        return ret;

    // Add new data to the ring buffer
    if (ret > 0) {
        if (CUDPSocket::match(addr, m_addr))
            m_buffer.addData(data, ret);
    }

    // Get required data from the ring buffer
    unsigned int avail = m_buffer.dataSize();
    if (avail < length)
        length = avail;

    if (length > 0U)
        m_buffer.getData(buffer, length);

    return int(length);
}

int CUDPController::write(const unsigned char* buffer, unsigned int length)
{
	assert(buffer != nullptr);
	assert(length > 0U);

	return m_socket.write(buffer, length, m_addr, m_addrLen) ? int(length) : -1;
}

void CUDPController::close()
{
	m_socket.close();
}
