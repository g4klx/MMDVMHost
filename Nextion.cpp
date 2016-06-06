/*
 *   Copyright (C) 2016 by Jonathan Naylor G4KLX
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

#include "Nextion.h"
#include "Log.h"
#include "Conf.h"
#include "FlagLookup.h"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <ctime>

CFLAGLookup* CNextion::m_flaglookup = NULL;

CNextion::CNextion(const std::string& callsign, unsigned int dmrid, const std::string& port, unsigned int brightness, unsigned int flagUsed, bool displayClock, bool utc) :
CDisplay(),
m_callsign(callsign),
m_dmrid(dmrid),
m_serial(port, SERIAL_9600),
m_brightness(brightness),
m_flagUsed(flagUsed),
m_mode(MODE_IDLE),
m_displayClock(displayClock),
m_utc(utc),
m_clockDisplayTimer(1000U, 0U, 400U)
{
	assert(brightness >= 0U && brightness <= 100U);
}

CNextion::~CNextion()
{
}

bool CNextion::open()
{
	bool ret = m_serial.open();
	if (!ret) {
		LogError("Cannot open the port for the Nextion display");
		return false;
	}
	sendCommand("bkcmd=0");
	char command[20U];
	::sprintf(command, "dim=%u", m_brightness);
	sendCommand(command);
	setIdle();

	return true;
}

void CNextion::setIdleInt()
{
	sendCommand("page MMDVM");
	char command[30];
	char callsign[8];
	::sprintf(command, "t0.txt=\"%-6s / %u\"", m_callsign.c_str(), m_dmrid);
	::sprintf(callsign, "%s", m_callsign.c_str());
	sendCommand(command);
        if (m_flagUsed==1) {
            std::string flag = m_flaglookup->flagFind(callsign);
            ::sprintf(command,"p1.pic=%s",flag.c_str());
            sendCommand(command);
           }

	sendCommand("t1.txt=\"MMDVM IDLE\"");

	m_clockDisplayTimer.start();

	m_mode = MODE_IDLE;
}

void CNextion::setErrorInt(const char* text)
{
	assert(text != NULL);

	sendCommand("page MMDVM");

	char command[20];
	::sprintf(command, "t0.txt=\"%s\"", text);
	sendCommand(command);
	sendCommand("t1.txt=\"ERROR\"");
        sendCommand("p1.pic=254");
        sendCommand("p2.pic=254");
	m_clockDisplayTimer.stop();
	m_mode = MODE_ERROR;
}

void CNextion::setLockoutInt()
{
	sendCommand("page MMDVM");

	sendCommand("t0.txt=\"LOCKOUT\"");
        sendCommand("p1.pic=254");
        sendCommand("p2.pic=254");

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
        char command[30];

	if (m_mode != MODE_DSTAR)
		sendCommand("page DStar");

        if (m_flagUsed==1) {
            std::string flag = m_flaglookup->flagFind(my1);
            ::sprintf(command,"p1.pic=%s",flag.c_str());
            sendCommand(command);
           }

	char text[30U];
	::sprintf(text, "t0.txt=\"%s %.8s/%4.4s\"", type, my1, my2);
	sendCommand(text);

	::sprintf(text, "t1.txt=\"%.8s\"", your);
	sendCommand(text);

	if (::strcmp(reflector, "        ") != 0) {
		::sprintf(text, "t2.txt=\"via %.8s\"", reflector);
		sendCommand(text);
	}

	m_clockDisplayTimer.stop();

	m_mode = MODE_DSTAR;
}

void CNextion::clearDStarInt()
{
	sendCommand("t0.txt=\"Listening\"");
	sendCommand("t1.txt=\"\"");
	sendCommand("t2.txt=\"\"");
	sendCommand("p1.pic=254");
        sendCommand("p2.pic=254");
}

void CNextion::writeDMRInt(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type)
{
	char callsign[8];
        char command[30];
	assert(type != NULL);

	if (m_mode != MODE_DMR) {
		sendCommand("page DMR");

		if (slotNo == 1U){
			sendCommand("t2.txt=\"2 Listening\"");
			sendCommand("p2.pic=254");
			}
		else
			{
			sendCommand("t0.txt=\"1 Listening\"");
			sendCommand("p1.pic=254");
			}
	}

	if (slotNo == 1U) {
		char text[30U];

		::sprintf(text, "t0.txt=\"1 %s %s\"", type, src.c_str());
		sendCommand(text);

		::sprintf(text, "t1.txt=\"%s%s\"", group ? "TG" : "", dst.c_str());
		sendCommand(text);

        if (m_flagUsed==1) {
            ::sprintf(callsign, "%-6s", src.c_str());
	    std::string flag = m_flaglookup->flagFind(callsign);
            ::sprintf(command,"p1.pic=%s",flag.c_str());
            sendCommand(command);
           }


	} else {
		char text[30U];

		::sprintf(text, "t2.txt=\"2 %s %s\"", type, src.c_str());
		sendCommand(text);

		::sprintf(text, "t3.txt=\"%s%s\"", group ? "TG" : "", dst.c_str());
		sendCommand(text);
        
	if (m_flagUsed==1) {
            ::sprintf(callsign, "%-6s", src.c_str());
	    std::string flag = m_flaglookup->flagFind(callsign);
            ::sprintf(command,"p2.pic=%s",flag.c_str());
            sendCommand(command);
           }

	}

	m_clockDisplayTimer.stop();

	m_mode = MODE_DMR;
}

void CNextion::clearDMRInt(unsigned int slotNo)
{
	if (slotNo == 1U) {
		sendCommand("t0.txt=\"1 Listening\"");
		sendCommand("t1.txt=\"\"");
		sendCommand("p1.pic=254");
	} else {
		sendCommand("t2.txt=\"2 Listening\"");
		sendCommand("t3.txt=\"\"");
		sendCommand("p2.pic=254");
	}
}

void CNextion::writeFusionInt(const char* source, const char* dest, const char* type, const char* origin)
{
	assert(source != NULL);
	assert(dest != NULL);
	assert(type != NULL);
	assert(origin != NULL);

	char callsign[8];
        char command[30];

	if (m_mode != MODE_YSF)
		sendCommand("page YSF");

	char text[30U];
	::sprintf(text, "t0.txt=\"%s %.10s\"", type, source);
	sendCommand(text);

	::sprintf(text, "t1.txt=\"%.10s\"", dest);
	sendCommand(text);
	if (::strcmp(origin, "          ") != 0) {
		::sprintf(text, "t2.txt=\"at %.10s\"", origin);
		sendCommand(text);
	}

	if (m_flagUsed==1) {
            ::sprintf(callsign, "%-6s",source);
	    std::string flag = m_flaglookup->flagFind(callsign);
            ::sprintf(command,"p1.pic=%s",flag.c_str());
            sendCommand(command);
           }
	m_clockDisplayTimer.stop();

	m_mode = MODE_YSF;
}

void CNextion::clearFusionInt()
{
	sendCommand("t0.txt=\"Listening\"");
	sendCommand("t1.txt=\"\"");
	sendCommand("t2.txt=\"\"");

	sendCommand("p1.pic=254"); // Clear both flags
	sendCommand("p2.pic=254");

	}

void CNextion::clockInt(unsigned int ms)
{
	// Update the clock display in IDLE mode every 400ms
	m_clockDisplayTimer.clock(ms);
	if (m_displayClock && m_mode == MODE_IDLE && m_clockDisplayTimer.isRunning() && m_clockDisplayTimer.hasExpired()) {
		time_t currentTime;
		struct tm *Time;
		::time(&currentTime);                   // Get the current time

		if (m_utc)
			Time = ::gmtime(&currentTime);
		else
			Time = ::localtime(&currentTime);

		int Day = Time->tm_mday;
		int Month = Time->tm_mon + 1;
		int Year = Time->tm_year + 1900;
		int Hour = Time->tm_hour;
		int Min = Time->tm_min;
		int Sec = Time->tm_sec;

		char text[50U];
		::sprintf(text, "t2.txt=\"%02d:%02d:%02d %02d/%02d/%2d\"", Hour, Min, Sec, Day, Month, Year % 100);
		sendCommand(text);

		m_clockDisplayTimer.start(); // restart the clock display timer
	}
}

void CNextion::close()
{
	m_serial.close();
}

void CNextion::sendCommand(const char* command)
{
	assert(command != NULL);

	m_serial.write((unsigned char*)command, ::strlen(command));
	m_serial.write((unsigned char*)"\xFF\xFF\xFF", 3U);
}
