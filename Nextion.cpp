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

#include "NetworkInfo.h"
#include "Nextion.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <ctime>
#include <clocale>
//#include <unistd.h>

const unsigned int DSTAR_RSSI_COUNT = 3U;		  // 3 * 420ms = 1260ms
const unsigned int DSTAR_BER_COUNT  = 63U;		// 63 * 20ms = 1260ms
const unsigned int DMR_RSSI_COUNT   = 4U;		  // 4 * 360ms = 1440ms
const unsigned int DMR_BER_COUNT    = 24U;		// 24 * 60ms = 1440ms
const unsigned int YSF_RSSI_COUNT   = 13U;		// 13 * 100ms = 1300ms
const unsigned int YSF_BER_COUNT    = 13U;		// 13 * 100ms = 1300ms
const unsigned int P25_RSSI_COUNT   = 7U;		  // 7 * 180ms = 1260ms
const unsigned int P25_BER_COUNT    = 7U;		  // 7 * 180ms = 1260ms
const unsigned int NXDN_RSSI_COUNT  = 28U;		  // 28 * 40ms = 1120ms
const unsigned int NXDN_BER_COUNT   = 28U;		  // 28 * 40ms = 1120ms

CNextion::CNextion(const std::string& callsign, unsigned int dmrid, ISerialPort* serial, unsigned int brightness, bool displayClock, bool utc, unsigned int idleBrightness, unsigned int screenLayout) :
CDisplay(),
m_callsign(callsign),
m_ipaddress("(ip unknown)"),
m_dmrid(dmrid),
m_serial(serial),
m_brightness(brightness),
m_mode(MODE_IDLE),
m_displayClock(displayClock),
m_utc(utc),
m_idleBrightness(idleBrightness),
m_screenLayout(screenLayout),
m_clockDisplayTimer(1000U, 0U, 400U),
m_rssiAccum1(0U),
m_rssiAccum2(0U),
m_berAccum1(0.0F),
m_berAccum2(0.0F),
m_rssiCount1(0U),
m_rssiCount2(0U),
m_berCount1(0U),
m_berCount2(0U)
{
	assert(serial != NULL);
	assert(brightness >= 0U && brightness <= 100U);
}

CNextion::~CNextion()
{
}

bool CNextion::open()
{
	unsigned char info[100U];
	CNetworkInfo* m_network;

	bool ret = m_serial->open();
	if (!ret) {
		LogError("Cannot open the port for the Nextion display");
		delete m_serial;
		return false;
	}

	info[0]=0;
	m_network = new CNetworkInfo;
	m_network->getNetworkInterface(info);
	m_ipaddress = (char*)info;

	sendCommand("bkcmd=0");
	sendCommandAction(0);

	setIdle();

	return true;
}


void CNextion::setIdleInt()
{
	sendCommand("page MMDVM");
	sendCommandAction(1U);

	char command[30U];
	::sprintf(command, "dim=%u", m_idleBrightness);
	sendCommand(command);
	
	::sprintf(command, "t0.txt=\"%s/%u\"", m_callsign.c_str(), m_dmrid);
	sendCommand(command);
	if (m_screenLayout > 2U) {
		::sprintf(command, "t4.txt=\"%s\"", m_callsign.c_str());
		sendCommand(command);
		::sprintf(command, "t5.txt=\"%u\"", m_dmrid);
		sendCommand(command);
	}
	sendCommandAction(17U);

	sendCommand("t1.txt=\"MMDVM IDLE\"");
	sendCommandAction(11U);

	::sprintf(command, "t3.txt=\"%s\"", m_ipaddress.c_str());
	sendCommand(command);
	sendCommandAction(16U);

	m_clockDisplayTimer.start();

	m_mode = MODE_IDLE;
}

