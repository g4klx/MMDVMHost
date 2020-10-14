/*
 *   Copyright (C) 2015-2020 by Jonathan Naylor G4KLX
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

#if !defined(M17Control_H)
#define	M17Control_H

#include "RSSIInterpolator.h"
#include "M17Network.h"
#include "M17Defines.h"
#include "RingBuffer.h"
#include "StopWatch.h"
#include "Display.h"
#include "Defines.h"
#include "Timer.h"
#include "Modem.h"

#include <string>

class CM17Control {
public:
	CM17Control(const std::string& callsign, bool selfOnly, CM17Network* network, CDisplay* display, unsigned int timeout, bool duplex, CRSSIInterpolator* rssiMapper);
	~CM17Control();

	bool writeModem(unsigned char* data, unsigned int len);

	unsigned int readModem(unsigned char* data);

	void clock(unsigned int ms);

	bool isBusy() const;

	void enable(bool enabled);

private:
	std::string                m_callsign;
	bool                       m_selfOnly;
	CM17Network*               m_network;
	CDisplay*                  m_display;
	bool                       m_duplex;
	CRingBuffer<unsigned char> m_queue;
	RPT_RF_STATE               m_rfState;
	RPT_NET_STATE              m_netState;
	CTimer                     m_rfTimeoutTimer;
	CTimer                     m_netTimeoutTimer;
	CTimer                     m_packetTimer;
	CTimer                     m_networkWatchdog;
	CStopWatch                 m_elapsed;
	unsigned int               m_rfFrames;
	unsigned int               m_netFrames;
	unsigned int               m_rfErrs;
	unsigned int               m_rfBits;
	CNXDNLICH                  m_rfLastLICH;
	CNXDNLayer3                m_rfLayer3;
	CNXDNLayer3                m_netLayer3;
	unsigned char              m_rfMask;
	unsigned char              m_netMask;
	CRSSIInterpolator* m_rssiMapper;
	unsigned char              m_rssi;
	unsigned char              m_maxRSSI;
	unsigned char              m_minRSSI;
	unsigned int               m_aveRSSI;
	unsigned int               m_rssiCount;
	bool                       m_enabled;
	FILE*                      m_fp;

	bool processVoice(unsigned char usc, unsigned char option, unsigned char* data);

	void writeQueueRF(const unsigned char* data);
	void writeQueueNet(const unsigned char* data);
	void writeNetwork(const unsigned char* data);
	void writeNetwork();

	void scrambler(unsigned char* data) const;

	void writeEndRF();
	void writeEndNet();

	bool openFile();
	bool writeFile(const unsigned char* data);
	void closeFile();
};

#endif
