/*
 *   Copyright (C) 2011-2016 by Jonathan Naylor G4KLX
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
#include "Timer.h"

#include <string>

enum RESP_TYPE_MMDVM {
	RTM_OK,
	RTM_TIMEOUT,
	RTM_ERROR
};

class CModem {
public:
	CModem(const std::string& port, bool rxInvert, bool txInvert, bool pttInvert, unsigned int txDelay, unsigned int rxLevel, unsigned int txLevel, unsigned int dmrDelay, bool debug = false);
	~CModem();

	void setRFParams(unsigned int rxFrequency, unsigned int txFrequency);
	void setModeParams(bool dstarEnabled, bool dmrEnabled, bool ysfEnabled);
	void setDMRParams(unsigned int colorCode);

	bool open();

	unsigned int readDStarData(unsigned char* data);
	unsigned int readDMRData1(unsigned char* data);
	unsigned int readDMRData2(unsigned char* data);
	unsigned int readYSFData(unsigned char* data);

	bool hasDStarSpace() const;
	bool hasDMRSpace1() const;
	bool hasDMRSpace2() const;
	bool hasYSFSpace() const;

	bool hasLockout() const;

	bool writeDStarData(const unsigned char* data, unsigned int length);
	bool writeDMRData1(const unsigned char* data, unsigned int length);
	bool writeDMRData2(const unsigned char* data, unsigned int length);
	bool writeYSFData(const unsigned char* data, unsigned int length);

	bool writeDMRStart(bool tx);
	bool writeDMRShortLC(const unsigned char* lc);

	bool setMode(unsigned char mode);

	void clock(unsigned int ms);

	void close();

private:
	std::string                m_port;
	unsigned int               m_colorCode;
	bool                       m_rxInvert;
	bool                       m_txInvert;
	bool                       m_pttInvert;
	unsigned int               m_txDelay;
	unsigned int               m_dmrDelay;
	unsigned int               m_rxLevel;
	unsigned int               m_txLevel;
	bool                       m_debug;
	unsigned int               m_rxFrequency;
	unsigned int               m_txFrequency;
	bool                       m_dstarEnabled;
	bool                       m_dmrEnabled;
	bool                       m_ysfEnabled;
	CSerialController          m_serial;
	unsigned char*             m_buffer;
	CRingBuffer<unsigned char> m_rxDStarData;
	CRingBuffer<unsigned char> m_txDStarData;
	CRingBuffer<unsigned char> m_rxDMRData1;
	CRingBuffer<unsigned char> m_rxDMRData2;
	CRingBuffer<unsigned char> m_txDMRData1;
	CRingBuffer<unsigned char> m_txDMRData2;
	CRingBuffer<unsigned char> m_rxYSFData;
	CRingBuffer<unsigned char> m_txYSFData;
	CTimer                     m_statusTimer;
	CTimer                     m_inactivityTimer;
	unsigned int               m_dstarSpace;
	unsigned int               m_dmrSpace1;
	unsigned int               m_dmrSpace2;
	unsigned int               m_ysfSpace;
	bool                       m_tx;
	bool                       m_lockout;

	bool readVersion();
	bool readStatus();
	bool setConfig();
	bool setFrequency();

	void printDebug();

	RESP_TYPE_MMDVM getResponse(unsigned char* buffer, unsigned int& length);
};

#endif
