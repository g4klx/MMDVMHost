/*
 *   Copyright (C) 2015-2019,2023 by Jonathan Naylor G4KLX
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

#include "RSSIInterpolator.h"
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

#include <nlohmann/json.hpp>

class CDStarControl {
public:
	CDStarControl(const std::string& callsign, const std::string& module, bool selfOnly, bool ackReply, unsigned int ackTime, DSTAR_ACK_MESSAGE ackMessage, bool errorReply, const std::vector<std::string>& blackList, const std::vector<std::string>& whiteList, CDStarNetwork* network, CDisplay* display, unsigned int timeout, bool duplex, bool remoteGateway, CRSSIInterpolator* rssiMapper);
	~CDStarControl();

	bool writeModem(unsigned char* data, unsigned int len);

	unsigned int readModem(unsigned char* data);

	void clock();

	bool isBusy() const;

	void enable(bool enabled);

private:
	unsigned char*             m_callsign;
	unsigned char*             m_gateway;
	bool                       m_selfOnly;
	bool                       m_ackReply;
	DSTAR_ACK_MESSAGE          m_ackMessage;
	bool                       m_errorReply;
	bool                       m_remoteGateway;
	std::vector<std::string>   m_blackList;
	std::vector<std::string>   m_whiteList;
	CDStarNetwork*             m_network;
	CDisplay*                  m_display;
	bool                       m_duplex;
	CRingBuffer<unsigned char> m_queue;
	CDStarHeader               m_rfHeader;
	CDStarHeader               m_netHeader;
	RPT_RF_STATE               m_rfState;
	RPT_NET_STATE              m_netState;
	bool                       m_net;
	CDStarSlowData             m_rfSlowData;
	CDStarSlowData             m_netSlowData;
	unsigned char              m_rfN;
	unsigned char              m_netN;
	CTimer                     m_networkWatchdog;
	CTimer                     m_rfTimeoutTimer;
	CTimer                     m_netTimeoutTimer;
	CTimer                     m_packetTimer;
	CTimer                     m_ackTimer;
	CTimer                     m_errTimer;
	CStopWatch                 m_interval;
	CStopWatch                 m_elapsed;
	unsigned int               m_rfFrames;
	unsigned int               m_netFrames;
	unsigned int               m_netLost;
	CAMBEFEC                   m_fec;
	unsigned int               m_rfBits;
	unsigned int               m_netBits;
	unsigned int               m_rfErrs;
	unsigned char*             m_lastFrame;
	bool                       m_lastFrameValid;
	CRSSIInterpolator*         m_rssiMapper;
	unsigned char              m_rssi;
	unsigned char              m_maxRSSI;
	unsigned char              m_minRSSI;
	unsigned int               m_aveRSSI;
	unsigned int               m_rssiCountTotal;
	unsigned int               m_rssiAccum;
	unsigned int               m_rssiCount;
	unsigned int               m_bitErrsAccum;
	unsigned int               m_bitsCount;
	bool                       m_enabled;
	FILE*                      m_fp;
	unsigned char*             m_rfVoiceSyncData;
	unsigned int               m_rfVoiceSyncDataLen;
	unsigned char*             m_netVoiceSyncData;
	unsigned int               m_netVoiceSyncDataLen;
	bool                       m_rfNextFrameIsFastData;
	bool                       m_netNextFrameIsFastData;
	unsigned int               m_rfSkipDTMFBlankingFrames;
	unsigned int               m_netSkipDTMFBlankingFrames;

	unsigned int maybeFixupVoiceFrame(
		unsigned char*  data,
		unsigned int    len,
		unsigned int    offset,
		const char*     log_prefix,
		unsigned char   n,
		bool            blank_dtmf,
		unsigned char*  voice_sync_data,
		unsigned int&   voice_sync_data_len,
		bool&           next_frame_is_fast_data,
		unsigned int&   skip_dtmf_blanking_frames
		);

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

	void writeJSONRSSI();
	void writeJSONBER();
	void writeJSONText(const unsigned char* text);

	void writeJSONRF(const char* action, const unsigned char* my1, const unsigned char* my2, const unsigned char* your);
	void writeJSONRF(const char* action, float duration, float ber);
	void writeJSONRF(const char* action, float duration, float ber, unsigned char minRSSI, unsigned char maxRSSI, unsigned int aveRSSI);
	void writeJSONNet(const char* action, const unsigned char* my1, const unsigned char* my2, const unsigned char* your, const unsigned char* reflector = NULL);
	void writeJSONNet(const char* action, float duration, float loss);

	void writeJSONRF(nlohmann::json& json, const char* action, float duration, float ber);

	std::string convertBuffer(const unsigned char* buffer, unsigned int length) const;

	bool openFile();
	bool writeFile(const unsigned char* data, unsigned int length);
	void closeFile();

	bool insertSilence(const unsigned char* data, unsigned char seqNo);
	void insertSilence(unsigned int count);

	void blankDTMF(unsigned char* data) const;

	void sendAck();
	void sendError();
};

#endif
