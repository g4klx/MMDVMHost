/*
 *   Copyright (C) 2016,2017,2018 by Jonathan Naylor G4KLX
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

#if !defined(DISPLAY_H)
#define	DISPLAY_H

#include "Timer.h"

#include <string>

#include <cstdint>

class CDisplay
{
public:
	CDisplay();
	virtual ~CDisplay() = 0;

	virtual bool open() = 0;

	void setIdle();
	void setLockout();
	void setError(const char* text);

	void writeDStar(const char* my1, const char* my2, const char* your, const char* type, const char* reflector);
	void writeDStarRSSI(unsigned char rssi);
	void writeDStarBER(float ber);
	void clearDStar();

	void writeDMR(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type);
	void writeDMRRSSI(unsigned int slotNo, unsigned char rssi);
	void writeDMRBER(unsigned int slotNo, float ber);
	void writeDMRTA(unsigned int slotNo, unsigned char* talkerAlias, const char* type);
	void clearDMR(unsigned int slotNo);

	void writeFusion(const char* source, const char* dest, const char* type, const char* origin);
	void writeFusionRSSI(unsigned char rssi);
	void writeFusionBER(float ber);
	void clearFusion();

	void writeP25(const char* source, bool group, unsigned int dest, const char* type);
	void writeP25RSSI(unsigned char rssi);
	void writeP25BER(float ber);
	void clearP25();

	void writeNXDN(const char* source, bool group, unsigned int dest, const char* type);
	void writeNXDNRSSI(unsigned char rssi);
	void writeNXDNBER(float ber);
	void clearNXDN();

	void writePOCSAG(uint32_t ric, const std::string& message);
	void clearPOCSAG();

	void writeCW();

	virtual void close() = 0;

	void clock(unsigned int ms);

protected:
	virtual void setIdleInt() = 0;
	virtual void setLockoutInt() = 0;
	virtual void setErrorInt(const char* text) = 0;

	virtual void writeDStarInt(const char* my1, const char* my2, const char* your, const char* type, const char* reflector) = 0;
	virtual void writeDStarRSSIInt(unsigned char rssi);
	virtual void writeDStarBERInt(float ber);
	virtual void clearDStarInt() = 0;

	virtual void writeDMRInt(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type) = 0;
	virtual void writeDMRRSSIInt(unsigned int slotNo, unsigned char rssi);
	virtual void writeDMRTAInt(unsigned int slotNo, unsigned char* talkerAlias, const char* type);
	virtual void writeDMRBERInt(unsigned int slotNo, float ber);
	virtual void clearDMRInt(unsigned int slotNo) = 0;

	virtual void writeFusionInt(const char* source, const char* dest, const char* type, const char* origin) = 0;
	virtual void writeFusionRSSIInt(unsigned char rssi);
	virtual void writeFusionBERInt(float ber);
	virtual void clearFusionInt() = 0;

	virtual void writeP25Int(const char* source, bool group, unsigned int dest, const char* type) = 0;
	virtual void writeP25RSSIInt(unsigned char rssi);
	virtual void writeP25BERInt(float ber);
	virtual void clearP25Int() = 0;

	virtual void writeNXDNInt(const char* source, bool group, unsigned int dest, const char* type) = 0;
	virtual void writeNXDNRSSIInt(unsigned char rssi);
	virtual void writeNXDNBERInt(float ber);
	virtual void clearNXDNInt() = 0;

	virtual void writePOCSAGInt(uint32_t ric, const std::string& message) = 0;
	virtual void clearPOCSAGInt() = 0;

	virtual void writeCWInt() = 0;
	virtual void clearCWInt() = 0;

	virtual void clockInt(unsigned int ms);

private:
	CTimer        m_timer1;
	CTimer        m_timer2;
	unsigned char m_mode1;
	unsigned char m_mode2;
};

#endif
