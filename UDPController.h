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

#ifndef UDPController_H
#define UDPController_H

#include "ModemPort.h"
#include "RingBuffer.h"
#include "UDPSocket.h"

#include <string>

class CUDPController : public IModemPort {
public:
	CUDPController(const std::string& modemAddress, unsigned int modemPort, const std::string& localAddress, unsigned int localPort);
	virtual ~CUDPController();

	virtual bool open();

	virtual int read(unsigned char* buffer, unsigned int length);

	virtual int write(const unsigned char* buffer, unsigned int length);

	virtual void close();
	
#if defined(__APPLE__)
	int setNonblock(bool nonblock) { return 0; }
#endif

protected:
	CUDPSocket       m_socket;
	sockaddr_storage m_addr;
	unsigned int     m_addrLen;
	CRingBuffer<unsigned char> m_buffer;
};

#endif
