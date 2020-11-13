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

#ifndef	NULLMODEM_H
#define	NULLMODEM_H

#include "Modem.h"
#include "Defines.h"

#include <string>

class CNullModem : public IModem {
public:
	CNullModem();
	virtual ~CNullModem();

	virtual void setSerialParams(const std::string& protocol, unsigned int address, unsigned int speed) {};
	virtual void setRFParams(unsigned int rxFrequency, int rxOffset, unsigned int txFrequency, int txOffset, int txDCOffset, int rxDCOffset, float rfLevel, unsigned int pocsagFrequency) {};
	virtual void setModeParams(bool dstarEnabled, bool dmrEnabled, bool ysfEnabled, bool p25Enabled, bool nxdnEnabled, bool m17Enabled, bool pocsagEnabled, bool fmEnabled, bool ax25Enabled) {};
	virtual void setLevels(float rxLevel, float cwIdTXLevel, float dstarTXLevel, float dmrTXLevel, float ysfTXLevel, float p25TXLevel, float nxdnTXLevel, float m17TXLevel, float pocsagLevel, float fmTXLevel, float ax25TXLevel) {};
	virtual void setDMRParams(unsigned int colorCode) {};
	virtual void setYSFParams(bool loDev, unsigned int txHang) {};
	virtual void setP25Params(unsigned int txHang) {};
	virtual void setNXDNParams(unsigned int txHang) {};
	virtual void setM17Params(unsigned int txHang) {};
	virtual void setAX25Params(int rxTwist, unsigned int txDelay, unsigned int slotTime, unsigned int pPersist) {};
	virtual void setTransparentDataParams(unsigned int sendFrameType) {};

	virtual void setFMCallsignParams(const std::string& callsign, unsigned int callsignSpeed, unsigned int callsignFrequency, unsigned int callsignTime, unsigned int callsignHoldoff, float callsignHighLevel, float callsignLowLevel, bool callsignAtStart, bool callsignAtEnd, bool callsignAtLatch) {};
	virtual void setFMAckParams(const std::string& rfAck, unsigned int ackSpeed, unsigned int ackFrequency, unsigned int ackMinTime, unsigned int ackDelay, float ackLevel) {};
	virtual void setFMMiscParams(unsigned int timeout, float timeoutLevel, float ctcssFrequency, unsigned int ctcssHighThreshold, unsigned int ctcssLowThreshold, float ctcssLevel, unsigned int kerchunkTime, unsigned int hangTime, unsigned int accessMode, bool cosInvert, bool noiseSquelch, unsigned int squelchHighThreshold, unsigned int squelchLowThreshold, unsigned int rfAudioBoost, float maxDevLevel) {};
	virtual void setFMExtParams(const std::string& ack, unsigned int audioBoost) {};

	virtual bool open();

	virtual unsigned int readDStarData(unsigned char* data) { return 0U; };
	virtual unsigned int readDMRData1(unsigned char* data) { return 0U; };
	virtual unsigned int readDMRData2(unsigned char* data) { return 0U; };
	virtual unsigned int readYSFData(unsigned char* data) { return 0U; };
	virtual unsigned int readP25Data(unsigned char* data) { return 0U; };
	virtual unsigned int readNXDNData(unsigned char* data) { return 0U; };
	virtual unsigned int readM17Data(unsigned char* data) { return 0U; };
	virtual unsigned int readFMData(unsigned char* data) { return 0U; };
	virtual unsigned int readAX25Data(unsigned char* data) { return 0U; };

	virtual bool hasDStarSpace()const { return true; };
	virtual bool hasDMRSpace1() const { return true; };
	virtual bool hasDMRSpace2() const { return true; };
	virtual bool hasYSFSpace() const  { return true; };
	virtual bool hasP25Space() const  { return true; };
	virtual bool hasNXDNSpace() const { return true; };
	virtual bool hasM17Space() const { return true; };
	virtual bool hasPOCSAGSpace() const { return true; };
	virtual unsigned int getFMSpace() const { return true; };
	virtual bool hasAX25Space() const { return true; };

	virtual bool hasTX() const { return false; };
	virtual bool hasCD() const { return false; };

	virtual bool hasLockout() const { return false; };
	virtual bool hasError() const   { return false; };

	virtual bool writeDStarData(const unsigned char* data, unsigned int length) { return true; };
	virtual bool writeDMRData1(const unsigned char* data, unsigned int length) { return true; };
	virtual bool writeDMRData2(const unsigned char* data, unsigned int length) { return true; };
	virtual bool writeYSFData(const unsigned char* data, unsigned int length) { return true; };
	virtual bool writeP25Data(const unsigned char* data, unsigned int length) { return true; };
	virtual bool writeNXDNData(const unsigned char* data, unsigned int length) { return true; };
	virtual bool writeM17Data(const unsigned char* data, unsigned int length) { return true; };
	virtual bool writePOCSAGData(const unsigned char* data, unsigned int length) { return true; };
	virtual bool writeFMData(const unsigned char* data, unsigned int length) { return true; };
	virtual bool writeAX25Data(const unsigned char* data, unsigned int length) { return true; };

	virtual bool writeConfig() { return true; };
	virtual bool writeDStarInfo(const char* my1, const char* my2, const char* your, const char* type, const char* reflector) { return true; };
	virtual bool writeDMRInfo(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type) { return true; };
	virtual bool writeYSFInfo(const char* source, const char* dest, unsigned char dgid, const char* type, const char* origin) { return true; };
	virtual bool writeP25Info(const char* source, bool group, unsigned int dest, const char* type) { return true; };
	virtual bool writeNXDNInfo(const char* source, bool group, unsigned int dest, const char* type) { return true; };
	virtual bool writeM17Info(const char* source, const char* dest, const char* type) { return true; };
	virtual bool writePOCSAGInfo(unsigned int ric, const std::string& message) { return true; };
	virtual bool writeIPInfo(const std::string& address) { return true; };

	virtual bool writeDMRStart(bool tx) { return true; };
	virtual bool writeDMRShortLC(const unsigned char* lc) { return true; };
	virtual bool writeDMRAbort(unsigned int slotNo) { return true; };

	virtual bool writeTransparentData(const unsigned char* data, unsigned int length) { return true; };
	virtual unsigned int readTransparentData(unsigned char* data) { return 0U; };

	virtual bool writeSerial(const unsigned char* data, unsigned int length) { return true; };
	virtual unsigned int readSerial(unsigned char* data, unsigned int length) { return 0U; };

	virtual bool writeI2C(const unsigned char* data, unsigned int length) { return true; };
	virtual unsigned int readI2C(unsigned char* data, unsigned int length) { return 0U; };

	virtual unsigned char getMode() const { return MODE_IDLE; };
	virtual bool setMode(unsigned char mode) { return true; };

	virtual bool sendCWId(const std::string& callsign) { return true; };

	virtual HW_TYPE getHWType() const { return HWT_MMDVM; };

	virtual void clock(unsigned int ms) {};

	virtual void close() {};

private:
};

#endif