void CNextion::setErrorInt(const char* text)
{
	assert(text != NULL);

	sendCommand("page MMDVM");
	sendCommandAction(1U);

	char command[20];
	::sprintf(command, "dim=%u", m_brightness);
	sendCommand(command);

	::sprintf(command, "t0.txt=\"%s\"", text);
	sendCommandAction(13U);

	sendCommand(command);
	sendCommand("t1.txt=\"ERROR\"");
	sendCommandAction(14U);

	m_clockDisplayTimer.stop();

	m_mode = MODE_ERROR;
}

void CNextion::setLockoutInt()
{
	sendCommand("page MMDVM");
	sendCommandAction(1U);

	char command[20];
	::sprintf(command, "dim=%u", m_brightness);
	sendCommand(command);

	sendCommand("t0.txt=\"LOCKOUT\"");
	sendCommandAction(15U);

	m_clockDisplayTimer.stop();

	m_mode = MODE_LOCKOUT;
}

void CNextion::writeDStarInt(const char* my1, const char* my2, const char* your, const char* type, const char* reflector)
{
	assert(my1 != NULL);
	assert(my2 != NULL);
	assert(your != NULL);
	assert(type != NULL);
	assert(reflector != NULL);

	if (m_mode != MODE_DSTAR) {
		sendCommand("page DStar");
		sendCommandAction(2U);
	}

	char text[50U];
	::sprintf(text, "dim=%u", m_brightness);
	sendCommand(text);

	::sprintf(text, "t0.txt=\"%s %.8s/%4.4s\"", type, my1, my2);
	sendCommand(text);
	sendCommandAction(42U);

	::sprintf(text, "t1.txt=\"%.8s\"", your);
	sendCommand(text);
	sendCommandAction(45U);

	if (::strcmp(reflector, "        ") != 0) {
		::sprintf(text, "t2.txt=\"via %.8s\"", reflector);
		sendCommand(text);
		sendCommandAction(46U);
	}

	m_clockDisplayTimer.stop();

	m_mode = MODE_DSTAR;
	m_rssiAccum1 = 0U;
	m_berAccum1  = 0.0F;
	m_rssiCount1 = 0U;
	m_berCount1  = 0U;
}

void CNextion::writeDStarRSSIInt(unsigned char rssi)
{
	m_rssiAccum1 += rssi;
	m_rssiCount1++;

	if (m_rssiCount1 == DSTAR_RSSI_COUNT) {
		char text[25U];
		::sprintf(text, "t3.txt=\"-%udBm\"", m_rssiAccum1 / DSTAR_RSSI_COUNT);
		sendCommand(text);
		sendCommandAction(47U);
		m_rssiAccum1 = 0U;
		m_rssiCount1 = 0U;
	}
}

void CNextion::writeDStarBERInt(float ber)
{
	m_berAccum1 += ber;
	m_berCount1++;

	if (m_berCount1 == DSTAR_BER_COUNT) {
		char text[25U];
		::sprintf(text, "t4.txt=\"%.1f%%\"", m_berAccum1 / float(DSTAR_BER_COUNT));
		sendCommand(text);
		sendCommandAction(48U);
		m_berAccum1 = 0.0F;
		m_berCount1 = 0U;
	}
}

void CNextion::clearDStarInt()
{
	sendCommand("t0.txt=\"Listening\"");
	sendCommandAction(41U);
	sendCommand("t1.txt=\"\"");
	sendCommand("t2.txt=\"\"");
	sendCommand("t3.txt=\"\"");
	sendCommand("t4.txt=\"\"");
}

