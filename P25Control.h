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

class CP25Control {
public:
	CP25Control(unsigned int nac, CP25Network* network, CDisplay* display, unsigned int timeout, bool duplex, CDMRLookup* lookup, int rssiMultiplier, int rssiOffset);
	~CP25Control();

	bool writeModem(unsigned char* data, unsigned int len);

	unsigned int readModem(unsigned char* data);

	void clock(unsigned int ms);

private:
	unsigned int  m_nac;
	CP25Network*  m_network;
	CDisplay*     m_display;
	bool          m_duplex;
	CDMRLookup*   m_lookup;
	int           m_rssiMultiplier;
	int           m_rssiOffset;
	CRingBuffer<unsigned char> m_queue;
	RPT_RF_STATE  m_rfState;
	RPT_NET_STATE m_netState;
	CTimer        m_rfTimeout;
	CTimer        m_netTimeout;
	unsigned int  m_rfFrames;
	unsigned int  m_rfBits;
	unsigned int  m_rfErrs;
	CP25NID       m_nid;
	CP25Audio     m_audio;
	CP25Data      m_rfData;
	CP25Data      m_netData;

	void writeQueueRF(const unsigned char* data, unsigned int length);
	void addBusyBits(unsigned char* data, unsigned int length, bool b1, bool b2);
};

#endif
