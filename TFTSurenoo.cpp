/*
 *   Copyright (C) 2019 by SASANO Takayoshi JG1UAA
 *   Copyright (C) 2015,2016,2018,2019,2020 by Jonathan Naylor G4KLX
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
#include "TFTSurenoo.h"
#include "Thread.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

/*
 * UART-TFT LCD Driver for Surenoo JC22-V05 (128x160)
 * other Surenoo UART-LCD will be work, but display area is still 160x128
 * (tested with JC028-V03 240x320 module)
 */

// x = 0 to 159, y = 0 to 127 - Landscape
// x = 0 to 127, y = 0 to 159 - Portrait
#define X_WIDTH			160
#define Y_WIDTH			128

#define ROTATION_PORTRAIT	0
#define ROTATION_LANDSCAPE	1

enum LcdColour {
	COLOUR_BLACK, COLOUR_RED, COLOUR_GREEN, COLOUR_BLUE,
	COLOUR_YELLOW, COLOUR_CYAN, COLOUR_MAGENTA, COLOUR_GREY,
	COLOUR_DARK_GREY, COLOUR_DARK_RED, COLOUR_DARK_GREEN, COLOUR_DARK_BLUE,
	COLOUR_DARK_YELLOW, COLOUR_DARK_CYAN, COLOUR_DARK_MAGENTA, COLOUR_WHITE
};

#define FONT_SMALL		16	//  8x16
#define FONT_MEDIUM		24	// 12x24
#define FONT_LARGE		32	// 16x32

#define INFO_COLOUR		COLOUR_CYAN
#define EXT_COLOUR		COLOUR_DARK_GREEN
#define BG_COLOUR		COLOUR_BLACK
#define ERROR_COLOUR		COLOUR_DARK_RED
#define MODE_COLOUR   		COLOUR_YELLOW

// MODE_FONT_SIZE should be equal or larger than STATUS_FONT_SIZE
#define MODE_FONT_SIZE		FONT_MEDIUM
#define STATUS_FONT_SIZE	FONT_SMALL

#define STATUS_MARGIN		32	// pixel

#define MODE_CHARS		(X_WIDTH / (MODE_FONT_SIZE / 2))
#define STATUS_CHARS		(X_WIDTH / (STATUS_FONT_SIZE / 2))
#define STATUS_LINES		((Y_WIDTH - STATUS_MARGIN) / STATUS_FONT_SIZE)
#define statusLineOffset(x)	((STATUS_CHARS + 1) * ((x) + 1))
#define statusLineNo(x)		(x)
#define INFO_LINES		statusLineNo(2)

// This module sometimes ignores display command (too busy?),
// so supress display refresh
#define REFRESH_PERIOD		600	// msec

#define STR_CRLF		"\x0D\x0A"
#define STR_DMR			"DMR"
#define STR_DSTAR		"D-STAR"
#define STR_MMDVM		"MMDVM"
#define STR_NXDN		"NXDN"
#define STR_P25			"P25"
#define STR_YSF			"SystemFusion"

CTFTSurenoo::CTFTSurenoo(const std::string& callsign, unsigned int dmrid, ISerialPort* serial, unsigned int brightness, bool duplex) :
CDisplay(),
m_callsign(callsign),
m_dmrid(dmrid),
m_serial(serial),
m_brightness(brightness),
m_mode(MODE_IDLE),
m_duplex(duplex),
//m_duplex(true),                      // uncomment to force duplex display for testing!
m_refresh(false),
m_refreshTimer(1000U, 0U, REFRESH_PERIOD),
m_lineBuf(NULL)
{
	assert(serial != NULL);
	assert(brightness >= 0U && brightness <= 255U);
}

CTFTSurenoo::~CTFTSurenoo()
{
}

bool CTFTSurenoo::open()
{
	bool ret = m_serial->open();
	if (!ret) {
		LogError("Cannot open the port for the TFT Serial");
		delete m_serial;
		return false;
	}

	m_lineBuf = new char[statusLineOffset(STATUS_LINES)];
	if (m_lineBuf == NULL) {
		LogError("Cannot allocate line buffer");
		m_serial->close();
		delete m_serial;
		return false;
	}

	lcdReset();
	clearScreen(BG_COLOUR);

	setRotation(ROTATION_LANDSCAPE);
	setBrightness(m_brightness);
	setBackground(BG_COLOUR);
	setIdle();

	m_refreshTimer.start();

	return true;
}

