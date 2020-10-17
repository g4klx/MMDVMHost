/*
 *   Copyright (C) 2009-2014,2016,2018,2020 by Jonathan Naylor G4KLX
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

#ifndef	M17Network_H
#define	M17Network_H

#include "M17Defines.h"
#include "RingBuffer.h"
#include "UDPSocket.h"
#include "Timer.h"

#include <random>
#include <cstdint>

enum M17NET_STATUS {
	M17N_NOTLINKED,
	M17N_LINKING,
	M17N_LINKED,
	M17N_UNLINKING
};

class CM17Network {
public:
	CM17Network(unsigned int port, bool debug);
	~CM17Network();

	bool open();

	bool link(const std::string& address, unsigned int port, const std::string& reflector, char module);

	void unlink();

	void enable(bool enabled);

	bool write(const unsigned char* data);

	bool read(unsigned char* data);

	void reset();

	void close();

	void clock(unsigned int ms);

private:
	CUDPSocket       m_socket;
	sockaddr_storage m_addr;
	unsigned int     m_addrLen;
	bool             m_debug;
	bool             m_enabled;
	uint16_t         m_outId;
	uint16_t         m_inId;
	CRingBuffer<unsigned char> m_buffer;
	std::mt19937     m_random;
	M17NET_STATUS    m_state;
	std::string      m_reflector;
	unsigned char*   m_encoded;
	char             m_module;
	CTimer           m_timer;

	void sendConnect();
	void sendDisconnect();
	void sendPong();
};

#endif
