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

#ifndef	FMIAXNetwork_H
#define	FMIAXNetwork_H

#include "FMNetwork.h"
#include "RingBuffer.h"
#include "UDPSocket.h"
#include "StopWatch.h"
#include "Timer.h"

#include <cstdint>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
#include <wincrypt.h>
#else
#include <md5.h>
#endif

enum IAX_STATUS {
	IAXS_DISCONNECTED,
	IAXS_CONNECTING,
	IAXS_REGISTERING,
	IAXS_CONNECTED
};

class CFMIAXNetwork : public IFMNetwork {
public:
	CFMIAXNetwork(const std::string& callsign, const std::string& username, const std::string& password, const std::string& node, const std::string& localAddress, unsigned short localPort, const std::string& gatewayAddress, unsigned short gatewayPort, bool debug);
	virtual ~CFMIAXNetwork();

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
	std::string         m_username;
	std::string         m_password;
	std::string         m_node;
	CUDPSocket          m_socket;
	sockaddr_storage    m_addr;
	unsigned int        m_addrLen;
	bool                m_debug;
	bool                m_enabled;
	CRingBuffer<unsigned char> m_buffer;
	IAX_STATUS          m_status;
	CTimer              m_retryTimer;
	CTimer              m_pingTimer;
	std::string         m_seed;
	CStopWatch          m_timestamp;
	unsigned short      m_sCallNo;
	unsigned short      m_dCallNo;
	unsigned char       m_iSeqNo;
	unsigned char       m_oSeqNo;
	unsigned int        m_rxJitter;
	unsigned int        m_rxLoss;
	unsigned int        m_rxFrames;
	unsigned short      m_rxDelay;
	unsigned int        m_rxDropped;
	unsigned int        m_rxOOO;
	bool                m_keyed;
#if defined(_WIN32) || defined(_WIN64)
	HCRYPTPROV          m_provider;
#endif

	bool writeNew(bool retry);
	bool writeAuthRep();
	bool writeKey(bool key);
	bool writePing();
	bool writePong(unsigned int ts);
	bool writeAck(unsigned int ts);
	bool writeLagRq();
	bool writeLagRp(unsigned int ts);
	bool writeHangup();
	bool writeRegReq(bool retry);
	bool writeAudio(const short* audio, unsigned int length);

	void uLawEncode(const short* audio, unsigned char* buffer, unsigned int length) const;
	void uLawDecode(const unsigned char* buffer, short* audio, unsigned int length) const;

	bool compareFrame(const unsigned char* buffer, unsigned char type1, unsigned char type2) const;
};

#endif