void CTFTSurenoo::setIdleInt()
{
	setModeLine(STR_MMDVM);

	::snprintf(m_temp, sizeof(m_temp), "%s / %u", m_callsign.c_str(), m_dmrid);
	setStatusLine(statusLineNo(0), m_temp);
	setStatusLine(statusLineNo(1), "IDLE");

	m_mode = MODE_IDLE;
}

void CTFTSurenoo::setErrorInt(const char* text)
{
	assert(text != NULL);

	setModeLine(STR_MMDVM);
	setStatusLine(statusLineNo(0), text);
	setStatusLine(statusLineNo(1), "ERROR");

	m_mode = MODE_ERROR;
}

void CTFTSurenoo::setLockoutInt()
{
	setModeLine(STR_MMDVM);
	setStatusLine(statusLineNo(1), "LOCKOUT");

	m_mode = MODE_LOCKOUT;
}

void CTFTSurenoo::setQuitInt()
{
	// m_refreshTimer has stopped, need delay here
	CThread::sleep(REFRESH_PERIOD);

	setModeLine(STR_MMDVM);
	setStatusLine(statusLineNo(1), "STOPPED");

	refreshDisplay();

	m_mode = MODE_QUIT;
}

void CTFTSurenoo::setFMInt()
{
	setModeLine(STR_MMDVM);
	setStatusLine(statusLineNo(1), "FM");

	m_mode = MODE_FM;
}

void CTFTSurenoo::writeDStarInt(const char* my1, const char* my2, const char* your, const char* type, const char* reflector)
{
	assert(my1 != NULL);
	assert(my2 != NULL);
	assert(your != NULL);
	assert(type != NULL);
	assert(reflector != NULL);

	setModeLine(STR_MMDVM);

	::snprintf(m_temp, sizeof(m_temp), "%s %s/%s", type, my1, my2);
	setStatusLine(statusLineNo(0), m_temp);

	::snprintf(m_temp, sizeof(m_temp), "%s", your);
	setStatusLine(statusLineNo(1), m_temp);

	if (::strcmp(reflector, "        ") != 0)
		::snprintf(m_temp, sizeof(m_temp), "via %s", reflector);
	else
		::strcpy(m_temp, "");
	setStatusLine(statusLineNo(2), m_temp);

	m_mode = MODE_DSTAR;
}

void CTFTSurenoo::clearDStarInt()
{
	setStatusLine(statusLineNo(0), "Listening");
	for (int i = 1; i < STATUS_LINES; i++)
		setStatusLine(statusLineNo(i), "");
}

void CTFTSurenoo::writeDMRInt(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type)
{
	assert(type != NULL);

	if (m_mode != MODE_DMR) {
		setModeLine(STR_DMR);
		if (m_duplex) {
			setStatusLine(statusLineNo(0), "Listening");
			setStatusLine(statusLineNo(1), "TS1");
			setStatusLine(statusLineNo(2), "Listening");
			setStatusLine(statusLineNo(3), "TS2");
		}
	}		

	int pos = m_duplex ? (slotNo - 1) : 0;
	::snprintf(m_temp, sizeof(m_temp), "%s %s", type, src.c_str());
	setStatusLine(statusLineNo(pos * 2), m_temp);

	::snprintf(m_temp, sizeof(m_temp), "TS%d %s%s", slotNo, group ? "TG" : "", dst.c_str());
	setStatusLine(statusLineNo(pos * 2 + 1), m_temp);

	m_mode = MODE_DMR;
}

int CTFTSurenoo::writeDMRIntEx(unsigned int slotNo, const class CUserDBentry& src, bool group, const std::string& dst, const char* type)
{
	assert(type != NULL);

	// duplex mode is not supported
	if (m_duplex)
		return -1;

	setModeLine(STR_DMR);
	setStatusLine(statusLineNo(2), (src.get(keyFIRST_NAME) + " " + src.get(keyLAST_NAME)).c_str());
	setStatusLine(statusLineNo(3), src.get(keyCITY).c_str());
	setStatusLine(statusLineNo(4), src.get(keySTATE).c_str());
	setStatusLine(statusLineNo(5), src.get(keyCOUNTRY).c_str());

	m_mode = MODE_DMR;

	return 1;
}

