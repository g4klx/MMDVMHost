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

#if !defined(DMRSlot_H)
#define	DMRSlot_H

#include "HomebrewDMRIPSC.h"
#include "StopWatch.h"
#include "EmbeddedLC.h"
#include "RingBuffer.h"
#include "AMBEFEC.h"
#include "DMRSlot.h"
#include "DMRData.h"
#include "Display.h"
#include "Defines.h"
#include "Timer.h"
#include "Modem.h"
#include "EMB.h"
#include "LC.h"

class CDMRSlot {
public:
	CDMRSlot(unsigned int slotNo, unsigned int timeout);
	~CDMRSlot();

	void writeModem(unsigned char* data);

	unsigned int readModem(unsigned char* data);

	void writeNetwork(const CDMRData& data);

	void clock(unsigned int ms);

	static void init(unsigned int colorCode, CModem* modem, CHomebrewDMRIPSC* network, IDisplay* display, bool duplex);

private:
	unsigned int               m_slotNo;
	CRingBuffer<unsigned char> m_queue;
	RPT_STATE                  m_state;
	CEmbeddedLC                m_embeddedLC;
	CLC*                       m_lc;
	unsigned char              m_seqNo;
	unsigned char              m_n;
	CTimer                     m_networkWatchdog;
	CTimer                     m_timeoutTimer;
	CTimer                     m_packetTimer;
	CStopWatch                 m_elapsed;
	unsigned int               m_frames;
	unsigned int               m_lost;
	CAMBEFEC                   m_fec;
	unsigned int               m_bits;
	unsigned int               m_errs;
	unsigned char*             m_lastFrame;
	CEMB                       m_lastEMB;
	FILE*                      m_fp;

	static unsigned int        m_colorCode;
	static CModem*             m_modem;
	static CHomebrewDMRIPSC*   m_network;
	static IDisplay*           m_display;
	static bool                m_duplex;

	static unsigned char*      m_idle;

	static FLCO                m_flco1;
	static unsigned char       m_id1;
	static bool                m_voice1;
	static FLCO                m_flco2;
	static unsigned char       m_id2;
	static bool                m_voice2;

	void writeQueue(const unsigned char* data);
	void writeNetwork(const unsigned char* data, unsigned char dataType);
	void writeNetwork(const unsigned char* data, unsigned char dataType, FLCO flco, unsigned int srcId, unsigned int dstId);

	void writeEndOfTransmission(bool writeEnd = false);

	bool openFile();
	bool writeFile(const unsigned char* data);
	void closeFile();

	void insertSilence(const unsigned char* data, unsigned char seqNo);
	void insertSilence(unsigned int count);

	static void setShortLC(unsigned int slotNo, unsigned int id, FLCO flco = FLCO_GROUP, bool voice = true);
};

#endif
