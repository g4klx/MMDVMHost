/*
 *   Copyright (C) 2015,2016,2017 by Jonathan Naylor G4KLX
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

#if !defined(YSFControl_H)
#define	YSFControl_H

#include "RSSIInterpolator.h"
#include "YSFNetwork.h"
#include "YSFDefines.h"
#include "YSFPayload.h"
#include "RingBuffer.h"
#include "StopWatch.h"
#include "YSFFICH.h"
#include "Display.h"
#include "Defines.h"
#include "Timer.h"
#include "Modem.h"

#include <string>

class CYSFControl {
public:
	CYSFControl(const std::string& callsign, bool selfOnly, CYSFNetwork* network, CDisplay* display, unsigned int timeout, bool duplex, bool lowDeviation, bool remoteGateway, CRSSIInterpolator* rssiMapper);
	~CYSFControl();

	void setSQL(bool on, unsigned char value);

	bool writeModem(unsigned char* data, unsigned int len);

	unsigned int readModem(unsigned char* data);

	void clock(unsigned int ms);

private:
	unsigned char*             m_callsign;
	unsigned char*             m_selfCallsign;
	bool                       m_selfOnly;
	CYSFNetwork*               m_network;
	CDisplay*                  m_display;
	bool                       m_duplex;
	bool                       m_lowDeviation;
	bool                       m_remoteGateway;
	bool                       m_sqlEnabled;
	unsigned char              m_sqlValue;
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
	unsigned int               m_netLost;
	unsigned int               m_rfErrs;
	unsigned int               m_rfBits;
	unsigned int               m_netErrs;
	unsigned int               m_netBits;
	unsigned char*             m_rfSource;
	unsigned char*             m_rfDest;
	unsigned char*             m_netSource;
	unsigned char*             m_netDest;
	CYSFFICH                   m_lastFICH;
	unsigned char              m_netN;
	CYSFPayload                m_rfPayload;
	CYSFPayload                m_netPayload;
	CRSSIInterpolator*         m_rssiMapper;
	unsigned char              m_rssi;
	unsigned char              m_maxRSSI;
	unsigned char              m_minRSSI;
	unsigned int               m_aveRSSI;
	unsigned int               m_rssiCount;
	FILE*                      m_fp;

	bool processVWData(bool valid, unsigned char *data);
	bool processDNData(bool valid, unsigned char *data);
	bool processFRData(bool valid, unsigned char *data);

	void writeQueueRF(const unsigned char* data);
	void writeQueueNet(const unsigned char* data);
	void writeNetwork(const unsigned char* data, unsigned int count);
	void writeNetwork();

	void writeEndRF();
	void writeEndNet();

	bool openFile();
	bool writeFile(const unsigned char* data);
	void closeFile();

	bool checkCallsign(const unsigned char* callsign) const;
	void processNetCallsigns(const unsigned char* data);
};

#endif