void CNextion::writeDMRInt(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type)
{
	assert(type != NULL);

	if (m_mode != MODE_DMR) {
		sendCommand("page DMR");
		sendCommandAction(3U);


		if (slotNo == 1U) {
			if (m_screenLayout == 2U) {
			    sendCommand("t2.pco=0");
			    sendCommand("t2.font=4");
			}

			sendCommand("t2.txt=\"2 Listening\"");
			sendCommandAction(69U);
		} else {
			if (m_screenLayout == 2U) {
			    sendCommand("t0.pco=0");
			    sendCommand("t0.font=4");
			}

			sendCommand("t0.txt=\"1 Listening\"");
			sendCommandAction(61U);
		}
	}

	char text[50U];
	::sprintf(text, "dim=%u", m_brightness);
	sendCommand(text);

	if (slotNo == 1U) {
		::sprintf(text, "t0.txt=\"1 %s %s\"", type, src.c_str());

		if (m_screenLayout == 2U) {
		    sendCommand("t0.pco=0");
		    sendCommand("t0.font=4");
		}

		sendCommand(text);
		sendCommandAction(62U);

		::sprintf(text, "t1.txt=\"%s%s\"", group ? "TG" : "", dst.c_str());
		sendCommand(text);
		sendCommandAction(65U);
	} else {
		::sprintf(text, "t2.txt=\"2 %s %s\"", type, src.c_str());

		if (m_screenLayout == 2U) {
		    sendCommand("t2.pco=0");
		    sendCommand("t2.font=4");
		}

		sendCommand(text);
		sendCommandAction(70U);

		::sprintf(text, "t3.txt=\"%s%s\"", group ? "TG" : "", dst.c_str());
		sendCommand(text);
		sendCommandAction(73U);
	}

	m_clockDisplayTimer.stop();

	m_mode = MODE_DMR;
	m_rssiAccum1 = 0U;
	m_rssiAccum2 = 0U;
	m_berAccum1  = 0.0F;
	m_berAccum2  = 0.0F;
	m_rssiCount1 = 0U;
	m_rssiCount2 = 0U;
	m_berCount1  = 0U;
	m_berCount2  = 0U;
}

void CNextion::writeDMRRSSIInt(unsigned int slotNo, unsigned char rssi)
{
	if (slotNo == 1U) {
		m_rssiAccum1 += rssi;
		m_rssiCount1++;
    
		if (m_rssiCount1 == DMR_RSSI_COUNT) {
			char text[25U];
			::sprintf(text, "t4.txt=\"-%udBm\"", m_rssiAccum1 / DMR_RSSI_COUNT);
			sendCommand(text);
			sendCommandAction(66U);
			m_rssiAccum1 = 0U;
			m_rssiCount1 = 0U;
		}
	} else {
		m_rssiAccum2 += rssi;
		m_rssiCount2++;

		if (m_rssiCount2 == DMR_RSSI_COUNT) {
			char text[25U];
			::sprintf(text, "t5.txt=\"-%udBm\"", m_rssiAccum2 / DMR_RSSI_COUNT);
			sendCommand(text);
			sendCommandAction(74U);
			m_rssiAccum2 = 0U;
			m_rssiCount2 = 0U;
		}
	}
}

void CNextion::writeDMRTAInt(unsigned int slotNo, unsigned char* talkerAlias, const char* type)
{
	if (m_screenLayout < 2U)
		return;

	if (type[0] == ' ') {
		if (slotNo == 1U) {
			if (m_screenLayout == 2U) sendCommand("t0.pco=33808");
			sendCommandAction(64U);
		} else {
			if (m_screenLayout == 2U) sendCommand("t2.pco=33808");
			sendCommandAction(72U);
		}

		return;
	}

	if (slotNo == 1U) {
		char text[50U];
		::sprintf(text, "t0.txt=\"1 %s %s\"", type, talkerAlias);

		if (m_screenLayout == 2U) {
			if (::strlen((char*)talkerAlias) > (16U-4U))
				sendCommand("t0.font=3");
			if (::strlen((char*)talkerAlias) > (20U-4U))
				sendCommand("t0.font=2");
			if (::strlen((char*)talkerAlias) > (24U-4U))
				sendCommand("t0.font=1");

			sendCommand("t0.pco=1024");
		}

		sendCommand(text);
		sendCommandAction(63U);
	} else {
		char text[50U];
		::sprintf(text, "t2.txt=\"2 %s %s\"", type, talkerAlias);

		if (m_screenLayout == 2U) {
			if (::strlen((char*)talkerAlias) > (16U-4U))
				sendCommand("t2.font=3");
			if (::strlen((char*)talkerAlias) > (20U-4U))
				sendCommand("t2.font=2");
			if (::strlen((char*)talkerAlias) > (24U-4U))
				sendCommand("t2.font=1");

			sendCommand("t2.pco=1024");
		}
		sendCommand(text);
		sendCommandAction(71U);
	}
}

