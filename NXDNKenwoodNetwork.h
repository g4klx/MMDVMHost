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

#ifndef	NXDNKenwoodNetwork_H
#define	NXDNKenwoodNetwork_H

#include "NXDNNetwork.h"
#include "UDPSocket.h"
#include "Timer.h"

#include <cstdint>
#include <string>
#include <random>

class CNXDNKenwoodNetwork : public INXDNNetwork {
public:
	CNXDNKenwoodNetwork(const std::string& localAddress, unsigned int localPort, const std::string& gwyAddress, unsigned int gwyPort, bool debug);
	virtual ~CNXDNKenwoodNetwork();

	virtual bool open();

    virtual void enable(bool enabled);

    virtual bool write(const unsigned char* data, NXDN_NETWORK_MESSAGE_TYPE type);

	virtual bool read(unsigned char* data);

    virtual void reset();

    virtual void close();

    virtual void clock(unsigned int ms);

private:
	CUDPSocket     m_rtpSocket;
    CUDPSocket     m_rtcpSocket;
    in_addr        m_address;
	unsigned int   m_rtcpPort;
    unsigned int   m_rtpPort;
    bool           m_enabled;
    bool           m_headerSeen;
    bool           m_seen1;
    bool           m_seen2;
    bool           m_seen3;
    bool           m_seen4;
    unsigned char* m_sacch;
    uint8_t        m_sessionId;
    uint16_t       m_seqNo;
    unsigned int   m_ssrc;
    bool           m_debug;
    uint32_t       m_startSecs;
    uint32_t       m_startUSecs;
    CTimer         m_rtcpTimer;
    CTimer         m_hangTimer;
    unsigned char  m_hangType;
    unsigned short m_hangSrc;
    unsigned short m_hangDst;
    std::mt19937   m_random;

    bool processIcomVoiceHeader(const unsigned char* data);
    bool processIcomVoiceData(const unsigned char* data);
    bool processIcomDataHeader(const unsigned char* data);
    bool processIcomDataData(const unsigned char* data);
    bool processIcomDataTrailer(const unsigned char* data);
    bool processKenwoodVoiceHeader(unsigned char* data);
    bool processKenwoodVoiceData(unsigned char* data);
    bool processKenwoodVoiceLateEntry(unsigned char* data);
    bool processKenwoodData(unsigned char* data);
    bool writeRTPVoiceHeader(const unsigned char* data);
    bool writeRTPVoiceData(const unsigned char* data);
    bool writeRTPVoiceTrailer(const unsigned char* data);
    bool writeRTPDataHeader(const unsigned char* data);
    bool writeRTPDataData(const unsigned char* data);
    bool writeRTPDataTrailer(const unsigned char* data);
    bool writeRTCPStart();
    bool writeRTCPPing();
    bool writeRTCPHang(unsigned char type, unsigned short src, unsigned short dst);
    bool writeRTCPHang();
    unsigned int readRTP(unsigned char* data);
    unsigned int readRTCP(unsigned char* data);
    unsigned long getTimeStamp() const;
};

#endif