void CTFTSurenoo::clearDMRInt(unsigned int slotNo)
{
	int pos = m_duplex ? (slotNo - 1) : 0;
	setStatusLine(statusLineNo(pos * 2), "Listening");

	if (m_duplex) {
		::snprintf(m_temp, sizeof(m_temp), "TS%d", slotNo);
		setStatusLine(statusLineNo(pos * 2 + 1), m_temp);
	} else {
		for (int i = 1; i < STATUS_LINES; i++)
			setStatusLine(statusLineNo(i), "");
	}
}

void CTFTSurenoo::writeFusionInt(const char* source, const char* dest, unsigned char dgid, const char* type, const char* origin)
{
	assert(source != NULL);
	assert(dest != NULL);
	assert(type != NULL);
	assert(origin != NULL);

	setModeLine(STR_YSF);

	::snprintf(m_temp, sizeof(m_temp), "%s %s", type, source);
	setStatusLine(statusLineNo(0), m_temp);

	::snprintf(m_temp, sizeof(m_temp), "DG-ID %u", dgid);
	setStatusLine(statusLineNo(1), m_temp);

	if (::strcmp(origin, "          ") != 0)
		::snprintf(m_temp, sizeof(m_temp), "at %s", origin);
	else
		::strcpy(m_temp, "");
	setStatusLine(statusLineNo(2), m_temp);

	m_mode = MODE_YSF;
}

void CTFTSurenoo::clearFusionInt()
{
	clearDStarInt();
}

void CTFTSurenoo::writeP25Int(const char* source, bool group, unsigned int dest, const char* type)
{
	assert(source != NULL);
	assert(type != NULL);

	setModeLine(STR_P25);

	::snprintf(m_temp, sizeof(m_temp), "%s %s", type, source);
	setStatusLine(statusLineNo(0), m_temp);

	::snprintf(m_temp, sizeof(m_temp), "%s%u", group ? "TG" : "", dest);
	setStatusLine(statusLineNo(1), m_temp);

	m_mode = MODE_P25;
}

void CTFTSurenoo::clearP25Int()
{
	clearDStarInt();
}

void CTFTSurenoo::writeNXDNInt(const char* source, bool group, unsigned int dest, const char* type)
{
	assert(source != NULL);
	assert(type != NULL);

	if (m_mode != MODE_NXDN)
		setModeLine(STR_NXDN);

	::snprintf(m_temp, sizeof(m_temp), "%s %s", type, source);
	setStatusLine(statusLineNo(0), m_temp);

	::snprintf(m_temp, sizeof(m_temp), "%s%u", group ? "TG" : "", dest);
	setStatusLine(statusLineNo(1), m_temp);

	m_mode = MODE_NXDN;
}

int CTFTSurenoo::writeNXDNIntEx(const class CUserDBentry& source, bool group, unsigned int dest, const char* type)
{
	assert(type != NULL);

	setModeLine(STR_NXDN);
	setStatusLine(statusLineNo(2), (source.get(keyFIRST_NAME) + " " + source.get(keyLAST_NAME)).c_str());
	setStatusLine(statusLineNo(3), source.get(keyCITY).c_str());
	setStatusLine(statusLineNo(4), source.get(keySTATE).c_str());
	setStatusLine(statusLineNo(5), source.get(keyCOUNTRY).c_str());

	m_mode = MODE_NXDN;

	return 1;
}

void CTFTSurenoo::clearNXDNInt()
{
	clearDStarInt();
}

void CTFTSurenoo::writePOCSAGInt(uint32_t ric, const std::string& message)
{
	setStatusLine(statusLineNo(1), "POCSAG TX");

	m_mode = MODE_POCSAG;
}

void CTFTSurenoo::clearPOCSAGInt()
{
	setStatusLine(statusLineNo(1), "IDLE");
}

void CTFTSurenoo::writeCWInt()
{
	setStatusLine(statusLineNo(1), "CW TX");

	m_mode = MODE_CW;
}

void CTFTSurenoo::clearCWInt()
{
	setStatusLine(statusLineNo(1), "IDLE");
}

void CTFTSurenoo::close()
{
	delete[] m_lineBuf;

	m_serial->close();
	delete m_serial;
}

void CTFTSurenoo::clockInt(unsigned int ms)
{
	m_refreshTimer.clock(ms);	// renew timer status 

	if (m_refreshTimer.isRunning() && m_refreshTimer.hasExpired()) {
		refreshDisplay();
		m_refreshTimer.start();	// reset timer, wait for next period
	}
}

