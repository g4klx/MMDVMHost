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

#if !defined(YSFControl_H)
#define	YSFControl_H

#include "YSFNetwork.h"
#include "YSFDefines.h"
#include "YSFPayload.h"
#include "RingBuffer.h"
#include "StopWatch.h"
#include "Display.h"
#include "Defines.h"
#include "Timer.h"
#include "Modem.h"

#include <string>

class CYSFControl {
public:
	CYSFControl(const std::string& callsign, CYSFNetwork* network, CDisplay* display, unsigned int timeout, bool duplex, bool remoteGateway);
	~CYSFControl();

	bool writeModem(unsigned char* data, unsigned int len);

	unsigned int readModem(unsigned char* data);

	void clock(unsigned int ms);

private:
	unsigned char*             m_callsign;
	CYSFNetwork*               m_network;
	CDisplay*                  m_display;
	bool                       m_duplex;
	bool                       m_remoteGateway;
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
	unsigned char*             m_lastFrame;
	bool                       m_lastFrameValid;
	unsigned char              m_lastMode;
	unsigned char              m_lastMR;
	unsigned char              m_netN;
	CYSFPayload                m_rfPayload;
	CYSFPayload                m_netPayload;
	FILE*                      m_fp;

	void writeQueueRF(const unsigned char* data);
	void writeQueueNet(const unsigned char* data);
	void writeNetwork(const unsigned char* data, unsigned int count);
	void writeNetwork();

	void writeEndRF();
	void writeEndNet();

	bool openFile();
	bool writeFile(const unsigned char* data);
	void closeFile();

	bool insertSilence(const unsigned char* data, unsigned char n);
	void insertSilence(unsigned int count);
};

#endif
