/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
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

#include "TFTSerial.h"
#include "Log.h"

#include <cstring>

CTFTSerial::CTFTSerial(const std::string& port) :
m_serial(port, SERIAL_9600)
{
}

CTFTSerial::~CTFTSerial()
{
}

bool CTFTSerial::open()
{
	bool ret = m_serial.open();
	if (!ret) {
		LogError("Cannot open the port for the TFT Serial");
		return false;
	}

	// Set background white
	m_serial.write((unsigned char*)"\x1B\x02\x07\xFF", 4U);

	// Set foreground black
	m_serial.write((unsigned char*)"\x1B\x01\x00\xFF", 4U);

	setIdle();

	return true;
}

void CTFTSerial::setIdle()
{
	// Clear the screen
	clearScreen();

	// Draw MMDVM logo
	writeImage(0U, 0U, "MMDVM_sm.bmp");

	// Draw all mode insignias
	writeImage(0U, 30U, "ALL_sm.bmp");
}

void CTFTSerial::setDStar()
{
	// Clear the screen
	clearScreen();

	// Draw MMDVM logo
	writeImage(0U, 0U, "MMDVM_sm.bmp");

	// Draw D-Star insignia
	writeImage(0U, 30U, "DStar_sm.bmp");

	writeText(0U, 8U, "Listening");
}

void CTFTSerial::writeDStar(const std::string& call1, const std::string& call2)
{
	char text[20U];
	::sprintf(text, "%s/%s", call1.c_str(), call2.c_str());

	writeText(0U, 8U, text);
}

void CTFTSerial::clearDStar()
{
	// Draw MMDVM logo
	writeImage(0U, 0U, "MMDVM_sm.bmp");

	// Draw D-Star insignia
	writeImage(0U, 30U, "DStar_sm.bmp");

	writeText(0U, 8U, "Listening");
}

void CTFTSerial::setDMR()
{
	// Clear the screen
	clearScreen();

	// Draw MMDVM logo
	writeImage(0U, 0U, "MMDVM_sm.bmp");

	// Draw DMR insignia
	writeImage(0U, 30U, "DMR_sm.bmp");

	writeText(0U, 8U, "1: Listening");
	writeText(0U, 9U, "2: Listening");
}

void CTFTSerial::writeDMR(unsigned int slotNo, unsigned int srcId, bool group, unsigned int dstId)
{
	char text[20U];
	::sprintf(text, "%u: %u %s%u", slotNo, srcId, group ? "TG " : "", dstId);

	if (slotNo == 1U)
		writeText(0U, 8U, text);
	else
		writeText(0U, 9U, text);
}

void CTFTSerial::clearDMR(unsigned int slotNo)
{
	if (slotNo == 1U)
		writeText(0U, 8U, "1: Listening");
	else
		writeText(0U, 9U, "2: Listening");
}

void CTFTSerial::setFusion()
{
	// Clear the screen
	clearScreen();

	// Draw MMDVM logo
	writeImage(0U, 0U, "MMDVM_sm.bmp");

	// Draw the System Fusion insignia
	writeImage(0U, 30U, "YSF_sm.bmp");

	writeText(0U, 8U, "Listening");
}

void CTFTSerial::writeFusion(const std::string& callsign)
{
	char text[20U];
	::sprintf(text, "%s", callsign.c_str());

	writeText(0U, 8U, text);
}

void CTFTSerial::clearFusion()
{
	// Draw MMDVM logo
	writeImage(0U, 0U, "MMDVM_sm.bmp");

	// Draw the System Fusion insignia
	writeImage(0U, 30U, "YSF_sm.bmp");

	writeText(0U, 8U, "Listening");
}

void CTFTSerial::close()
{
	m_serial.close();
}

void CTFTSerial::clearScreen()
{
	m_serial.write((unsigned char*)"\x1B\x00\xFF", 3U);
}

void CTFTSerial::writeText(unsigned char x, unsigned char y, const char* text)
{
	m_serial.write((unsigned char*)"\x1B\x06", 2U);
	m_serial.write(&x, 1U);
	m_serial.write(&y, 1U);
	m_serial.write((unsigned char*)"\xFF", 1U);
	m_serial.write((unsigned char*)text, ::strlen(text));
}

void CTFTSerial::writeImage(unsigned char x, unsigned char y, const char* filename)
{
	m_serial.write((unsigned char*)"\x1B\x0D", 2U);
	m_serial.write(&x, 1U);
	m_serial.write(&y, 1U);
	m_serial.write((unsigned char*)filename, ::strlen(filename));
	m_serial.write((unsigned char*)"\xFF", 1U);
}
