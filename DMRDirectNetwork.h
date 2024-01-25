/*
 *   Copyright (C) 2015,2016,2017,2018,2020,2021 by Jonathan Naylor G4KLX
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

#if !defined(DMRDirectNetwork_H)
#define	DMRDirectNetwork_H

#include "DMRNetwork.h"
#include "UDPSocket.h"
#include "Timer.h"
#include "RingBuffer.h"
#include "DMRData.h"

#include <string>
#include <cstdint>
#include <random>

class CDMRDirectNetwork : public IDMRNetwork
{
public:
	CDMRDirectNetwork(const std::string& remoteAddress, unsigned short remotePort, const std::string& localAddress, unsigned short localPort, unsigned int id, const std::string& password, bool duplex, const char* version, bool slot1, bool slot2, HW_TYPE hwType, bool debug);
	virtual ~CDMRDirectNetwork();

	virtual void setOptions(const std::string& options);

	virtual void setConfig(const std::string& callsign, unsigned int rxFrequency, unsigned int txFrequency, unsigned int power, unsigned int colorCode, float latitude, float longitude, int height, const std::string& location, const std::string& description, const std::string& url);

	virtual bool open();

	virtual void enable(bool enabled);

	virtual bool read(CDMRData& data);

	virtual bool write(const CDMRData& data);

	virtual bool writeRadioPosition(unsigned int id, const unsigned char* data);

	virtual bool writeTalkerAlias(unsigned int id, unsigned char type, const unsigned char* data);

	virtual bool wantsBeacon();

	virtual void clock(unsigned int ms);

	virtual bool isConnected() const;

	virtual void close(bool sayGoodbye);

private:
	std::string      m_address;
	unsigned short   m_port;
	sockaddr_storage m_addr;
	unsigned int     m_addrLen;
	uint8_t*         m_id;
	std::string      m_password;
	bool             m_duplex;
	const char*      m_version;
	bool             m_debug;
	CUDPSocket       m_socket;
	bool             m_enabled;
	bool             m_slot1;
	bool             m_slot2;
	HW_TYPE          m_hwType;

	enum STATUS {
		WAITING_CONNECT,
		WAITING_LOGIN,
		WAITING_AUTHORISATION,
		WAITING_CONFIG,
		WAITING_OPTIONS,
		RUNNING
	};

	STATUS         m_status;
	CTimer         m_retryTimer;
	CTimer         m_timeoutTimer;
	unsigned char* m_buffer;
	uint32_t*      m_streamId;
	unsigned char* m_salt;

	CRingBuffer<unsigned char> m_rxData;

	std::string    m_options;

	std::mt19937   m_random;
	std::string    m_callsign;
	unsigned int   m_rxFrequency;
	unsigned int   m_txFrequency;
	unsigned int   m_power;
	unsigned int   m_colorCode;
	float          m_latitude;
	float          m_longitude;
	int            m_height;
	std::string    m_location;
	std::string    m_description;
	std::string    m_url;
	bool           m_beacon;

	bool writeLogin();
	bool writeAuthorisation();
	bool writeOptions();
	bool writeConfig();
	bool writePing();

	bool write(const unsigned char* data, unsigned int length);
};

#endif
