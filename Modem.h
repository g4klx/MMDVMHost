/*
 *   Copyright (C) 2011-2018 by Jonathan Naylor G4KLX
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
	CModem(const std::string& port, const std::string& protocol, unsigned int address, bool duplex, bool rxInvert, bool txInvert, bool pttInvert, unsigned int txDelay, unsigned int dmrDelay, bool trace, bool debug);
	~CModem();

	void setRFParams(unsigned int rxFrequency, int rxOffset, unsigned int txFrequency, int txOffset, int txDCOffset, int rxDCOffset, float rfLevel, unsigned int pocsagFrequency);
	void setModeParams(bool dstarEnabled, bool dmrEnabled, bool ysfEnabled, bool p25Enabled, bool nxdnEnabled, bool pocsagEnabled);
	void setLevels(float rxLevel, float cwIdTXLevel, float dstarTXLevel, float dmrTXLevel, float ysfTXLevel, float p25TXLevel, float nxdnTXLevel, float pocsagLevel);
	void setDMRParams(unsigned int colorCode);
	void setYSFParams(bool loDev, unsigned int txHang);

	bool open();

	unsigned int readDStarData(unsigned char* data);
	unsigned int readDMRData1(unsigned char* data);
	unsigned int readDMRData2(unsigned char* data);
	unsigned int readYSFData(unsigned char* data);
	unsigned int readP25Data(unsigned char* data);
	unsigned int readNXDNData(unsigned char* data);
	unsigned int readTransparentData(unsigned char* data);

	unsigned int readSerial(unsigned char* data, unsigned int length);

	bool hasDStarSpace() const;
	bool hasDMRSpace1() const;
	bool hasDMRSpace2() const;
	bool hasYSFSpace() const;
	bool hasP25Space() const;
	bool hasNXDNSpace() const;
	bool hasPOCSAGSpace() const;

	bool hasTX() const;
	bool hasCD() const;

	bool hasLockout() const;
	bool hasError() const;

	bool writeDStarData(const unsigned char* data, unsigned int length);
	bool writeDMRData1(const unsigned char* data, unsigned int length);
	bool writeDMRData2(const unsigned char* data, unsigned int length);
	bool writeYSFData(const unsigned char* data, unsigned int length);
	bool writeP25Data(const unsigned char* data, unsigned int length);
	bool writeNXDNData(const unsigned char* data, unsigned int length);
	bool writePOCSAGData(const unsigned char* data, unsigned int length);
	bool writeTransparentData(const unsigned char* data, unsigned int length);

	bool writeDMRStart(bool tx);
	bool writeDMRShortLC(const unsigned char* lc);
	bool writeDMRAbort(unsigned int slotNo);

	bool writeSerial(const unsigned char* data, unsigned int length);

	bool setMode(unsigned char mode);

	bool sendCWId(const std::string& callsign);

	HW_TYPE getHWType() const;

	void clock(unsigned int ms);

	void close();

private:
	std::string                m_port;
	unsigned int               m_dmrColorCode;
	bool                       m_ysfLoDev;
	unsigned int               m_ysfTXHang;
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
	float                      m_rfLevel;
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
	int                        m_rxDCOffset;
	int                        m_txDCOffset;
	CSerialController          m_serial;
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
	HW_TYPE                    m_hwType;

	bool readVersion();
	bool readStatus();
	bool setConfig();
	bool setFrequency();

	void printDebug();

	RESP_TYPE_MMDVM getResponse();
};

#endif
