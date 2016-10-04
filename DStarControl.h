/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
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

#if !defined(DStarControl_H)
#define	DStarControl_H

#include "DStarNetwork.h"
#include "DStarSlowData.h"
#include "DStarDefines.h"
#include "DStarHeader.h"
#include "RingBuffer.h"
#include "StopWatch.h"
#include "AMBEFEC.h"
#include "Display.h"
#include "Defines.h"
#include "Timer.h"
#include "Modem.h"

#include <string>
#include <vector>

class CDStarControl {
public:
	CDStarControl(const std::string& callsign, const std::string& module, bool selfOnly, const std::vector<std::string>& blackList, CDStarNetwork* network, CDisplay* display, unsigned int timeout, bool duplex);
	~CDStarControl();

	bool writeModem(unsigned char* data, unsigned int len);

	unsigned int readModem(unsigned char* data);

	void clock();

private:
	unsigned char*             m_callsign;
	unsigned char*             m_gateway;
	bool                       m_selfOnly;
	std::vector<std::string>   m_blackList;
	CDStarNetwork*             m_network;
	CDisplay*                  m_display;
	bool                       m_duplex;
	CRingBuffer<unsigned char> m_queue;
	CDStarHeader               m_rfHeader;
	CDStarHeader               m_netHeader;
	RPT_RF_STATE               m_rfState;
	RPT_NET_STATE              m_netState;
	bool                       m_net;
	CDStarSlowData             m_slowData;
	unsigned char              m_rfN;
	unsigned char              m_netN;
	CTimer                     m_networkWatchdog;
	CTimer                     m_rfTimeoutTimer;
	CTimer                     m_netTimeoutTimer;
	CTimer                     m_packetTimer;
	CTimer                     m_ackTimer;
	CStopWatch                 m_interval;
	CStopWatch                 m_elapsed;
	unsigned int               m_rfFrames;
	unsigned int               m_netFrames;
	unsigned int               m_netLost;
	CAMBEFEC                   m_fec;
	unsigned int               m_rfBits;
	unsigned int               m_netBits;
	unsigned int               m_rfErrs;
	unsigned int               m_netErrs;
	unsigned char*             m_lastFrame;
	bool                       m_lastFrameValid;
	FILE*                      m_fp;

	void writeNetwork();

	void writeQueueHeaderRF(const unsigned char* data);
	void writeQueueDataRF(const unsigned char* data);
	void writeQueueEOTRF();
	void writeQueueHeaderNet(const unsigned char* data);
	void writeQueueDataNet(const unsigned char* data);
	void writeQueueEOTNet();
	void writeNetworkHeaderRF(const unsigned char* data);
	void writeNetworkDataRF(const unsigned char* data, unsigned int errors, bool end);

	void writeEndRF();
	void writeEndNet();

	bool openFile();
	bool writeFile(const unsigned char* data, unsigned int length);
	void closeFile();

	bool insertSilence(const unsigned char* data, unsigned char seqNo);
	void insertSilence(unsigned int count);

	void blankDTMF(unsigned char* data) const;

	void sendAck();
};

#endif
