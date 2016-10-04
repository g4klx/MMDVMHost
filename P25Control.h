/*
*   Copyright (C) 2016 by Jonathan Naylor G4KLX
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

#if !defined(P25Control_H)
#define	P25Control_H

#include "P25LowSpeedData.h"
#include "RingBuffer.h"
#include "P25Network.h"
#include "DMRLookup.h"
#include "P25Audio.h"
#include "Defines.h"
#include "Display.h"
#include "P25Data.h"
#include "P25NID.h"
#include "Modem.h"
#include "Timer.h"

#include <cstdio>

class CP25Control {
public:
	CP25Control(unsigned int nac, CP25Network* network, CDisplay* display, unsigned int timeout, bool duplex, CDMRLookup* lookup);
	~CP25Control();

	bool writeModem(unsigned char* data, unsigned int len);

	unsigned int readModem(unsigned char* data);

	void clock(unsigned int ms);

private:
	unsigned int   m_nac;
	CP25Network*   m_network;
	CDisplay*      m_display;
	bool           m_duplex;
	CDMRLookup*    m_lookup;
	CRingBuffer<unsigned char> m_queue;
	RPT_RF_STATE   m_rfState;
	RPT_NET_STATE  m_netState;
	CTimer         m_rfTimeout;
	CTimer         m_netTimeout;
	CTimer         m_networkWatchdog;
	unsigned int   m_rfFrames;
	unsigned int   m_rfBits;
	unsigned int   m_rfErrs;
	unsigned int   m_netFrames;
	unsigned int   m_netLost;
	CP25NID        m_nid;
	unsigned char  m_lastDUID;
	CP25Audio      m_audio;
	CP25Data       m_rfData;
	CP25Data       m_netData;
	CP25LowSpeedData m_rfLSD;
	CP25LowSpeedData m_netLSD;
	unsigned char* m_netLDU1;
	unsigned char* m_netLDU2;
	unsigned char* m_lastIMBE;
	unsigned char* m_rfLDU;
	FILE*          m_fp;

	void writeQueueRF(const unsigned char* data, unsigned int length);
	void writeQueueNet(const unsigned char* data, unsigned int length);
	void writeNetwork(const unsigned char *data, unsigned char type, bool end);
	void writeNetwork();

	void addBusyBits(unsigned char* data, unsigned int length, bool b1, bool b2);

	void checkNetLDU1();
	void checkNetLDU2();

	void insertMissingAudio(unsigned char* data);

	void createRFHeader();

	void createNetHeader();
	void createNetLDU1();
	void createNetLDU2();
	void createNetTerminator();

	bool openFile();
	bool writeFile(const unsigned char* data, unsigned char length);
	void closeFile();
};

#endif
