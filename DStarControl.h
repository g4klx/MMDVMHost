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
#include "StopWatch.h"
#include "RingBuffer.h"
#include "AMBEFEC.h"
#include "Display.h"
#include "Defines.h"
#include "Timer.h"
#include "Modem.h"

#include <string>

class CDStarControl {
public:
	CDStarControl(const std::string& callsign, const std::string& module, CDStarNetwork* network, IDisplay* display, unsigned int timeout, bool duplex);
	~CDStarControl();

	void writeModem(unsigned char* data);

	unsigned int readModem(unsigned char* data);

	void clock(unsigned int ms);

private:
	unsigned char*             m_callsign;
	unsigned char*             m_gateway;
	CDStarNetwork*             m_network;
	IDisplay*                  m_display;
	bool                       m_duplex;
	CRingBuffer<unsigned char> m_queue;
	RPT_STATE                  m_state;
	bool                       m_net;
	CDStarSlowData             m_slowData;
	unsigned char              m_n;
	CTimer                     m_networkWatchdog;
	CTimer                     m_holdoffTimer;
	CTimer                     m_timeoutTimer;
	CTimer                     m_packetTimer;
	CStopWatch                 m_elapsed;
	unsigned int               m_frames;
	unsigned int               m_lost;
	CAMBEFEC                   m_fec;
	unsigned int               m_bits;
	unsigned int               m_errs;
	unsigned char*             m_lastFrame;
	FILE*                      m_fp;

	void writeNetwork();

	void writeQueueHeader(const unsigned char* data);
	void writeQueueData(const unsigned char* data);
	void writeNetworkHeader(const unsigned char* data, bool busy);
	void writeNetworkData(const unsigned char* data, unsigned int errors, bool end, bool busy);

	void writeEndOfTransmission();

	bool openFile();
	bool writeFile(const unsigned char* data, unsigned int length);
	void closeFile();

	void insertSilence(const unsigned char* data, unsigned char seqNo);
	void insertSilence(unsigned int count);

	void blankDTMF(unsigned char* data) const;
	unsigned int matchSync(const unsigned char* data) const;
};

#endif
