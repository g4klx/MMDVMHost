/*
 *   Copyright (C) 2011-2018,2020,2021 by Jonathan Naylor G4KLX
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
#ifdef notdef
<<<<<<< HEAD
	virtual ~IModem() = 0;

	virtual void setSerialParams(const std::string& protocol, unsigned int address, unsigned int speed) = 0;
	virtual void setRFParams(unsigned int rxFrequency, int rxOffset, unsigned int txFrequency, int txOffset, int txDCOffset, int rxDCOffset, float rfLevel, unsigned int pocsagFrequency) = 0;
	virtual void setModeParams(bool dstarEnabled, bool dmrEnabled, bool ysfEnabled, bool p25Enabled, bool nxdnEnabled, bool m17ENabled, bool pocsagEnabled, bool fmEnabled, bool ax25Enabled) = 0;
	virtual void setLevels(float rxLevel, float cwIdTXLevel, float dstarTXLevel, float dmrTXLevel, float ysfTXLevel, float p25TXLevel, float nxdnTXLevel, float m17TXLevel, float pocsagLevel, float fmTXLevel, float ax25TXLevel) = 0;
	virtual void setDMRParams(unsigned int colorCode) = 0;
	virtual void setYSFParams(bool loDev, unsigned int txHang) = 0;
	virtual void setP25Params(unsigned int txHang) = 0;
	virtual void setNXDNParams(unsigned int txHang) = 0;
	virtual void setM17Params(unsigned int txHang) = 0;
	virtual void setAX25Params(int rxTwist, unsigned int txDelay, unsigned int slotTime, unsigned int pPersist) = 0;
	virtual void setTransparentDataParams(unsigned int sendFrameType) = 0;

	virtual void setFMCallsignParams(const std::string& callsign, unsigned int callsignSpeed, unsigned int callsignFrequency, unsigned int callsignTime, unsigned int callsignHoldoff, float callsignHighLevel, float callsignLowLevel, bool callsignAtStart, bool callsignAtEnd, bool callsignAtLatch) = 0;
	virtual void setFMAckParams(const std::string& rfAck, unsigned int ackSpeed, unsigned int ackFrequency, unsigned int ackMinTime, unsigned int ackDelay, float ackLevel) = 0;
	virtual void setFMMiscParams(unsigned int timeout, float timeoutLevel, float ctcssFrequency, unsigned int ctcssHighThreshold, unsigned int ctcssLowThreshold, float ctcssLevel, unsigned int kerchunkTime, unsigned int hangTime, unsigned int accessMode, bool cosInvert, bool noiseSquelch, unsigned int squelchHighThreshold, unsigned int squelchLowThreshold, unsigned int rfAudioBoost, float maxDevLevel) = 0;
	virtual void setFMExtParams(const std::string& ack, unsigned int audioBoost) = 0;

	virtual bool open() = 0;

	virtual unsigned int readDStarData(unsigned char* data) = 0;
	virtual unsigned int readDMRData1(unsigned char* data) = 0;
	virtual unsigned int readDMRData2(unsigned char* data) = 0;
	virtual unsigned int readYSFData(unsigned char* data) = 0;
	virtual unsigned int readP25Data(unsigned char* data) = 0;
	virtual unsigned int readNXDNData(unsigned char* data) = 0;
	virtual unsigned int readM17Data(unsigned char* data) = 0;
	virtual unsigned int readFMData(unsigned char* data) = 0;
	virtual unsigned int readAX25Data(unsigned char* data) = 0;

	virtual bool hasDStarSpace() const = 0;
	virtual bool hasDMRSpace1() const = 0;
	virtual bool hasDMRSpace2() const = 0;
	virtual bool hasYSFSpace() const = 0;
	virtual bool hasP25Space() const = 0;
	virtual bool hasNXDNSpace() const = 0;
	virtual bool hasM17Space() const = 0;
	virtual bool hasPOCSAGSpace() const = 0;
	virtual unsigned int getFMSpace() const = 0;
	virtual bool hasAX25Space() const = 0;

	virtual bool hasTX() const = 0;
	virtual bool hasCD() const = 0;

	virtual bool hasLockout() const = 0;
	virtual bool hasError() const = 0;

	virtual bool writeConfig() = 0;
	virtual bool writeDStarData(const unsigned char* data, unsigned int length) = 0;
	virtual bool writeDMRData1(const unsigned char* data, unsigned int length) = 0;
	virtual bool writeDMRData2(const unsigned char* data, unsigned int length) = 0;
	virtual bool writeYSFData(const unsigned char* data, unsigned int length) = 0;
	virtual bool writeP25Data(const unsigned char* data, unsigned int length) = 0;
	virtual bool writeNXDNData(const unsigned char* data, unsigned int length) = 0;
	virtual bool writeM17Data(const unsigned char* data, unsigned int length) = 0;
	virtual bool writePOCSAGData(const unsigned char* data, unsigned int length) = 0;
	virtual bool writeFMData(const unsigned char* data, unsigned int length) = 0;
	virtual bool writeAX25Data(const unsigned char* data, unsigned int length) = 0;

	virtual bool writeDStarInfo(const char* my1, const char* my2, const char* your, const char* type, const char* reflector) = 0;
	virtual bool writeDMRInfo(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type) = 0;
	virtual bool writeYSFInfo(const char* source, const char* dest, unsigned char dgid, const char* type, const char* origin) = 0;
	virtual bool writeP25Info(const char* source, bool group, unsigned int dest, const char* type) = 0;
	virtual bool writeNXDNInfo(const char* source, bool group, unsigned int dest, const char* type) = 0;
	virtual bool writeM17Info(const char* source, const char* dest, const char* type) = 0;
	virtual bool writePOCSAGInfo(unsigned int ric, const std::string& message) = 0;
	virtual bool writeIPInfo(const std::string& address) = 0;

	virtual bool writeDMRStart(bool tx) = 0;
	virtual bool writeDMRShortLC(const unsigned char* lc) = 0;
	virtual bool writeDMRAbort(unsigned int slotNo) = 0;

	virtual bool writeTransparentData(const unsigned char* data, unsigned int length) = 0;
	virtual unsigned int readTransparentData(unsigned char* data) = 0;

	virtual bool writeSerial(const unsigned char* data, unsigned int length) = 0;
	virtual unsigned int readSerial(unsigned char* data, unsigned int length) = 0;

	virtual bool writeI2CCommand(unsigned char address, const unsigned char* data, unsigned int length) = 0;
	virtual bool writeI2CData(unsigned char address, const unsigned char* data, unsigned int length) = 0;

	virtual unsigned char getMode() const = 0;
	virtual bool setMode(unsigned char mode) = 0;

	virtual bool sendCWId(const std::string& callsign) = 0;

	virtual HW_TYPE getHWType() const = 0;

	virtual void clock(unsigned int ms) = 0;

	virtual void close() = 0;
=======
#endif
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
	void setM17Params(unsigned int txHang);
	void setAX25Params(int rxTwist, unsigned int txDelay, unsigned int slotTime, unsigned int pPersist);
	void setTransparentDataParams(unsigned int sendFrameType);

	void setFMCallsignParams(const std::string& callsign, unsigned int callsignSpeed, unsigned int callsignFrequency, unsigned int callsignTime, unsigned int callsignHoldoff, float callsignHighLevel, float callsignLowLevel, bool callsignAtStart, bool callsignAtEnd, bool callsignAtLatch);
	void setFMAckParams(const std::string& rfAck, unsigned int ackSpeed, unsigned int ackFrequency, unsigned int ackMinTime, unsigned int ackDelay, float ackLevel);
	void setFMMiscParams(unsigned int timeout, float timeoutLevel, float ctcssFrequency, unsigned int ctcssHighThreshold, unsigned int ctcssLowThreshold, float ctcssLevel, unsigned int kerchunkTime, unsigned int hangTime, unsigned int accessMode, bool cosInvert, bool noiseSquelch, unsigned int squelchHighThreshold, unsigned int squelchLowThreshold, unsigned int rfAudioBoost, float maxDevLevel);
	void setFMExtParams(const std::string& ack, unsigned int audioBoost);

	bool open();

	unsigned int readDStarData(unsigned char* data);
	unsigned int readDMRData1(unsigned char* data);
	unsigned int readDMRData2(unsigned char* data);
	unsigned int readYSFData(unsigned char* data);
	unsigned int readP25Data(unsigned char* data);
	unsigned int readNXDNData(unsigned char* data);
	unsigned int readM17Data(unsigned char* data);
	unsigned int readFMData(unsigned char* data);
	unsigned int readAX25Data(unsigned char* data);

	bool hasDStarSpace() const;
	bool hasDMRSpace1() const;
	bool hasDMRSpace2() const;
	bool hasYSFSpace() const;
	bool hasP25Space() const;
	bool hasNXDNSpace() const;
	bool hasM17Space() const;
	bool hasPOCSAGSpace() const;
	unsigned int getFMSpace() const;
	bool hasAX25Space() const;

	bool hasTX() const;
	bool hasCD() const;

	bool hasLockout() const;
	bool hasError() const;

	bool writeConfig();
	bool writeDStarData(const unsigned char* data, unsigned int length);
	bool writeDMRData1(const unsigned char* data, unsigned int length);
	bool writeDMRData2(const unsigned char* data, unsigned int length);
	bool writeYSFData(const unsigned char* data, unsigned int length);
	bool writeP25Data(const unsigned char* data, unsigned int length);
	bool writeNXDNData(const unsigned char* data, unsigned int length);
	bool writeM17Data(const unsigned char* data, unsigned int length);
	bool writePOCSAGData(const unsigned char* data, unsigned int length);
	bool writeFMData(const unsigned char* data, unsigned int length);
	bool writeAX25Data(const unsigned char* data, unsigned int length);

	bool writeDStarInfo(const char* my1, const char* my2, const char* your, const char* type, const char* reflector);
	bool writeDMRInfo(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type);
	bool writeYSFInfo(const char* source, const char* dest, unsigned char dgid, const char* type, const char* origin);
	bool writeP25Info(const char* source, bool group, unsigned int dest, const char* type);
	bool writeNXDNInfo(const char* source, bool group, unsigned int dest, const char* type);
	bool writeM17Info(const char* source, const char* dest, const char* type);
	bool writePOCSAGInfo(unsigned int ric, const std::string& message);
	bool writeIPInfo(const std::string& address);

	bool writeDMRStart(bool tx);
	bool writeDMRShortLC(const unsigned char* lc);
	bool writeDMRAbort(unsigned int slotNo);

	bool writeTransparentData(const unsigned char* data, unsigned int length);
	unsigned int readTransparentData(unsigned char* data);

	bool writeSerial(const unsigned char* data, unsigned int length);
	unsigned int readSerial(unsigned char* data, unsigned int length);

	bool writeI2CCommand(unsigned char address, const unsigned char* data, unsigned int length);
	bool writeI2CData(unsigned char address, const unsigned char* data, unsigned int length);

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
	unsigned int               m_m17TXHang;
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
	float                      m_m17TXLevel;
	float                      m_pocsagTXLevel;
	float                      m_fmTXLevel;
	float                      m_ax25TXLevel;
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
	bool                       m_m17Enabled;
	bool                       m_pocsagEnabled;
	bool                       m_fmEnabled;
	bool                       m_ax25Enabled;
	int                        m_rxDCOffset;
	int                        m_txDCOffset;
	IModemPort*                m_port;
	unsigned char*             m_buffer;
	unsigned int               m_length;
	unsigned int               m_offset;
	SERIAL_STATE               m_state;
	unsigned char              m_type;
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
	CRingBuffer<unsigned char> m_rxM17Data;
	CRingBuffer<unsigned char> m_txM17Data;
	CRingBuffer<unsigned char> m_txPOCSAGData;
	CRingBuffer<unsigned char> m_rxFMData;
	CRingBuffer<unsigned char> m_txFMData;
	CRingBuffer<unsigned char> m_rxAX25Data;
	CRingBuffer<unsigned char> m_txAX25Data;
	CRingBuffer<unsigned char> m_rxSerialData;
	CRingBuffer<unsigned char> m_txSerialData;
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
	unsigned int               m_m17Space;
	unsigned int               m_pocsagSpace;
	unsigned int               m_fmSpace;
	unsigned int               m_ax25Space;
	bool                       m_tx;
	bool                       m_cd;
	bool                       m_lockout;
	bool                       m_error;
	unsigned char              m_mode;
	HW_TYPE                    m_hwType;
	int                        m_ax25RXTwist;
	unsigned int               m_ax25TXDelay;
	unsigned int               m_ax25SlotTime;
	unsigned int               m_ax25PPersist;

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
	bool                       m_fmCOSInvert;
	bool                       m_fmNoiseSquelch;
	unsigned int               m_fmSquelchHighThreshold;
	unsigned int               m_fmSquelchLowThreshold;
	unsigned int               m_fmRFAudioBoost;
	unsigned int               m_fmExtAudioBoost;
	float                      m_fmMaxDevLevel;
	bool                       m_fmExtEnable;

	bool readVersion();
	bool readStatus();
	bool setConfig1();
	bool setConfig2();
	bool setFrequency();
	bool setFMCallsignParams();
	bool setFMAckParams();
	bool setFMMiscParams();
	bool setFMExtParams();

	void printDebug();

	RESP_TYPE_MMDVM getResponse();
};

#endif
