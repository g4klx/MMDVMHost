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
#include "Display.h"
#include "Defines.h"
#include "Timer.h"
#include "Modem.h"

#include <string>

class CYSFControl {
public:
	CYSFControl(const std::string& callsign, CYSFNetwork* network, CDisplay* display, unsigned int timeout, bool duplex);
	~CYSFControl();

	bool writeModem(unsigned char* data);

	unsigned int readModem(unsigned char* data);

	void clock(unsigned int ms);

private:
	CYSFNetwork*               m_network;
	CDisplay*                  m_display;
	bool                       m_duplex;
	CRingBuffer<unsigned char> m_queue;
	RPT_RF_STATE               m_rfState;
	RPT_NET_STATE              m_netState;
	CTimer                     m_rfTimeoutTimer;
	CTimer                     m_netTimeoutTimer;
	CTimer                     m_networkWatchdog;
	CTimer                     m_holdoffTimer;
	unsigned int               m_rfFrames;
	unsigned int               m_netFrames;
	unsigned int               m_rfErrs;
	unsigned int               m_rfBits;
	unsigned int               m_netErrs;
	unsigned int               m_netBits;
	unsigned char*             m_rfSource;
	unsigned char*             m_rfDest;
	unsigned char*             m_netSource;
	unsigned char*             m_netDest;
	CYSFPayload                m_rfPayload;
	CYSFPayload                m_netPayload;
	unsigned char              m_netSeqNo;
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
};

#endif
