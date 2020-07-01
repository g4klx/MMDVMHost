/*
 *   Copyright (C) 2009-2014,2016 by Jonathan Naylor G4KLX
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

#ifndef	DStarNetwork_H
#define	DStarNetwork_H

#include "DStarDefines.h"
#include "RingBuffer.h"
#include "UDPSocket.h"
#include "Timer.h"

#include <cstdint>
#include <string>
#include <random>

class CDStarNetwork {
public:
	CDStarNetwork(const std::string& gatewayAddress, unsigned int gatewayPort, unsigned int localPort, bool duplex, const char* version, bool debug);
	~CDStarNetwork();

	bool open();

	void enable(bool enabled);

	bool writeHeader(const unsigned char* header, unsigned int length, bool busy);
	bool writeData(const unsigned char* data, unsigned int length, unsigned int errors, bool end, bool busy);

	void getStatus(LINK_STATUS& status, unsigned char* reflector);

	unsigned int read(unsigned char* data, unsigned int length);

	void reset();

	void close();

	void clock(unsigned int ms);

private:
	CUDPSocket     m_socket;
	in_addr        m_address;
	unsigned int   m_port;
	bool           m_duplex;
	const char*    m_version;
	bool           m_debug;
	bool           m_enabled;
	uint16_t       m_outId;
	uint8_t        m_outSeq;
	uint16_t       m_inId;
	CRingBuffer<unsigned char> m_buffer;
	CTimer         m_pollTimer;
	LINK_STATUS    m_linkStatus;
	unsigned char* m_linkReflector;
	std::mt19937   m_random;

	bool writePoll(const char* text);
};

#endif
