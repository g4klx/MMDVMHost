/*
 *   Copyright (C) 2011-2018,2020,2021,2023 by Jonathan Naylor G4KLX
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

#ifndef	Modem_H
#define	Modem_H

#include "ModemPort.h"
#include "RingBuffer.h"
#include "Defines.h"
#include "Timer.h"

#include <string>

enum RESP_TYPE_MMDVM {
	RTM_OK,
	RTM_TIMEOUT,
	RTM_ERROR
};

enum SERIAL_STATE {
	SS_START,
	SS_LENGTH1,
	SS_LENGTH2,
	SS_TYPE,
	SS_DATA
};

class CModem {
public:
	CModem(bool duplex, bool rxInvert, bool txInvert, bool pttInvert, unsigned int txDelay, unsigned int dmrDelay, bool useCOSAsLockout, bool trace, bool debug);
	~CModem();

	void setPort(IModemPort* port);
	void setRFParams(unsigned int rxFrequency, int rxOffset, unsigned int txFrequency, int txOffset, int txDCOffset, int rxDCOffset, float rfLevel, unsigned int pocsagFrequency);
	void setModeParams(bool dstarEnabled, bool dmrEnabled, bool ysfEnabled, bool p25Enabled, bool nxdnEnabled, bool m17Enabled, bool pocsagEnabled, bool fmEnabled, bool ax25Enabled);
	void setLevels(float rxLevel, float cwIdTXLevel, float dstarTXLevel, float dmrTXLevel, float ysfTXLevel, float p25TXLevel, float nxdnTXLevel, float m17TXLevel, float pocsagLevel, float fmTXLevel, float ax25TXLevel);
	void setDMRParams(unsigned int colorCode);
	void setYSFParams(bool loDev, unsigned int txHang);
	void setP25Params(unsigned int txHang);
	void setNXDNParams(unsigned int txHang);
#if defined(USE_M17)
	void setM17Params(unsigned int txHang);
#endif
#if defined(USE_AX25)
	void setAX25Params(int rxTwist, unsigned int txDelay, unsigned int slotTime, unsigned int pPersist);
#endif
	void setTransparentDataParams(unsigned int sendFrameType);

#if defined(USE_FM)
	void setFMCallsignParams(const std::string& callsign, unsigned int callsignSpeed, unsigned int callsignFrequency, unsigned int callsignTime, unsigned int callsignHoldoff, float callsignHighLevel, float callsignLowLevel, bool callsignAtStart, bool callsignAtEnd, bool callsignAtLatch);
	void setFMAckParams(const std::string& rfAck, unsigned int ackSpeed, unsigned int ackFrequency, unsigned int ackMinTime, unsigned int ackDelay, float ackLevel);
	void setFMMiscParams(unsigned int timeout, float timeoutLevel, float ctcssFrequency, unsigned int ctcssHighThreshold, unsigned int ctcssLowThreshold, float ctcssLevel, unsigned int kerchunkTime, unsigned int hangTime, unsigned int accessMode, bool linkMode, bool cosInvert, bool noiseSquelch, unsigned int squelchHighThreshold, unsigned int squelchLowThreshold, unsigned int rfAudioBoost, float maxDevLevel);
	void setFMExtParams(const std::string& ack, unsigned int audioBoost);
#endif

	bool open();

#if defined(USE_DSTAR)
	bool hasDStar() const;
#endif
	bool hasDMR() const;
	bool hasYSF() const;
	bool hasP25() const;
	bool hasNXDN() const;
#if defined(USE_M17)
	bool hasM17() const;
#endif
#if defined(USE_POCSAG)
	bool hasPOCSAG() const;
#endif
#if defined(USE_FM)
	bool hasFM() const;
#endif
#if defined(USE_AX25)
	bool hasAX25() const;
#endif
	unsigned int getVersion() const;

#if defined(USE_DSTAR)
	unsigned int readDStarData(unsigned char* data);
#endif
	unsigned int readDMRData1(unsigned char* data);
	unsigned int readDMRData2(unsigned char* data);
	unsigned int readYSFData(unsigned char* data);
	unsigned int readP25Data(unsigned char* data);
	unsigned int readNXDNData(unsigned char* data);
#if defined(USE_M17)
	unsigned int readM17Data(unsigned char* data);
#endif
#if defined(USE_FM)
	unsigned int readFMData(unsigned char* data);
#endif
#if defined(USE_AX25)
	unsigned int readAX25Data(unsigned char* data);
#endif

#if defined(USE_DSTAR)
	bool hasDStarSpace() const;
#endif
	bool hasDMRSpace1() const;
	bool hasDMRSpace2() const;
	bool hasYSFSpace() const;
	bool hasP25Space() const;
	bool hasNXDNSpace() const;
#if defined(USE_M17)
	bool hasM17Space() const;
#endif
#if defined(USE_POCSAG)
	bool hasPOCSAGSpace() const;
#endif
#if defined(USE_FM)
	unsigned int getFMSpace() const;
#endif
#if defined(USE_AX25)
	bool hasAX25Space() const;
#endif

	bool hasTX() const;
	bool hasCD() const;

	bool hasLockout() const;
	bool hasError() const;

	bool writeConfig();

#if defined(USE_DSTAR)
	bool writeDStarData(const unsigned char* data, unsigned int length);
#endif
	bool writeDMRData1(const unsigned char* data, unsigned int length);
	bool writeDMRData2(const unsigned char* data, unsigned int length);
	bool writeYSFData(const unsigned char* data, unsigned int length);
	bool writeP25Data(const unsigned char* data, unsigned int length);
	bool writeNXDNData(const unsigned char* data, unsigned int length);
#if defined(USE_M17)
	bool writeM17Data(const unsigned char* data, unsigned int length);
#endif
#if defined(USE_POCSAG)
	bool writePOCSAGData(const unsigned char* data, unsigned int length);
#endif
#if defined(USE_FM)
	bool writeFMData(const unsigned char* data, unsigned int length);
#endif
#if defined(USE_AX25)
	bool writeAX25Data(const unsigned char* data, unsigned int length);
#endif

#if defined(USE_DSTAR)
	bool writeDStarInfo(const char* my1, const char* my2, const char* your, const char* type, const char* reflector);
#endif
	bool writeDMRInfo(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type);
	bool writeYSFInfo(const char* source, const char* dest, unsigned char dgid, const char* type, const char* origin);
	bool writeP25Info(const char* source, bool group, unsigned int dest, const char* type);
	bool writeNXDNInfo(const char* source, bool group, unsigned int dest, const char* type);
#if defined(USE_M17)
	bool writeM17Info(const char* source, const char* dest, const char* type);
#endif
#if defined(USE_POCSAG)
	bool writePOCSAGInfo(unsigned int ric, const std::string& message);
#endif
	bool writeIPInfo(const std::string& address);

	bool writeDMRStart(bool tx);
	bool writeDMRShortLC(const unsigned char* lc);
	bool writeDMRAbort(unsigned int slotNo);

	bool writeTransparentData(const unsigned char* data, unsigned int length);
	unsigned int readTransparentData(unsigned char* data);

	bool writeSerialData(const unsigned char* data, unsigned int length);
	unsigned int readSerialData(unsigned char* data);

	unsigned char getMode() const;
	bool setMode(unsigned char mode);

	bool sendCWId(const std::string& callsign);

	HW_TYPE getHWType() const;

	void clock(unsigned int ms);

	void close();

private:
	unsigned int               m_protocolVersion;
	unsigned int               m_dmrColorCode;
	bool                       m_ysfLoDev;
	unsigned int               m_ysfTXHang;
	unsigned int               m_p25TXHang;
	unsigned int               m_nxdnTXHang;
#if defined(USE_M17)
	unsigned int               m_m17TXHang;
#endif
	bool                       m_duplex;
	bool                       m_rxInvert;
	bool                       m_txInvert;
	bool                       m_pttInvert;
	unsigned int               m_txDelay;
	unsigned int               m_dmrDelay;
	float                      m_rxLevel;
	float                      m_cwIdTXLevel;
#if defined(USE_DSTAR)
	float                      m_dstarTXLevel;
#endif
	float                      m_dmrTXLevel;
	float                      m_ysfTXLevel;
	float                      m_p25TXLevel;
	float                      m_nxdnTXLevel;
#if defined(USE_M17)
	float                      m_m17TXLevel;
#endif
#if defined(USE_POCSAG)
	float                      m_pocsagTXLevel;
#endif
#if defined(USE_FM)
	float                      m_fmTXLevel;
#endif
#if defined(USE_AX25)
	float                      m_ax25TXLevel;
#endif
	float                      m_rfLevel;
	bool                       m_useCOSAsLockout;
	bool                       m_trace;
	bool                       m_debug;
	unsigned int               m_rxFrequency;
	unsigned int               m_txFrequency;
#if defined(USE_POCSAG)
	unsigned int               m_pocsagFrequency;
#endif
#if defined(USE_DSTAR)
	bool                       m_dstarEnabled;
#endif
	bool                       m_dmrEnabled;
	bool                       m_ysfEnabled;
	bool                       m_p25Enabled;
	bool                       m_nxdnEnabled;
#if defined(USE_M17)
	bool                       m_m17Enabled;
#endif
#if defined(USE_POCSAG)
	bool                       m_pocsagEnabled;
#endif
#if defined(USE_FM)
	bool                       m_fmEnabled;
#endif
#if defined(USE_AX25)
	bool                       m_ax25Enabled;
#endif
	int                        m_rxDCOffset;
	int                        m_txDCOffset;
	IModemPort*                m_port;
	unsigned char*             m_buffer;
	unsigned int               m_length;
	unsigned int               m_offset;
	SERIAL_STATE               m_state;
	unsigned char              m_type;

#if defined(USE_DSTAR)
	CRingBuffer<unsigned char> m_rxDStarData;
	CRingBuffer<unsigned char> m_txDStarData;
#endif
	CRingBuffer<unsigned char> m_rxDMRData1;
	CRingBuffer<unsigned char> m_rxDMRData2;
	CRingBuffer<unsigned char> m_txDMRData1;
	CRingBuffer<unsigned char> m_txDMRData2;
	CRingBuffer<unsigned char> m_rxYSFData;
	CRingBuffer<unsigned char> m_txYSFData;
	CRingBuffer<unsigned char> m_rxP25Data;
	CRingBuffer<unsigned char> m_txP25Data;
	CRingBuffer<unsigned char> m_rxNXDNData;
	CRingBuffer<unsigned char> m_txNXDNData;
#if defined(USE_M17)
	CRingBuffer<unsigned char> m_rxM17Data;
	CRingBuffer<unsigned char> m_txM17Data;
#endif
#if defined(USE_POCSAG)
	CRingBuffer<unsigned char> m_txPOCSAGData;
#endif
#if defined(USE_FM)
	CRingBuffer<unsigned char> m_rxFMData;
	CRingBuffer<unsigned char> m_txFMData;
#endif
#if defined(USE_AX25)
	CRingBuffer<unsigned char> m_rxAX25Data;
	CRingBuffer<unsigned char> m_txAX25Data;
#endif
	CRingBuffer<unsigned char> m_rxSerialData;
	CRingBuffer<unsigned char> m_txSerialData;
	CRingBuffer<unsigned char> m_rxTransparentData;
	CRingBuffer<unsigned char> m_txTransparentData;
	unsigned int               m_sendTransparentDataFrameType;
	CTimer                     m_statusTimer;
	CTimer                     m_inactivityTimer;
	CTimer                     m_playoutTimer;
#if defined(USE_DSTAR)
	unsigned int               m_dstarSpace;
#endif
	unsigned int               m_dmrSpace1;
	unsigned int               m_dmrSpace2;
	unsigned int               m_ysfSpace;
	unsigned int               m_p25Space;
	unsigned int               m_nxdnSpace;
#if defined(USE_M17)
	unsigned int               m_m17Space;
#endif
#if defined(USE_POCSAG)
	unsigned int               m_pocsagSpace;
#endif
#if defined(USE_FM)
	unsigned int               m_fmSpace;
#endif
#if defined(USE_AX25)
	unsigned int               m_ax25Space;
#endif
	bool                       m_tx;
	bool                       m_cd;
	bool                       m_lockout;
	bool                       m_error;
	unsigned char              m_mode;
	HW_TYPE                    m_hwType;
#if defined(USE_AX25)
	int                        m_ax25RXTwist;
	unsigned int               m_ax25TXDelay;
	unsigned int               m_ax25SlotTime;
	unsigned int               m_ax25PPersist;
#endif
#if defined(USE_FM)
	std::string                m_fmCallsign;
	unsigned int               m_fmCallsignSpeed;
	unsigned int               m_fmCallsignFrequency;
	unsigned int               m_fmCallsignTime;
	unsigned int               m_fmCallsignHoldoff;
	float                      m_fmCallsignHighLevel;
	float                      m_fmCallsignLowLevel;
	bool                       m_fmCallsignAtStart;
	bool                       m_fmCallsignAtEnd;
	bool                       m_fmCallsignAtLatch;
	std::string                m_fmRfAck;
	std::string                m_fmExtAck;
	unsigned int               m_fmAckSpeed;
	unsigned int               m_fmAckFrequency;
	unsigned int               m_fmAckMinTime;
	unsigned int               m_fmAckDelay;
	float                      m_fmAckLevel;
	unsigned int               m_fmTimeout;
	float                      m_fmTimeoutLevel;
	float                      m_fmCtcssFrequency;
	unsigned int               m_fmCtcssHighThreshold;
	unsigned int               m_fmCtcssLowThreshold;
	float                      m_fmCtcssLevel;
	unsigned int               m_fmKerchunkTime;
	unsigned int               m_fmHangTime;
	unsigned int               m_fmAccessMode;
	bool                       m_fmLinkMode;
	bool                       m_fmCOSInvert;
	bool                       m_fmNoiseSquelch;
	unsigned int               m_fmSquelchHighThreshold;
	unsigned int               m_fmSquelchLowThreshold;
	unsigned int               m_fmRFAudioBoost;
	unsigned int               m_fmExtAudioBoost;
	float                      m_fmMaxDevLevel;
	bool                       m_fmExtEnable;
#endif
	unsigned char              m_capabilities1;
	unsigned char              m_capabilities2;

	bool readVersion();
	bool readStatus();
	bool setConfig1();
	bool setConfig2();
	bool setFrequency();
#if defined(USE_FM)
	bool setFMCallsignParams();
	bool setFMAckParams();
	bool setFMMiscParams();
	bool setFMExtParams();
#endif
	void printDebug();

	RESP_TYPE_MMDVM getResponse();
};

#endif
