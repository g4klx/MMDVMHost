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

#ifndef	P25Network_H
#define	P25Network_H

#include "P25LowSpeedData.h"
#include "RingBuffer.h"
#include "UDPSocket.h"
#include "P25Audio.h"
#include "P25Data.h"

#include <cstdint>
#include <string>

class CP25Network {
public:
	CP25Network(const std::string& gatewayAddress, unsigned int gatewayPort, unsigned int localPort, bool debug);
	~CP25Network();

	bool open();

	void enable(bool enabled);

	bool writeLDU1(const unsigned char* ldu1, const CP25Data& control, const CP25LowSpeedData& lsd, bool end);

	bool writeLDU2(const unsigned char* ldu2, const CP25Data& control, const CP25LowSpeedData& lsd, bool end);

	unsigned int read(unsigned char* data, unsigned int length);

	void close();

	void clock(unsigned int ms);

private:
	CUDPSocket     m_socket;
	in_addr        m_address;
	unsigned int   m_port;
	bool           m_debug;
	bool           m_enabled;
	CRingBuffer<unsigned char> m_buffer;
	CP25Audio      m_audio;
};

#endif
