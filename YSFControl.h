/*
 *   Copyright (C) 2015-2020,2023 by Jonathan Naylor G4KLX
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

#include "RSSIInterpolator.h"
#include "YSFNetwork.h"
#include "YSFDefines.h"
#include "YSFPayload.h"
#include "RingBuffer.h"
#include "StopWatch.h"
#include "YSFFICH.h"
#include "Defines.h"
#include "Timer.h"
#include "Modem.h"

#if defined(USE_YSF)

#include <string>

#include <nlohmann/json.hpp>

class CYSFControl {
public:
	CYSFControl(const std::string& callsign, bool selfOnly, CYSFNetwork* network, unsigned int timeout, bool duplex, bool lowDeviation, bool remoteGateway, CRSSIInterpolator* rssiMapper);
	~CYSFControl();

	bool writeModem(unsigned char* data, unsigned int len);

	unsigned int readModem(unsigned char* data);

	void clock(unsigned int ms);

	bool isBusy() const;

	void enable(bool enabled);

private:
	unsigned char*             m_callsign;
	unsigned char*             m_selfCallsign;
	bool                       m_selfOnly;
	CYSFNetwork*               m_network;
	bool                       m_duplex;
	bool                       m_lowDeviation;
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
	unsigned int               m_netBits;
	unsigned char*             m_rfSource;
	unsigned char*             m_rfDest;
	unsigned char*             m_netSource;
	unsigned char*             m_netDest;
	CYSFFICH                   m_lastFICH;
	unsigned char              m_netN;
	CYSFPayload                m_rfPayload;
	CYSFPayload                m_netPayload;
	CRSSIInterpolator*         m_rssiMapper;
	unsigned char              m_rssi;
	unsigned char              m_maxRSSI;
	unsigned char              m_minRSSI;
	unsigned int               m_aveRSSI;
	unsigned int               m_rssiCountTotal;
	unsigned int               m_rssiAccum;
	unsigned int               m_rssiCount;
	unsigned int               m_bitsCount;
	unsigned int               m_bitErrsAccum;
	bool                       m_enabled;

	bool processVWData(bool valid, unsigned char *data);
	bool processDNData(bool valid, unsigned char *data);
	bool processFRData(bool valid, unsigned char *data);

	void writeQueueRF(const unsigned char* data);
	void writeQueueNet(const unsigned char* data);
	void writeNetwork(const unsigned char* data, unsigned int count);
	void writeNetwork();

	void writeEndRF();
	void writeEndNet();

	void writeJSONRSSI();
	void writeJSONBER(unsigned int bits, unsigned int errs);

	void writeJSONRF(const char* action, const char* mode, const unsigned char* source, unsigned char dgid);
	void writeJSONRF(const char* action, float duration, float ber);
	void writeJSONRF(const char* action, float duration, float ber, unsigned char minRSSI, unsigned char maxRSSI, unsigned int aveRSSI);

	void writeJSONNet(const char* action, const unsigned char* source, unsigned char dgid, const unsigned char* reflector);
	void writeJSONNet(const char* action, float duration, unsigned int loss);

	void writeJSONRF(nlohmann::json& json, const char* action);
	void writeJSONRF(nlohmann::json& json, const char* action, const unsigned char* source, unsigned char dgid);

	void writeJSONNet(nlohmann::json& json, const char* action);
	void writeJSONNet(nlohmann::json& json, const char* action, const unsigned char* source, unsigned char dgid);

	std::string convertBuffer(const unsigned char* buffer) const;

	bool checkCallsign(const unsigned char* callsign) const;
	void processNetCallsigns(const unsigned char* data, unsigned char dgid);
};

#endif

#endif

