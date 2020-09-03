/*
 *   Copyright (C) 2015,2016,2017,2018,2020 by Jonathan Naylor G4KLX
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

#if !defined(DMRNetwork_H)
#define	DMRNetwork_H

#include "UDPSocket.h"
#include "Timer.h"
#include "RingBuffer.h"
#include "DMRData.h"
#include "Defines.h"

#include <string>
#include <cstdint>
#include <random>

class CDMRNetwork
{
public:
	CDMRNetwork(const std::string& address, unsigned int port, unsigned int local, unsigned int id, bool duplex, const char* version, bool debug, bool slot1, bool slot2, HW_TYPE hwType);
	~CDMRNetwork();

	void setConfig(const std::string& callsign, unsigned int rxFrequency, unsigned int txFrequency, unsigned int power, unsigned int colorCode);

	bool open();

	void enable(bool enabled);

	bool read(CDMRData& data);

	bool write(const CDMRData& data);

	bool writeRadioPosition(unsigned int id, const unsigned char* data);

	bool writeTalkerAlias(unsigned int id, unsigned char type, const unsigned char* data);

	bool wantsBeacon();

	void clock(unsigned int ms);

	void close();

private: 
	std::string     m_addressStr;
	sockaddr_storage m_address;
	unsigned int    m_addrlen;
	unsigned int    m_port;
	uint8_t*        m_id;
	bool            m_duplex;
	const char*     m_version;
	bool            m_debug;
	CUDPSocket      m_socket;
	bool            m_enabled;
	bool            m_slot1;
	bool            m_slot2;
	HW_TYPE         m_hwType;
	unsigned char*  m_buffer;
	uint32_t*       m_streamId;
	CRingBuffer<unsigned char> m_rxData;
	bool            m_beacon;
	std::mt19937    m_random;
	std::string     m_callsign;
	unsigned int    m_rxFrequency;
	unsigned int    m_txFrequency;
	unsigned int    m_power;
	unsigned int    m_colorCode;
	CTimer          m_pingTimer;

	bool writeConfig();

	bool write(const unsigned char* data, unsigned int length);
};

#endif
