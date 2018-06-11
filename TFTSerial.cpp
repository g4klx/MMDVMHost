/*
 *   Copyright (C) 2015,2016,2018 by Jonathan Naylor G4KLX
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

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned char ROTATION_PORTRAIT_LEFT  = 0U;
const unsigned char ROTATION_LANDSCAPE_UD   = 1U;
const unsigned char ROTATION_PORTRAIT_RIGHT = 2U;
const unsigned char ROTATION_LANDSCAPE      = 3U;

const unsigned char COLOUR_BLACK   = 0U;
const unsigned char COLOUR_BLUE    = 1U;
const unsigned char COLOUR_RED     = 2U;
const unsigned char COLOUR_GREEN   = 3U;
const unsigned char COLOUR_CYAN    = 4U;
const unsigned char COLOUR_MAGENTA = 5U;
const unsigned char COLOUR_YELLOW  = 6U;
const unsigned char COLOUR_WHITE   = 7U;

const unsigned char FONT_SMALL  = 1U;
const unsigned char FONT_MEDIUM = 2U;
const unsigned char FONT_LARGE  = 3U;

// x = 0 to 159, y = 0 to 127 - Landscape
// x = 0 to 127, y = 0 to 159 - Portrait

CTFTSerial::CTFTSerial(const std::string& callsign, unsigned int dmrid, ISerialPort* serial, unsigned int brightness) :
CDisplay(),
m_callsign(callsign),
m_dmrid(dmrid),
m_serial(serial),
m_brightness(brightness),
m_mode(MODE_IDLE)
{
	assert(serial != NULL);
	assert(brightness >= 0U && brightness <= 100U);
}

CTFTSerial::~CTFTSerial()
{
}

bool CTFTSerial::open()
{
	bool ret = m_serial->open();
	if (!ret) {
		LogError("Cannot open the port for the TFT Serial");
		delete m_serial;
		return false;
	}

	setRotation(ROTATION_LANDSCAPE);

	setBrightness(m_brightness);

	setBackground(COLOUR_WHITE);

	setForeground(COLOUR_BLACK);

	setIdle();

	return true;
}

void CTFTSerial::setIdleInt()
{
	// Clear the screen
	clearScreen();

	setFontSize(FONT_LARGE);

	// Draw MMDVM logo
	displayBitmap(0U, 0U, "MMDVM_sm.bmp");

	char text[30];
	::sprintf(text, "%-6s / %u", m_callsign.c_str(), m_dmrid);

	gotoPosPixel(18U, 55U);
	displayText(text);

	gotoPosPixel(45U, 90U);
	displayText("IDLE");

	m_mode = MODE_IDLE;
}

void CTFTSerial::setErrorInt(const char* text)
{
	assert(text != NULL);

	// Clear the screen
	clearScreen();

	setFontSize(FONT_MEDIUM);

	// Draw MMDVM logo
	displayBitmap(0U, 0U, "MMDVM_sm.bmp");

	setForeground(COLOUR_RED);

	gotoPosPixel(18U, 55U);
	displayText(text);

	gotoPosPixel(18U, 90U);
	displayText("ERROR");

	setForeground(COLOUR_BLACK);

	m_mode = MODE_ERROR;
}

void CTFTSerial::setLockoutInt()
{
	// Clear the screen
	clearScreen();

	setFontSize(FONT_LARGE);

	// Draw MMDVM logo
	displayBitmap(0U, 0U, "MMDVM_sm.bmp");

	gotoPosPixel(20U, 60U);
	displayText("LOCKOUT");

	m_mode = MODE_LOCKOUT;
}

void CTFTSerial::writeDStarInt(const char* my1, const char* my2, const char* your, const char* type, const char* reflector)
{
	assert(my1 != NULL);
	assert(my2 != NULL);
	assert(your != NULL);
	assert(type != NULL);
	assert(reflector != NULL);

	if (m_mode != MODE_DSTAR) {
		// Clear the screen
		clearScreen();

		setFontSize(FONT_MEDIUM);

		// Draw D-Star insignia
		displayBitmap(0U, 0U, "DStar_sm.bmp");
	}

	char text[30U];

	::sprintf(text, "%s %.8s/%4.4s", type, my1, my2);
	gotoPosPixel(5U, 70U);
	displayText(text);

	::sprintf(text, "%.8s", your);
	gotoPosPixel(5U, 90U);
	displayText(text);

	if (::strcmp(reflector, "        ") != 0) {
		::sprintf(text, "via %.8s", reflector);
		gotoPosPixel(5U, 110U);
		displayText(text);
	} else {
		gotoPosPixel(5U, 110U);
		displayText("            ");
	}

	m_mode = MODE_DSTAR;
}

void CTFTSerial::clearDStarInt()
{
	gotoPosPixel(5U, 70U);
	displayText("  Listening  ");

	gotoPosPixel(5U, 90U);
	displayText("         ");

	gotoPosPixel(5U, 110U);
	displayText("             ");
}

void CTFTSerial::writeDMRInt(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type)
{
	assert(type != NULL);

	if (m_mode != MODE_DMR) {
		// Clear the screen
		clearScreen();

		setFontSize(FONT_MEDIUM);

		// Draw DMR insignia
		displayBitmap(0U, 0U, "DMR_sm.bmp");

		if (slotNo == 1U) {
			gotoPosPixel(5U, 90U);
			displayText("2 Listening");
		} else {
			gotoPosPixel(5U, 55U);
			displayText("1 Listening");
		}
	}

	if (slotNo == 1U) {
		char text[30U];

		::sprintf(text, "1 %s %s", type, src.c_str());
		gotoPosPixel(5U, 55U);
		displayText(text);

		::sprintf(text, "%s%s", group ? "TG" : "", dst.c_str());
		gotoPosPixel(65U, 72U);
		displayText(text);
	} else {
		char text[30U];

		::sprintf(text, "2 %s %s", type, src.c_str());
		gotoPosPixel(5U, 90U);
		displayText(text);

		::sprintf(text, "%s%s", group ? "TG" : "", dst.c_str());
		gotoPosPixel(65U, 107U);
		displayText(text);
	}

	m_mode = MODE_DMR;
}

void CTFTSerial::clearDMRInt(unsigned int slotNo)
{
	if (slotNo == 1U) {
		gotoPosPixel(5U, 55U);
		displayText("1 Listening ");

		gotoPosPixel(65U, 72U);
		displayText("       ");
	} else {
		gotoPosPixel(5U, 90U);
		displayText("2 Listening ");

		gotoPosPixel(65U, 107U);
		displayText("       ");
	}
}

void CTFTSerial::writeFusionInt(const char* source, const char* dest, const char* type, const char* origin)
{
	assert(source != NULL);
	assert(dest != NULL);
	assert(type != NULL);
	assert(origin != NULL);

	if (m_mode != MODE_YSF) {
		// Clear the screen
		clearScreen();

		setFontSize(FONT_MEDIUM);

		// Draw the System Fusion insignia
		displayBitmap(0U, 0U, "YSF_sm.bmp");
	}

	char text[30U];
	::sprintf(text, "%s %.10s", type, source);

	gotoPosPixel(5U, 70U);
	displayText(text);

	::sprintf(text, "  %.10s", dest);

	gotoPosPixel(5U, 90U);
	displayText(text);

	if (::strcmp(origin, "          ") != 0) {
		::sprintf(text, "at %.10s", origin);
		gotoPosPixel(5U, 110U);
		displayText(text);
	} else {
		gotoPosPixel(5U, 110U);
		displayText("              ");
	}

	m_mode = MODE_YSF;
}

void CTFTSerial::clearFusionInt()
{
	gotoPosPixel(5U, 70U);
	displayText("  Listening   ");

	gotoPosPixel(5U, 90U);
	displayText("              ");

	gotoPosPixel(5U, 110U);
	displayText("              ");
}

void CTFTSerial::writeP25Int(const char* source, bool group, unsigned int dest, const char* type)
{
	assert(source != NULL);
	assert(type != NULL);

	if (m_mode != MODE_P25) {
		// Clear the screen
		clearScreen();

		setFontSize(FONT_MEDIUM);

		// Draw the P25 insignia
		displayBitmap(0U, 0U, "P25_sm.bmp");
	}

	char text[30U];
	::sprintf(text, "%s %.10s", type, source);

	gotoPosPixel(5U, 70U);
	displayText(text);

	::sprintf(text, "  %s%u", group ? "TG" : "", dest);

	gotoPosPixel(5U, 90U);
	displayText(text);

	m_mode = MODE_P25;
}

void CTFTSerial::clearP25Int()
{
	gotoPosPixel(5U, 70U);
	displayText("  Listening   ");

	gotoPosPixel(5U, 90U);
	displayText("              ");

	gotoPosPixel(5U, 110U);
	displayText("              ");
}

void CTFTSerial::writeNXDNInt(const char* source, bool group, unsigned int dest, const char* type)
{
	assert(source != NULL);
	assert(type != NULL);

	if (m_mode != MODE_NXDN) {
		// Clear the screen
		clearScreen();

		setFontSize(FONT_MEDIUM);

		// Draw the P25 insignia
		displayBitmap(0U, 0U, "NXDN_sm.bmp");
	}

	char text[30U];
	::sprintf(text, "%s %.10s", type, source);

	gotoPosPixel(5U, 70U);
	displayText(text);

	::sprintf(text, "  %s%u", group ? "TG" : "", dest);

	gotoPosPixel(5U, 90U);
	displayText(text);

	m_mode = MODE_NXDN;
}

void CTFTSerial::clearNXDNInt()
{
	gotoPosPixel(5U, 70U);
	displayText("  Listening   ");

	gotoPosPixel(5U, 90U);
	displayText("              ");

	gotoPosPixel(5U, 110U);
	displayText("              ");
}

void CTFTSerial::writePOCSAGInt(uint32_t ric, const std::string& message)
{
	gotoPosPixel(15U, 90U);
	displayText("POCSAG TX");

	m_mode = MODE_CW;
}

void CTFTSerial::clearPOCSAGInt()
{
	gotoPosPixel(45U, 90U);
	displayText("IDLE");
}

void CTFTSerial::writeCWInt()
{
	gotoPosPixel(45U, 90U);
	displayText("CW TX");

	m_mode = MODE_CW;
}

void CTFTSerial::clearCWInt()
{
	gotoPosPixel(45U, 90U);
	displayText("IDLE");
}

void CTFTSerial::close()
{
	m_serial->close();
	delete m_serial;
}

void CTFTSerial::clearScreen()
{
	m_serial->write((unsigned char*)"\x1B\x00\xFF", 3U);
}

void CTFTSerial::setForeground(unsigned char colour)
{
	assert(colour >= 0U && colour <= 7U);

	m_serial->write((unsigned char*)"\x1B\x01", 2U);
	m_serial->write(&colour, 1U);
	m_serial->write((unsigned char*)"\xFF", 1U);
}

void CTFTSerial::setBackground(unsigned char colour)
{
	assert(colour >= 0U && colour <= 7U);

	m_serial->write((unsigned char*)"\x1B\x02", 2U);
	m_serial->write(&colour, 1U);
	m_serial->write((unsigned char*)"\xFF", 1U);
}

void CTFTSerial::setRotation(unsigned char rotation)
{
	assert(rotation >= 0U && rotation <= 3U);

	m_serial->write((unsigned char*)"\x1B\x03", 2U);
	m_serial->write(&rotation, 1U);
	m_serial->write((unsigned char*)"\xFF", 1U);
}

void CTFTSerial::setFontSize(unsigned char size)
{
	assert(size >= 1U && size <= 3U);

	m_serial->write((unsigned char*)"\x1B\x04", 2U);
	m_serial->write(&size, 1U);
	m_serial->write((unsigned char*)"\xFF", 1U);
}

void CTFTSerial::gotoBegOfLine()
{
	m_serial->write((unsigned char*)"\x1B\x05\xFF", 3U);
}

void CTFTSerial::gotoPosText(unsigned char x, unsigned char y)
{
	m_serial->write((unsigned char*)"\x1B\x06", 2U);
	m_serial->write(&x, 1U);
	m_serial->write(&y, 1U);
	m_serial->write((unsigned char*)"\xFF", 1U);
}

void CTFTSerial::gotoPosPixel(unsigned char x, unsigned char y)
{
	m_serial->write((unsigned char*)"\x1B\x07", 2U);
	m_serial->write(&x, 1U);
	m_serial->write(&y, 1U);
	m_serial->write((unsigned char*)"\xFF", 1U);
}

void CTFTSerial::drawLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2)
{
	m_serial->write((unsigned char*)"\x1B\x08", 2U);
	m_serial->write(&x1, 1U);
	m_serial->write(&y1, 1U);
	m_serial->write(&x2, 1U);
	m_serial->write(&y2, 1U);
	m_serial->write((unsigned char*)"\xFF", 1U);
}

void CTFTSerial::drawBox(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, bool filled)
{
	if (filled)
		m_serial->write((unsigned char*)"\x1B\x0A", 2U);
	else
		m_serial->write((unsigned char*)"\x1B\x09", 2U);

	m_serial->write(&x1, 1U);
	m_serial->write(&y1, 1U);
	m_serial->write(&x2, 1U);
	m_serial->write(&y2, 1U);
	m_serial->write((unsigned char*)"\xFF", 1U);
}

void CTFTSerial::drawCircle(unsigned char x, unsigned char y, unsigned char radius, bool filled)
{
	if (filled)
		m_serial->write((unsigned char*)"\x1B\x0C", 2U);
	else
		m_serial->write((unsigned char*)"\x1B\x0B", 2U);

	m_serial->write(&x, 1U);
	m_serial->write(&y, 1U);
	m_serial->write(&radius, 1U);
	m_serial->write((unsigned char*)"\xFF", 1U);
}

void CTFTSerial::displayBitmap(unsigned char x, unsigned char y, const char* filename)
{
	assert(filename != NULL);

	m_serial->write((unsigned char*)"\x1B\x0D", 2U);
	m_serial->write(&x, 1U);
	m_serial->write(&y, 1U);
	m_serial->write((unsigned char*)filename, (unsigned int)::strlen(filename));
	m_serial->write((unsigned char*)"\xFF", 1U);
}

void CTFTSerial::setBrightness(unsigned char brightness)
{
	assert(brightness >= 0U && brightness <= 100U);

	m_serial->write((unsigned char*)"\x1B\x0E", 2U);
	m_serial->write(&brightness, 1U);
	m_serial->write((unsigned char*)"\xFF", 1U);
}

void CTFTSerial::displayText(const char* text)
{
	assert(text != NULL);

	m_serial->write((unsigned char*)text, (unsigned int)::strlen(text));
}