void CNextion::writeDMRBERInt(unsigned int slotNo, float ber)
{
	if (slotNo == 1U) {
		m_berAccum1 += ber;
		m_berCount1++;

		if (m_berCount1 == DMR_BER_COUNT) {
			char text[25U];
			::sprintf(text, "t6.txt=\"%.1f%%\"", m_berAccum1 / DMR_BER_COUNT);
			sendCommand(text);
			sendCommandAction(67U);
			m_berAccum1 = 0U;
			m_berCount1 = 0U;
		}
	} else {
		m_berAccum2 += ber;
		m_berCount2++;

		if (m_berCount2 == DMR_BER_COUNT) {
			char text[25U];
			::sprintf(text, "t7.txt=\"%.1f%%\"", m_berAccum2 / DMR_BER_COUNT);
			sendCommand(text);
			sendCommandAction(75U);
			m_berAccum2 = 0U;
			m_berCount2 = 0U;
		}
	}
}

void CNextion::clearDMRInt(unsigned int slotNo)
{
	if (slotNo == 1U) {
		sendCommand("t0.txt=\"1 Listening\"");
		sendCommandAction(61U);

		if (m_screenLayout == 2U) {
		    sendCommand("t0.pco=0");
		    sendCommand("t0.font=4");
		}

		sendCommand("t1.txt=\"\"");
		sendCommand("t4.txt=\"\"");
		sendCommand("t6.txt=\"\"");
	} else {
		sendCommand("t2.txt=\"2 Listening\"");
		sendCommandAction(69U);

		if (m_screenLayout == 2U) {
		    sendCommand("t2.pco=0");
		    sendCommand("t2.font=4");
		}

		sendCommand("t3.txt=\"\"");
		sendCommand("t5.txt=\"\"");
		sendCommand("t7.txt=\"\"");
	}
}

void CNextion::writeFusionInt(const char* source, const char* dest, const char* type, const char* origin)
{
	assert(source != NULL);
	assert(dest != NULL);
	assert(type != NULL);
	assert(origin != NULL);

	if (m_mode != MODE_YSF) {
		sendCommand("page YSF");
		sendCommandAction(4U);
	}


	char text[30U];
	::sprintf(text, "dim=%u", m_brightness);
	sendCommand(text);

	::sprintf(text, "t0.txt=\"%s %.10s\"", type, source);
	sendCommand(text);
	sendCommandAction(82U);

	::sprintf(text, "t1.txt=\"%.10s\"", dest);
	sendCommand(text);
	sendCommandAction(83U);
	if (::strcmp(origin, "          ") != 0) {
		::sprintf(text, "t2.txt=\"at %.10s\"", origin);
		sendCommand(text);
		sendCommandAction(84U);
	}

	m_clockDisplayTimer.stop();

	m_mode = MODE_YSF;
	m_rssiAccum1 = 0U;
	m_berAccum1  = 0.0F;
	m_rssiCount1 = 0U;
	m_berCount1  = 0U;
}

void CNextion::writeFusionRSSIInt(unsigned char rssi)
{
	m_rssiAccum1 += rssi;
	m_rssiCount1++;

	if (m_rssiCount1 == YSF_RSSI_COUNT) {
		char text[25U];
		::sprintf(text, "t3.txt=\"-%udBm\"", m_rssiAccum1 / YSF_RSSI_COUNT);
		sendCommand(text);
		sendCommandAction(85U);
		m_rssiAccum1 = 0U;
		m_rssiCount1 = 0U;
	}
}

