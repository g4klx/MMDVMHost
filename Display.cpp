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

#include "Display.h"
#include "Defines.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

CDisplay::CDisplay() :
m_timer1(3000U, 3U),
m_timer2(3000U, 3U),
m_mode1(MODE_IDLE),
m_mode2(MODE_IDLE)
{
}

CDisplay::~CDisplay()
{
}

void CDisplay::setIdle()
{
	m_timer1.stop();
	m_timer2.stop();

	m_mode1 = MODE_IDLE;
	m_mode2 = MODE_IDLE;

	setIdleInt();
}

void CDisplay::setLockout()
{
	m_timer1.stop();
	m_timer2.stop();

	m_mode1 = MODE_IDLE;
	m_mode2 = MODE_IDLE;

	setLockoutInt();
}

void CDisplay::setError(const char* text)
{
	assert(text != NULL);

	m_timer1.stop();
	m_timer2.stop();

	m_mode1 = MODE_IDLE;
	m_mode2 = MODE_IDLE;

	setErrorInt(text);
}

void CDisplay::writeDStar(const char* my1, const char* my2, const char* your, const char* type, const char* reflector)
{
	assert(my1 != NULL);
	assert(my2 != NULL);
	assert(your != NULL);
	assert(type != NULL);
	assert(reflector != NULL);

	m_timer1.start();
	m_mode1 = MODE_IDLE;

	writeDStarInt(my1, my2, your, type, reflector);
}

void CDisplay::writeDStarRSSI(unsigned char rssi)
{
	if (rssi != 0U)
		writeDStarRSSIInt(rssi);
}

void CDisplay::writeDStarBER(float ber)
{
	writeDStarBERInt(ber);
}

void CDisplay::clearDStar()
{
	if (m_timer1.hasExpired()) {
		clearDStarInt();
		m_timer1.stop();
		m_mode1 = MODE_IDLE;
	} else {
		m_mode1 = MODE_DSTAR;
	}
}

void CDisplay::writeDMR(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type)
{
	assert(type != NULL);

	if (slotNo == 1U) {
		m_timer1.start();
		m_mode1 = MODE_IDLE;
	} else {
		m_timer2.start();
		m_mode2 = MODE_IDLE;
	}
	writeDMRInt(slotNo, src, group, dst, type);
}

void CDisplay::writeDMRRSSI(unsigned int slotNo, unsigned char rssi)
{
	if (rssi != 0U)
		writeDMRRSSIInt(slotNo, rssi);
}

void CDisplay::writeDMRTA(unsigned int slotNo, unsigned char* talkerAlias, const char* type)
{
    char TA[32U];
    unsigned char *b;
    unsigned char c;
    int j;
    unsigned int i,t1,t2, TAsize, TAformat;

    if (strcmp(type," ")==0) { writeDMRTAInt(slotNo, (unsigned char*)TA, type); return; }

    TAformat=(talkerAlias[0]>>6U) & 0x03U;
    TAsize = (talkerAlias[0]>>1U) & 0x1FU;
    ::strcpy(TA,"(could not decode)");
    switch (TAformat) {
	case 0U:		// 7 bit
		::memset (&TA,0,32U);
		b=&talkerAlias[0];
		t1=0; t2=0; c=0;
		for (i=0;(i<32U)&&(t2<TAsize);i++) {
		    for (j=7U;j>=0;j--) {
			c = (c<<1U) | (b[i] >> j);
			if (++t1==7U) { if (i>0) {TA[t2++]=c & 0x7FU; } t1=0; c=0; }
		    }
		}
		break;
	case 1U:		// ISO 8 bit
	case 2U:		// UTF8
		::strcpy(TA,(char*)talkerAlias+1U);
		break;
	case 3U:		// UTF16 poor man's conversion
		t2=0;
		::memset (&TA,0,32U);
		for(i=0;(i<15)&&(t2<TAsize);i++) {
		    if (talkerAlias[2U*i+1U]==0)
			TA[t2++]=talkerAlias[2U*i+2U]; else TA[t2++]='?';
		}
		TA[TAsize]=0;
		break;
    }
    LogMessage("DMR Talker Alias (Data Format %u, Received %u/%u char): '%s'", TAformat, ::strlen(TA), TAsize, TA);
    if (::strlen(TA)>TAsize) { if (strlen(TA)<29U) strcat(TA," ?"); else strcpy(TA+28U," ?"); }
    if (strlen((char*)TA)>=4U) writeDMRTAInt(slotNo, (unsigned char*)TA, type);

}

void CDisplay::writeDMRBER(unsigned int slotNo, float ber)
{
	writeDMRBERInt(slotNo, ber);
}
void CDisplay::clearDMR(unsigned int slotNo)
{
	if (slotNo == 1U) {
		if (m_timer1.hasExpired()) {
			clearDMRInt(slotNo);
			m_timer1.stop();
			m_mode1 = MODE_IDLE;
		} else {
			m_mode1 = MODE_DMR;
		}
	} else {
		if (m_timer2.hasExpired()) {
			clearDMRInt(slotNo);
			m_timer2.stop();
			m_mode2 = MODE_IDLE;
		} else {
			m_mode2 = MODE_DMR;
		}
	}
}

