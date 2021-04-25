/*
 *   Copyright (C) 2009-2014,2016,2020,2021 by Jonathan Naylor G4KLX
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

#ifndef	YSFNetwork_H
#define	YSFNetwork_H

#include "YSFDefines.h"
#include "RingBuffer.h"
#include "UDPSocket.h"
#include "Timer.h"

#include <cstdint>
#include <string>

class CYSFNetwork {
public:
	CYSFNetwork(const std::string& localAddress, unsigned short localPort, const std::string& gatewayAddress, unsigned short gatewayPort, const std::string& callsign, bool debug);
	~CYSFNetwork();

	bool open();

	void enable(bool enabled);

	bool write(const unsigned char* src, const unsigned char* dest, const unsigned char* data, unsigned int count, bool end);

	unsigned int read(unsigned char* data);

	void reset();

	bool isConnected() const;

	void close();

	void clock(unsigned int ms);

private:
	CUDPSocket       m_socket;
	sockaddr_storage m_addr;
	unsigned int     m_addrLen;
	std::string      m_callsign;
	bool             m_debug;
	bool             m_enabled;
	CRingBuffer<unsigned char> m_buffer;
	CTimer           m_pollTimer;
	unsigned char*   m_tag;

	bool writePoll();
};

#endif