void CNextion::writeFusionBERInt(float ber)
{
	m_berAccum1 += ber;
	m_berCount1++;

	if (m_berCount1 == YSF_BER_COUNT) {
		char text[25U];
		::sprintf(text, "t4.txt=\"%.1f%%\"", m_berAccum1 / float(YSF_BER_COUNT));
		sendCommand(text);
		sendCommandAction(86U);
		m_berAccum1 = 0.0F;
		m_berCount1 = 0U;
	}
}

void CNextion::clearFusionInt()
{
	sendCommand("t0.txt=\"Listening\"");
	sendCommandAction(81U);
	sendCommand("t1.txt=\"\"");
	sendCommand("t2.txt=\"\"");
	sendCommand("t3.txt=\"\"");
	sendCommand("t4.txt=\"\"");
}

void CNextion::writeP25Int(const char* source, bool group, unsigned int dest, const char* type)
{
	assert(source != NULL);
	assert(type != NULL);

	if (m_mode != MODE_P25) {
		sendCommand("page P25");
		sendCommandAction(5U);
	}

	char text[30U];
	::sprintf(text, "dim=%u", m_brightness);
	sendCommand(text);

	::sprintf(text, "t0.txt=\"%s %.10s\"", type, source);
	sendCommand(text);
	sendCommandAction(102U);

	::sprintf(text, "t1.txt=\"%s%u\"", group ? "TG" : "", dest);
	sendCommand(text);
	sendCommandAction(103U);

	m_clockDisplayTimer.stop();

	m_mode = MODE_P25;
	m_rssiAccum1 = 0U;
	m_berAccum1  = 0.0F;
	m_rssiCount1 = 0U;
	m_berCount1  = 0U;
}

void CNextion::writeP25RSSIInt(unsigned char rssi)
{
	m_rssiAccum1 += rssi;
	m_rssiCount1++;

	if (m_rssiCount1 == P25_RSSI_COUNT) {
		char text[25U];
		::sprintf(text, "t2.txt=\"-%udBm\"", m_rssiAccum1 / P25_RSSI_COUNT);
		sendCommand(text);
		sendCommandAction(104U);
		m_rssiAccum1 = 0U;
		m_rssiCount1 = 0U;
	}
}

void CNextion::writeP25BERInt(float ber)
{
	m_berAccum1 += ber;
	m_berCount1++;

	if (m_berCount1 == P25_BER_COUNT) {
		char text[25U];
		::sprintf(text, "t3.txt=\"%.1f%%\"", m_berAccum1 / float(P25_BER_COUNT));
		sendCommand(text);
		sendCommandAction(105U);
		m_berAccum1 = 0.0F;
		m_berCount1 = 0U;
	}
}

void CNextion::clearP25Int()
{
	sendCommand("t0.txt=\"Listening\"");
	sendCommandAction(101U);
	sendCommand("t1.txt=\"\"");
	sendCommand("t2.txt=\"\"");
	sendCommand("t3.txt=\"\"");
}

void CNextion::writeNXDNInt(const char* source, bool group, unsigned int dest, const char* type)
{
	assert(source != NULL);
	assert(type != NULL);

	if (m_mode != MODE_NXDN) {
		sendCommand("page NXDN");
		sendCommandAction(7U);
	}

	char text[30U];
	::sprintf(text, "dim=%u", m_brightness);
	sendCommand(text);

	::sprintf(text, "t0.txt=\"%s %.10s\"", type, source);
	sendCommand(text);
	sendCommandAction(122U);

	::sprintf(text, "t1.txt=\"%s%u\"", group ? "TG" : "", dest);
	sendCommand(text);
	sendCommandAction(123U);

	m_clockDisplayTimer.stop();

	m_mode = MODE_NXDN;
	m_rssiAccum1 = 0U;
	m_berAccum1  = 0.0F;
	m_rssiCount1 = 0U;
	m_berCount1  = 0U;
}

