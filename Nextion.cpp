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

#include <cstdio>
#include <cassert>
#include <cstring>

CNextion::CNextion(const std::string& port, unsigned int brightness) :
m_serial(port, SERIAL_9600),
m_brightness(brightness)
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

void CNextion::setIdle()
{
	sendCommand("page MMDVM");

	sendCommand("t0.txt=\"IDLE\"");
}

void CNextion::setError(const char* text)
{
	assert(text != NULL);

	sendCommand("page MMDVM");

	char command[20];
	::sprintf(command, "t0.txt=\"%s\"", text);

	sendCommand(command);
	sendCommand("t1.txt=\"ERROR\"");
}

void CNextion::setLockout()
{
	sendCommand("page MMDVM");

	sendCommand("t0.txt=\"LOCKOUT\"");
}

void CNextion::setDStar()
{
	sendCommand("page DStar");

	sendCommand("t0.txt=\"Listening\"");
}

void CNextion::writeDStar(const char* my1, const char* my2, const char* your)
{
	assert(my1 != NULL);
	assert(my2 != NULL);
	assert(your != NULL);

	char text[30U];
	::sprintf(text, "t0.txt=\"%.8s/%4.4s\"", my1, my2);
	sendCommand(text);

	::sprintf(text, "t1.txt=\"%.8s\"", your);
	sendCommand(text);
}

void CNextion::clearDStar()
{
	sendCommand("t0.txt=\"Listening\"");
	sendCommand("t1.txt=\"\"");
}

void CNextion::setDMR()
{
	sendCommand("page DMR");

	sendCommand("t0.txt=\"1 Listening\"");
	sendCommand("t2.txt=\"2 Listening\"");
}

void CNextion::writeDMR(unsigned int slotNo, unsigned int srcId, bool group, unsigned int dstId, const char* type)
{
	assert(type != NULL);

	if (slotNo == 1U) {
		char text[30U];

		::sprintf(text, "t0.txt=\"1 %s %u\"", type, srcId);
		sendCommand(text);

		::sprintf(text, "t1.txt=\"%s%u\"", group ? "TG" : "", dstId);
		sendCommand(text);
	} else {
		char text[30U];

		::sprintf(text, "t2.txt=\"2 %s %u\"", type, srcId);
		sendCommand(text);

		::sprintf(text, "t3.txt=\"%s%u\"", group ? "TG" : "", dstId);
		sendCommand(text);
	}
}

void CNextion::clearDMR(unsigned int slotNo)
{
	if (slotNo == 1U) {
		sendCommand("t0.txt=\"1 Listening\"");
		sendCommand("t1.txt=\"\"");
	} else {
		sendCommand("t2.txt=\"2 Listening\"");
		sendCommand("t3.txt=\"\"");
	}
}

void CNextion::setFusion()
{
	sendCommand("page YSF");

	sendCommand("t0.txt=\"Listening\"");
}

void CNextion::writeFusion(const char* source, const char* dest)
{
	assert(source != NULL);
	assert(dest != NULL);

	char text[30U];
	::sprintf(text, "t0.txt=\"%.10s\"", source);
	sendCommand(text);

	::sprintf(text, "t1.txt=\"%.10s\"", dest);
	sendCommand(text);
}

void CNextion::clearFusion()
{
	sendCommand("t0.txt=\"Listening\"");
	sendCommand("t1.txt=\"\"");
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
