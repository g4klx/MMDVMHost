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
	m_serial.write((unsigned char*)"\x1B\x00\xFF", 3U);

	// Draw MMDVM logo
	m_serial.write((unsigned char*)"\x1B\x0D\x00\x00MMDVM_sm.bmp\xFF", 15U);

	// Draw all mode insignias
	m_serial.write((unsigned char*)"\x1B\x0D\x00\x00ALL_sm.bmp\xFF", 15U);
}

void CTFTSerial::setDStar()
{
	// Clear the screen
	m_serial.write((unsigned char*)"\x1B\x00\xFF", 3U);

	// Draw MMDVM logo
	m_serial.write((unsigned char*)"\x1B\x0D\x00\x00MMDVM_sm.bmp\xFF", 15U);

	// Draw D-Star insignia
	m_serial.write((unsigned char*)"\x1B\x0D\x00\x00DStar_sm.bmp\xFF", 17U);

	m_serial.write((unsigned char*)"\x1B\x06\x00\x08\xFF", 5U);
	m_serial.write((unsigned char*)"Listening", 9U);
}

void CTFTSerial::writeDStar(const std::string& call1, const std::string& call2)
{
	m_serial.write((unsigned char*)"\x1B\x06\x00\x08\xFF", 5U);

	char text[20U];
	::sprintf(text, "%s/%s", call1.c_str(), call2.c_str());

	m_serial.write((unsigned char*)text, ::strlen(text));
}

void CTFTSerial::clearDStar()
{
	// Draw MMDVM logo
	m_serial.write((unsigned char*)"\x1B\x0D\x00\x00MMDVM_sm.bmp\xFF", 15U);

	// Draw D-Star insignia
	m_serial.write((unsigned char*)"\x1B\x0D\x00\x00DStar_sm.bmp\xFF", 17U);

	m_serial.write((unsigned char*)"\x1B\x06\x00\x08\xFF", 5U);
	m_serial.write((unsigned char*)"Listening", 9U);
}

void CTFTSerial::setDMR()
{
	// Clear the screen
	m_serial.write((unsigned char*)"\x1B\x00\xFF", 3U);

	// Draw MMDVM logo
	m_serial.write((unsigned char*)"\x1B\x0D\x00\x00MMDVM_sm.bmp\xFF", 15U);

	// Draw DMR insignia
	m_serial.write((unsigned char*)"\x1B\x0D\x00\x00DMR_sm.bmp\xFF", 15U);

	m_serial.write((unsigned char*)"\x1B\x06\x00\x08\xFF", 5U);
	m_serial.write((unsigned char*)"1: Listening", 9U);

	m_serial.write((unsigned char*)"\x1B\x06\x00\x09\xFF", 5U);
	m_serial.write((unsigned char*)"2: Listening", 9U);
}

void CTFTSerial::writeDMR(unsigned int slotNo, unsigned int srcId, bool group, unsigned int dstId)
{
	char text[20U];
	::sprintf(text, "%u: %u %s%u", slotNo, srcId, group ? "TG " : "", dstId);

	if (slotNo == 1U)
		m_serial.write((unsigned char*)"\x1B\x06\x00\x08\xFF", 5U);
	else
		m_serial.write((unsigned char*)"\x1B\x06\x00\x09\xFF", 5U);

	m_serial.write((unsigned char*)text, ::strlen(text));
}

void CTFTSerial::clearDMR(unsigned int slotNo)
{
	if (slotNo == 1U) {
		m_serial.write((unsigned char*)"\x1B\x06\x00\x08\xFF", 5U);
		m_serial.write((unsigned char*)"1: Listening", 11U);
	} else {
		m_serial.write((unsigned char*)"\x1B\x06\x00\x09\xFF", 5U);
		m_serial.write((unsigned char*)"2: Listening", 11U);
	}
}

void CTFTSerial::setFusion()
{
	// Clear the screen
	m_serial.write((unsigned char*)"\x1B\x00\xFF", 3U);

	// Draw MMDVM logo
	m_serial.write((unsigned char*)"\x1B\x0D\x00\x00MMDVM_sm.bmp\xFF", 15U);

	// Draw the System Fusion insignia
	m_serial.write((unsigned char*)"\x1B\x0D\x00\x00YSF_sm.bmp\xFF", 15U);

	m_serial.write((unsigned char*)"\x1B\x06\x00\x08\xFF", 5U);
	m_serial.write((unsigned char*)"Listening", 9U);
}

void CTFTSerial::writeFusion(const std::string& callsign)
{
	m_serial.write((unsigned char*)"\x1B\x06\x00\x08\xFF", 5U);

	char text[20U];
	::sprintf(text, "%s", callsign.c_str());

	m_serial.write((unsigned char*)text, ::strlen(text));
}

void CTFTSerial::clearFusion()
{
	// Draw MMDVM logo
	m_serial.write((unsigned char*)"\x1B\x0D\x00\x00MMDVM_sm.bmp\xFF", 15U);

	// Draw the System Fusion insignia
	m_serial.write((unsigned char*)"\x1B\x0D\x00\x00YSF_sm.bmp\xFF", 15U);

	m_serial.write((unsigned char*)"\x1B\x06\x00\x08\xFF", 5U);
	m_serial.write((unsigned char*)"Listening", 9U);
}

void CTFTSerial::close()
{
	m_serial.close();
}