void CNextion::writeNXDNRSSIInt(unsigned char rssi)
{
	m_rssiAccum1 += rssi;
	m_rssiCount1++;

	if (m_rssiCount1 == NXDN_RSSI_COUNT) {
		char text[25U];
		::sprintf(text, "t2.txt=\"-%udBm\"", m_rssiAccum1 / NXDN_RSSI_COUNT);
		sendCommand(text);
		sendCommandAction(124U);
		m_rssiAccum1 = 0U;
		m_rssiCount1 = 0U;
	}
}

void CNextion::writeNXDNBERInt(float ber)
{
	m_berAccum1 += ber;
	m_berCount1++;

	if (m_berCount1 == NXDN_BER_COUNT) {
		char text[25U];
		::sprintf(text, "t3.txt=\"%.1f%%\"", m_berAccum1 / float(NXDN_BER_COUNT));
		sendCommand(text);
		sendCommandAction(125U);
		m_berAccum1 = 0.0F;
		m_berCount1 = 0U;
	}
}

void CNextion::clearNXDNInt()
{
	sendCommand("t0.txt=\"Listening\"");
	sendCommandAction(121U);
	sendCommand("t1.txt=\"\"");
	sendCommand("t2.txt=\"\"");
	sendCommand("t3.txt=\"\"");
}

void CNextion::writePOCSAGInt(uint32_t ric, const std::string& message)
{
	if (m_mode != MODE_POCSAG) {
		sendCommand("page POCSAG");
		sendCommandAction(6U);
	}

	char text[200U];
	::sprintf(text, "dim=%u", m_brightness);
	sendCommand(text);

	::sprintf(text, "t0.txt=\"RIC: %u\"", ric);
	sendCommand(text);
	sendCommandAction(132U);

	::sprintf(text, "t1.txt=\"%s\"", message.c_str());
	sendCommand(text);
	sendCommandAction(133U);

	m_clockDisplayTimer.stop();

	m_mode = MODE_POCSAG;
}

void CNextion::clearPOCSAGInt()
{
	sendCommand("t0.txt=\"Waiting\"");
	sendCommandAction(134U);
	sendCommand("t1.txt=\"\"");
}

void CNextion::writeCWInt()
{
	sendCommand("t1.txt=\"Sending CW Ident\"");
	sendCommandAction(12U);
	m_clockDisplayTimer.start();

	m_mode = MODE_CW;
}

void CNextion::clearCWInt()
{
	sendCommand("t1.txt=\"MMDVM IDLE\"");
	sendCommandAction(11U);
}

void CNextion::clockInt(unsigned int ms)
{
	// Update the clock display in IDLE mode every 400ms
	m_clockDisplayTimer.clock(ms);
	if (m_displayClock && (m_mode == MODE_IDLE || m_mode == MODE_CW) && m_clockDisplayTimer.isRunning() && m_clockDisplayTimer.hasExpired()) {
		time_t currentTime;
		struct tm *Time;
		::time(&currentTime);                   // Get the current time

		if (m_utc)
			Time = ::gmtime(&currentTime);
		else
			Time = ::localtime(&currentTime);

		setlocale(LC_TIME,"");
		char text[50U];
		strftime(text, 50, "t2.txt=\"%x %X\"", Time);
		sendCommand(text);

		m_clockDisplayTimer.start(); // restart the clock display timer
	}
}

void CNextion::close()
{
	sendCommand("page MMDVM");
	sendCommandAction(1U);
	sendCommand("t1.txt=\"MMDVM STOPPED\"");
	sendCommandAction(19U);
	m_serial->close();
	delete m_serial;
}

void CNextion::sendCommandAction(unsigned int status)
{
    if (m_screenLayout<3U) return;

    char text[30U];
    ::sprintf(text, "MMDVM.status.val=%d", status);
    sendCommand(text);
    sendCommand("click S0,1");
}


void CNextion::sendCommand(const char* command)
{
	assert(command != NULL);

	m_serial->write((unsigned char*)command, (unsigned int)::strlen(command));
	m_serial->write((unsigned char*)"\xFF\xFF\xFF", 3U);
}
