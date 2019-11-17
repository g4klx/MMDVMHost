/*
 *   Copyright (C) 2015-2019 by Jonathan Naylor G4KLX
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

#include "RSSIInterpolator.h"
#include "DMREmbeddedData.h"
#include "DMRNetwork.h"
#include "DMRTA.h"
#include "RingBuffer.h"
#include "StopWatch.h"
#include "DMRLookup.h"
#include "AMBEFEC.h"
#include "DMRSlot.h"
#include "DMRData.h"
#include "Display.h"
#include "Defines.h"
#include "Timer.h"
#include "Modem.h"
#include "DMRLC.h"

#include <vector>

enum ACTIVITY_TYPE {
	ACTIVITY_NONE,
	ACTIVITY_VOICE,
	ACTIVITY_DATA,
	ACTIVITY_CSBK,
	ACTIVITY_EMERG
};

class CDMRSlot {
public:
	CDMRSlot(unsigned int slotNo, unsigned int timeout);
	~CDMRSlot();

	bool writeModem(unsigned char* data, unsigned int len);

	unsigned int readModem(unsigned char* data);

	void writeNetwork(const CDMRData& data);

	void clock();

	bool isBusy() const;

	void enable(bool enabled);

	static void init(unsigned int colorCode, bool embeddedLCOnly, bool dumpTAData, unsigned int callHang, CModem* modem, CDMRNetwork* network, CDisplay* display, bool duplex, CDMRLookup* lookup, CRSSIInterpolator* rssiMapper, unsigned int jitter, DMR_OVCM_TYPES ovcm);

private:
	unsigned int               m_slotNo;
	CRingBuffer<unsigned char> m_queue;
	RPT_RF_STATE               m_rfState;
	RPT_NET_STATE              m_netState;
	CDMREmbeddedData           m_rfEmbeddedLC;
	CDMREmbeddedData*          m_rfEmbeddedData;
	unsigned int               m_rfEmbeddedReadN;
	unsigned int               m_rfEmbeddedWriteN;
	unsigned char              m_rfTalkerId;
	CDMRTA                     m_rfTalkerAlias;
	CDMREmbeddedData           m_netEmbeddedLC;
	CDMREmbeddedData*          m_netEmbeddedData;
	unsigned int               m_netEmbeddedReadN;
	unsigned int               m_netEmbeddedWriteN;
	unsigned char              m_netTalkerId;
	CDMRLC*                    m_rfLC;
	CDMRLC*                    m_netLC;
	unsigned char              m_rfSeqNo;
	unsigned char              m_rfN;
	unsigned char              m_lastrfN;
	unsigned char              m_netN;
	CTimer                     m_networkWatchdog;
	CTimer                     m_rfTimeoutTimer;
	CTimer                     m_netTimeoutTimer;
	CTimer                     m_packetTimer;
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
	bool                       m_rfTimeout;
	bool                       m_netTimeout;
	unsigned char*             m_lastFrame;
	bool                       m_lastFrameValid;
	unsigned char              m_rssi;
	unsigned char              m_maxRSSI;
	unsigned char              m_minRSSI;
	unsigned int               m_aveRSSI;
	unsigned int               m_rssiCount;
	bool                       m_enabled;
	FILE*                      m_fp;

	static unsigned int        m_colorCode;

	static bool                m_embeddedLCOnly;
	static bool                m_dumpTAData;

	static CModem*             m_modem;
	static CDMRNetwork*        m_network;
	static CDisplay*           m_display;
	static bool                m_duplex;
	static CDMRLookup*         m_lookup;
	static unsigned int        m_hangCount;
	static DMR_OVCM_TYPES      m_ovcm;

	static CRSSIInterpolator*  m_rssiMapper;

	static unsigned int        m_jitterTime;
	static unsigned int        m_jitterSlots;

	static unsigned char*      m_idle;

    static FLCO                m_flco1;
	static unsigned char       m_id1;
	static ACTIVITY_TYPE       m_activity1;
	static FLCO                m_flco2;
	static unsigned char       m_id2;
	static ACTIVITY_TYPE       m_activity2;

	void logGPSPosition(const unsigned char* data);

	void writeQueueRF(const unsigned char* data);
	void writeQueueNet(const unsigned char* data);
	void writeNetworkRF(const unsigned char* data, unsigned char dataType, unsigned char errors = 0U);
	void writeNetworkRF(const unsigned char* data, unsigned char dataType, FLCO flco, unsigned int srcId, unsigned int dstId, unsigned char errors = 0U);

	void writeEndRF(bool writeEnd = false);
	void writeEndNet(bool writeEnd = false);

	bool openFile();
	bool writeFile(const unsigned char* data);
	void closeFile();

	bool insertSilence(const unsigned char* data, unsigned char seqNo);
	void insertSilence(unsigned int count);

	static void setShortLC(unsigned int slotNo, unsigned int id, FLCO flco = FLCO_GROUP, ACTIVITY_TYPE type = ACTIVITY_NONE);
};

#endif
