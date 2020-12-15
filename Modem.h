/*
 *   Copyright (C) 2011-2018,2020 by Jonathan Naylor G4KLX
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

#ifndef	MODEM_H
#define	MODEM_H

#include "SerialController.h"
#include "RingBuffer.h"
#include "Defines.h"
#include "Timer.h"

#include <string>

enum RESP_TYPE_MMDVM {
	RTM_OK,
	RTM_TIMEOUT,
	RTM_ERROR
};

class CModem {
public:
	CModem(const std::string& port, bool duplex, bool rxInvert, bool txInvert, bool pttInvert, unsigned int txDelay, unsigned int dmrDelay, bool useCOSAsLockout, bool trace, bool debug);
	virtual ~CModem();

	virtual void setSerialParams(const std::string& protocol, unsigned int address);
	virtual void setRFParams(unsigned int rxFrequency, int rxOffset, unsigned int txFrequency, int txOffset, int txDCOffset, int rxDCOffset, float rfLevel, unsigned int pocsagFrequency);
	virtual void setModeParams(bool dstarEnabled, bool dmrEnabled, bool ysfEnabled, bool p25Enabled, bool nxdnEnabled, bool pocsagEnabled, bool fmEnabled);
	virtual void setLevels(float rxLevel, float cwIdTXLevel, float dstarTXLevel, float dmrTXLevel, float ysfTXLevel, float p25TXLevel, float nxdnTXLevel, float pocsagLevel, float fmTXLevel);
	virtual void setDMRParams(unsigned int colorCode);
	virtual void setYSFParams(bool loDev, unsigned int txHang);
	virtual void setP25Params(unsigned int txHang);
	virtual void setNXDNParams(unsigned int txHang);
	virtual void setTransparentDataParams(unsigned int sendFrameType);

	virtual void setFMCallsignParams(const std::string& callsign, unsigned int callsignSpeed, unsigned int callsignFrequency, unsigned int callsignTime, unsigned int callsignHoldoff, float callsignHighLevel, float callsignLowLevel, bool callsignAtStart, bool callsignAtEnd, bool callsignAtLatch);
	virtual void setFMAckParams(const std::string& rfAck, unsigned int ackSpeed, unsigned int ackFrequency, unsigned int ackMinTime, unsigned int ackDelay, float ackLevel);
	virtual void setFMMiscParams(unsigned int timeout, float timeoutLevel, float ctcssFrequency, unsigned int ctcssHighThreshold, unsigned int ctcssLowThreshold, float ctcssLevel, unsigned int kerchunkTime, unsigned int hangTime, unsigned int accessMode, bool cosInvert, unsigned int rfAudioBoost, float maxDevLevel);

	virtual bool open();

	virtual unsigned int readDStarData(unsigned char* data);
	virtual unsigned int readDMRData1(unsigned char* data);
	virtual unsigned int readDMRData2(unsigned char* data);
	virtual unsigned int readYSFData(unsigned char* data);
	virtual unsigned int readP25Data(unsigned char* data);
	virtual unsigned int readNXDNData(unsigned char* data);
	virtual unsigned int readTransparentData(unsigned char* data);

	virtual unsigned int readSerial(unsigned char* data, unsigned int length);

	virtual bool hasDStarSpace() const;
	virtual bool hasDMRSpace1() const;
	virtual bool hasDMRSpace2() const;
	virtual bool hasYSFSpace() const;
	virtual bool hasP25Space() const;
	virtual bool hasNXDNSpace() const;
	virtual bool hasPOCSAGSpace() const;

	virtual bool hasTX() const;
	virtual bool hasCD() const;

	virtual bool hasLockout() const;
	virtual bool hasError() const;

	virtual bool writeConfig();
	virtual bool writeDStarData(const unsigned char* data, unsigned int length);
	virtual bool writeDMRData1(const unsigned char* data, unsigned int length);
	virtual bool writeDMRData2(const unsigned char* data, unsigned int length);
	virtual bool writeYSFData(const unsigned char* data, unsigned int length);
	virtual bool writeP25Data(const unsigned char* data, unsigned int length);
	virtual bool writeNXDNData(const unsigned char* data, unsigned int length);
	virtual bool writePOCSAGData(const unsigned char* data, unsigned int length);

	virtual bool writeTransparentData(const unsigned char* data, unsigned int length);

	virtual bool writeDStarInfo(const char* my1, const char* my2, const char* your, const char* type, const char* reflector);
	virtual bool writeDMRInfo(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type);
	virtual bool writeYSFInfo(const char* source, const char* dest, unsigned char dgid, const char* type, const char* origin);
	virtual bool writeP25Info(const char* source, bool group, unsigned int dest, const char* type);
	virtual bool writeNXDNInfo(const char* source, bool group, unsigned int dest, const char* type);
	virtual bool writePOCSAGInfo(unsigned int ric, const std::string& message);
	virtual bool writeIPInfo(const std::string& address);

	virtual bool writeDMRStart(bool tx);
	virtual bool writeDMRShortLC(const unsigned char* lc);
	virtual bool writeDMRAbort(unsigned int slotNo);

	virtual bool writeSerial(const unsigned char* data, unsigned int length);

	virtual unsigned char getMode() const;
	virtual bool setMode(unsigned char mode);

	virtual bool sendCWId(const std::string& callsign);

	virtual HW_TYPE getHWType() const;

	virtual void clock(unsigned int ms);

	virtual void close();

	static CModem* createModem(const std::string& port, bool duplex, bool rxInvert, bool txInvert, bool pttInvert, unsigned int txDelay, unsigned int dmrDelay, bool useCOSAsLockout, bool trace, bool debug);

private:
	std::string                m_port;
	unsigned int               m_dmrColorCode;
	bool                       m_ysfLoDev;
	unsigned int               m_ysfTXHang;
	unsigned int               m_p25TXHang;
	unsigned int               m_nxdnTXHang;
	bool                       m_duplex;
	bool                       m_rxInvert;
	bool                       m_txInvert;
	bool                       m_pttInvert;
	unsigned int               m_txDelay;
	unsigned int               m_dmrDelay;
	float                      m_rxLevel;
	float                      m_cwIdTXLevel;
	float                      m_dstarTXLevel;
	float                      m_dmrTXLevel;
	float                      m_ysfTXLevel;
	float                      m_p25TXLevel;
	float                      m_nxdnTXLevel;
	float                      m_pocsagTXLevel;
	float                      m_fmTXLevel;
	float                      m_rfLevel;
	bool                       m_useCOSAsLockout;
	bool                       m_trace;
	bool                       m_debug;
	unsigned int               m_rxFrequency;
	unsigned int               m_txFrequency;
	unsigned int               m_pocsagFrequency;
	bool                       m_dstarEnabled;
	bool                       m_dmrEnabled;
	bool                       m_ysfEnabled;
	bool                       m_p25Enabled;
	bool                       m_nxdnEnabled;
	bool                       m_pocsagEnabled;
	bool                       m_fmEnabled;
	int                        m_rxDCOffset;
	int                        m_txDCOffset;
	CSerialController*         m_serial;
	unsigned char*             m_buffer;
	unsigned int               m_length;
	unsigned int               m_offset;
	CRingBuffer<unsigned char> m_rxDStarData;
	CRingBuffer<unsigned char> m_txDStarData;
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
	CRingBuffer<unsigned char> m_txPOCSAGData;
	CRingBuffer<unsigned char> m_rxTransparentData;
	CRingBuffer<unsigned char> m_txTransparentData;
	unsigned int               m_sendTransparentDataFrameType;
	CTimer                     m_statusTimer;
	CTimer                     m_inactivityTimer;
	CTimer                     m_playoutTimer;
	unsigned int               m_dstarSpace;
	unsigned int               m_dmrSpace1;
	unsigned int               m_dmrSpace2;
	unsigned int               m_ysfSpace;
	unsigned int               m_p25Space;
	unsigned int               m_nxdnSpace;
	unsigned int               m_pocsagSpace;
	bool                       m_tx;
	bool                       m_cd;
	bool                       m_lockout;
	bool                       m_error;
	unsigned char              m_mode;
	HW_TYPE                    m_hwType;

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
	bool                       m_fmCOSInvert;
	unsigned int               m_fmRFAudioBoost;
	float                      m_fmMaxDevLevel;

	bool readVersion();
	bool readStatus();
	bool setConfig();
	bool setFrequency();
	bool setFMCallsignParams();
	bool setFMAckParams();
	bool setFMMiscParams();

	void printDebug();

	RESP_TYPE_MMDVM getResponse();
};

#endif