void CDisplay::writeFusion(const char* source, const char* dest, const char* type, const char* origin)
{
	assert(source != NULL);
	assert(dest != NULL);
	assert(type != NULL);
	assert(origin != NULL);

	m_timer1.start();
	m_mode1 = MODE_IDLE;

	writeFusionInt(source, dest, type, origin);
}

void CDisplay::writeFusionRSSI(unsigned char rssi)
{
	if (rssi != 0U)
		writeFusionRSSIInt(rssi);
}

void CDisplay::writeFusionBER(float ber)
{
	writeFusionBERInt(ber);
}

void CDisplay::clearFusion()
{
	if (m_timer1.hasExpired()) {
		clearFusionInt();
		m_timer1.stop();
		m_mode1 = MODE_IDLE;
	} else {
		m_mode1 = MODE_YSF;
	}
}

void CDisplay::writeP25(const char* source, bool group, unsigned int dest, const char* type)
{
	assert(source != NULL);
	assert(type != NULL);

	m_timer1.start();
	m_mode1 = MODE_IDLE;

	writeP25Int(source, group, dest, type);
}

void CDisplay::writeP25RSSI(unsigned char rssi)
{
	if (rssi != 0U)
		writeP25RSSIInt(rssi);
}

void CDisplay::writeP25BER(float ber)
{
	writeP25BERInt(ber);
}

void CDisplay::clearP25()
{
	if (m_timer1.hasExpired()) {
		clearP25Int();
		m_timer1.stop();
		m_mode1 = MODE_IDLE;
	} else {
		m_mode1 = MODE_P25;
	}
}

void CDisplay::writeNXDN(const char* source, bool group, unsigned int dest, const char* type)
{
	assert(source != NULL);
	assert(type != NULL);

	m_timer1.start();
	m_mode1 = MODE_IDLE;

	writeNXDNInt(source, group, dest, type);
}

void CDisplay::writeNXDNRSSI(unsigned char rssi)
{
	if (rssi != 0U)
		writeNXDNRSSIInt(rssi);
}

void CDisplay::writeNXDNBER(float ber)
{
	writeNXDNBERInt(ber);
}

void CDisplay::clearNXDN()
{
	if (m_timer1.hasExpired()) {
		clearNXDNInt();
		m_timer1.stop();
		m_mode1 = MODE_IDLE;
	} else {
		m_mode1 = MODE_NXDN;
	}
}

void CDisplay::writePOCSAG(uint32_t ric, const std::string& message)
{
	m_timer1.start();
	m_mode1 = MODE_POCSAG;

	writePOCSAGInt(ric, message);
}

void CDisplay::clearPOCSAG()
{
	if (m_timer1.hasExpired()) {
		clearPOCSAGInt();
		m_timer1.stop();
		m_mode1 = MODE_IDLE;
	} else {
		m_mode1 = MODE_POCSAG;
	}
}

void CDisplay::writeCW()
{
	m_timer1.start();
	m_mode1 = MODE_CW;

	writeCWInt();
}

void CDisplay::clock(unsigned int ms)
{
	m_timer1.clock(ms);
	if (m_timer1.isRunning() && m_timer1.hasExpired()) {
		switch (m_mode1) {
		case MODE_DSTAR:
			clearDStarInt();
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		case MODE_DMR:
			clearDMRInt(1U);
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		case MODE_YSF:
			clearFusionInt();
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		case MODE_P25:
			clearP25Int();
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		case MODE_NXDN:
			clearNXDNInt();
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		case MODE_POCSAG:
			clearPOCSAGInt();
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		case MODE_CW:
			clearCWInt();
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		default:
			break;
		}
	}

	// Timer/mode 2 are only used for DMR
	m_timer2.clock(ms);
	if (m_timer2.isRunning() && m_timer2.hasExpired()) {
		if (m_mode2 == MODE_DMR) {
			clearDMRInt(2U);
			m_mode2 = MODE_IDLE;
			m_timer2.stop();
		}
	}

	clockInt(ms);
}

void CDisplay::clockInt(unsigned int ms)
{
}

void CDisplay::writeDStarRSSIInt(unsigned char rssi)
{
}

void CDisplay::writeDStarBERInt(float ber)
{
}

void CDisplay::writeDMRRSSIInt(unsigned int slotNo, unsigned char rssi)
{
}

void CDisplay::writeDMRTAInt(unsigned int slotNo, unsigned char* talkerAlias, const char* type)
{
}

void CDisplay::writeDMRBERInt(unsigned int slotNo, float ber)
{
}

void CDisplay::writeFusionRSSIInt(unsigned char rssi)
{
}

void CDisplay::writeFusionBERInt(float ber)
{
}

void CDisplay::writeP25RSSIInt(unsigned char rssi)
{
}

void CDisplay::writeP25BERInt(float ber)
{
}

void CDisplay::writeNXDNRSSIInt(unsigned char rssi)
{
}

void CDisplay::writeNXDNBERInt(float ber)
{
}
