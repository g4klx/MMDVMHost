/*
 *   Copyright (C) 2020,2021,2023,2024 by Jonathan Naylor G4KLX
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

#ifndef	FMUSRPNetwork_H
#define	FMUSRPNetwork_H

#include "FMNetwork.h"
#include "RingBuffer.h"
#include "UDPSocket.h"

#include <cstdint>
#include <string>

class CFMUSRPNetwork : public IFMNetwork {
public:
	CFMUSRPNetwork(const std::string& callsign, const std::string& localAddress, unsigned short localPort, const std::string& gatewayAddress, unsigned short gatewayPort, bool debug);
	virtual ~CFMUSRPNetwork();

	virtual bool open();

	virtual void enable(bool enabled);

	virtual bool writeStart();

	virtual bool writeData(const float* data, unsigned int nSamples);

	virtual bool writeEnd();

	virtual unsigned int readData(float* out, unsigned int nOut);

	virtual void reset();

	virtual void close();

	virtual void clock(unsigned int ms);

private:
	std::string         m_callsign;
	CUDPSocket          m_socket;
	sockaddr_storage    m_addr;
	unsigned int        m_addrLen;
	bool                m_debug;
	bool                m_enabled;
	CRingBuffer<unsigned char> m_buffer;
	unsigned int        m_seqNo;
};

#endif

