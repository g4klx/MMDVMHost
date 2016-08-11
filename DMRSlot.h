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

#include "DMREmbeddedLC.h"
#include "DMRDataHeader.h"
#include "RingBuffer.h"
#include "StopWatch.h"
#include "DMRLookup.h"
#include "AMBEFEC.h"
#include "DMRSlot.h"
#include "DMRData.h"
#include "Display.h"
#include "Defines.h"
#include "DMRIPSC.h"
#include "DMREMB.h"
#include "Timer.h"
#include "Modem.h"
#include "DMRLC.h"

#include <vector>

class CDMRSlot {
public:
	CDMRSlot(unsigned int slotNo, unsigned int timeout);
	~CDMRSlot();

	void writeModem(unsigned char* data, unsigned int len);

	unsigned int readModem(unsigned char* data);

	void writeNetwork(const CDMRData& data);

	void clock();

	static void init(unsigned int id, unsigned int colorCode, unsigned int callHang, bool selfOnly, const std::vector<unsigned int>& prefixes, const std::vector<unsigned int>& SrcIdBlackList, const std::vector<unsigned int>& DstIdBlacklistSlot1RF, const std::vector<unsigned int>& DstIdWhitelistSlot1RF, const std::vector<unsigned int>& DstIdBlacklistSlot2RF, const std::vector<unsigned int>& DstIdWhitelistSlot2RF,  const std::vector<unsigned int>& DstIdBlacklistSlot1NET, const std::vector<unsigned int>& DstIdWhitelistSlot1NET, const std::vector<unsigned int>& DstIdBlacklistSlot2NET, const std::vector<unsigned int>& DstIdWhitelistSlot2NET, CModem* modem, CDMRIPSC* network, CDisplay* display, bool duplex, CDMRLookup* lookup, int rssiMultiplier, int rssiOffset);

private:
	unsigned int               m_slotNo;
	CRingBuffer<unsigned char> m_queue;
	RPT_RF_STATE               m_rfState;
	RPT_NET_STATE              m_netState;
	CDMREmbeddedLC             m_rfEmbeddedLC;
	CDMREmbeddedLC             m_netEmbeddedLC;
	CDMRLC*                    m_rfLC;
	CDMRLC*                    m_netLC;
	CDMRDataHeader             m_rfDataHeader;
	CDMRDataHeader             m_netDataHeader;
	unsigned char              m_rfSeqNo;
	unsigned char              m_netSeqNo;
	unsigned char              m_rfN;
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
	unsigned char*             m_lastFrame;
	bool                       m_lastFrameValid;
	CDMREMB                    m_lastEMB;
	unsigned char              m_rssi;
	FILE*                      m_fp;

	static unsigned int        m_id;
	static unsigned int        m_colorCode;
	static bool                m_selfOnly;
	static std::vector<unsigned int> m_prefixes;
	static std::vector<unsigned int> m_blackList;

	static CModem*             m_modem;
	static CDMRIPSC*           m_network;
	static CDisplay*           m_display;
	static bool                m_duplex;
	static CDMRLookup*         m_lookup;
	static unsigned int        m_hangCount;

	static int                 m_rssiMultiplier;
	static int                 m_rssiOffset;

	static unsigned char*      m_idle;

	static FLCO                m_flco1;
	static unsigned char       m_id1;
	static bool                m_voice1;
	static FLCO                m_flco2;
	static unsigned char       m_id2;
	static bool                m_voice2;

	void writeQueueRF(const unsigned char* data);
	void writeQueueNet(const unsigned char* data);
	void writeNetworkRF(const unsigned char* data, unsigned char dataType, unsigned char errors = 0U);
	void writeNetworkRF(const unsigned char* data, unsigned char dataType, FLCO flco, unsigned int srcId, unsigned int dstId, unsigned char errors = 0U);

	void endOfRFData();
	void endOfNetData();

	void writeEndRF(bool writeEnd = false);
	void writeEndNet(bool writeEnd = false);

	bool openFile();
	bool writeFile(const unsigned char* data);
	void closeFile();

	bool insertSilence(const unsigned char* data, unsigned char seqNo);
	void insertSilence(unsigned int count);

	static void setShortLC(unsigned int slotNo, unsigned int id, FLCO flco = FLCO_GROUP, bool voice = true);
	static bool validateId(unsigned int id);
};

#endif
