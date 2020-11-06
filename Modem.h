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

#include "Defines.h"

#include <string>

class IModem {
public:
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
	virtual unsigned int readTransparentData(unsigned char* data) = 0;

	virtual unsigned int readSerial(unsigned char* data, unsigned int length) = 0;

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

	virtual bool writeTransparentData(const unsigned char* data, unsigned int length) = 0;

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

	virtual bool writeSerial(const unsigned char* data, unsigned int length) = 0;

	virtual unsigned char getMode() const = 0;
	virtual bool setMode(unsigned char mode) = 0;

	virtual bool sendCWId(const std::string& callsign) = 0;

	virtual HW_TYPE getHWType() const = 0;

	virtual void clock(unsigned int ms) = 0;

	virtual void close() = 0;

private:
};

#endif