void CTFTSurenoo::setLineBuffer(char *buf, const char *text, int maxchar)
{
	int i;

	for (i = 0; i < maxchar && text[i] != '\0'; i++)
		buf[i] = text[i];
	buf[i] = '\0';

	m_refresh = true;
}

void CTFTSurenoo::setModeLine(const char *text)
{
	setLineBuffer(m_lineBuf, text, MODE_CHARS);

	// clear all status line
	for (int i = 0; i < STATUS_LINES; i++) setStatusLine(i, "");
}

void CTFTSurenoo::setStatusLine(unsigned int line, const char *text)
{
	setLineBuffer(m_lineBuf + statusLineOffset(line), text, STATUS_CHARS);
}  

void CTFTSurenoo::refreshDisplay(void)
{
	if (!m_refresh) return;

	// send CR+LF to avoid first command is not processed
	::snprintf(m_temp, sizeof(m_temp), STR_CRLF);
	m_serial->write((unsigned char*)m_temp, (unsigned int)::strlen(m_temp));

	// clear display
	::snprintf(m_temp, sizeof(m_temp), "BOXF(%d,%d,%d,%d,%d);",
		   0, 0, X_WIDTH - 1, Y_WIDTH - 1, BG_COLOUR);
	m_serial->write((unsigned char*)m_temp, ::strlen(m_temp));

	// mode line
	::snprintf(m_temp, sizeof(m_temp), "DCV%d(%d,%d,'%s',%d);",
		   MODE_FONT_SIZE, 0, 0, m_lineBuf, MODE_COLOUR);
	m_serial->write((unsigned char*)m_temp, (unsigned int)::strlen(m_temp));

	// status line
	for (int i = 0; i < STATUS_LINES; i++) {
		char *p = m_lineBuf + statusLineOffset(i);
		if (!::strlen(p)) continue;

		::snprintf(m_temp, sizeof(m_temp), "DCV%d(%d,%d,'%s',%d);",
			   STATUS_FONT_SIZE, 0,
			   STATUS_MARGIN + STATUS_FONT_SIZE * i, p,
			   (!m_duplex && i >= INFO_LINES) ? EXT_COLOUR : INFO_COLOUR);
		m_serial->write((unsigned char*)m_temp, (unsigned int)::strlen(m_temp));
	}

	// sending CR+LF finishes commands
	::snprintf(m_temp, sizeof(m_temp), STR_CRLF);
	m_serial->write((unsigned char*)m_temp, (unsigned int)::strlen(m_temp));

	m_refresh = false;
}

void CTFTSurenoo::lcdReset(void)
{
	::snprintf(m_temp, sizeof(m_temp), "RESET;" STR_CRLF);
	m_serial->write((unsigned char*)m_temp, (unsigned int)::strlen(m_temp));
	CThread::sleep(250);	// document says 230ms
}

void CTFTSurenoo::clearScreen(unsigned char colour)
{
	assert(colour >= 0U && colour <= 63U);

	::snprintf(m_temp, sizeof(m_temp), "CLR(%d);" STR_CRLF, colour);
	m_serial->write((unsigned char*)m_temp, (unsigned int)::strlen(m_temp));
	CThread::sleep(100);	// at least 60ms (@240x320 panel)
}

void CTFTSurenoo::setBackground(unsigned char colour)
{
	assert(colour >= 0U && colour <= 63U);

	::snprintf(m_temp, sizeof(m_temp), "SBC(%d);", colour);
	m_serial->write((unsigned char*)m_temp, (unsigned int)::strlen(m_temp));
}

void CTFTSurenoo::setRotation(unsigned char rotation)
{
	assert(rotation >= 0U && rotation <= 1U);

	::snprintf(m_temp, sizeof(m_temp), "DIR(%d);", rotation);
	m_serial->write((unsigned char*)m_temp, (unsigned int)::strlen(m_temp));
}

void CTFTSurenoo::setBrightness(unsigned char brightness)
{
	assert(brightness >= 0U && brightness <= 255U);

	::snprintf(m_temp, sizeof(m_temp), "BL(%d);", brightness);
	m_serial->write((unsigned char*)m_temp, (unsigned int)::strlen(m_temp));
}
