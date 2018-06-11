/*
 *   Copyright (C) 2016,2017,2018 by Jonathan Naylor G4KLX & Tony Corbett G0WFV
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

#include "HD44780.h"
#include "Log.h"

#include <wiringPi.h>
#include <softPwm.h>
#include <lcd.h>
#include <pthread.h>

#include <cstdio>
#include <cassert>
#include <cstring>

const char* LISTENING = "Listening                               ";
const char* DEADSPACE = "                                        ";

char        m_buffer1[128U];
char        m_buffer2[128U];
char        m_buffer3[128U];
char        m_buffer4[128U];

const unsigned int DSTAR_RSSI_COUNT = 3U;    // 3 * 420ms = 1260ms 
const unsigned int DMR_RSSI_COUNT   = 4U;    // 4 * 360ms = 1440ms 
const unsigned int YSF_RSSI_COUNT   = 13U;   // 13 * 100ms = 1300ms 
const unsigned int P25_RSSI_COUNT   = 7U;    // 7 * 180ms = 1260ms
const unsigned int NXDN_RSSI_COUNT  = 28U;   // 28 * 40ms = 1120ms

CHD44780::CHD44780(unsigned int rows, unsigned int cols, const std::string& callsign, unsigned int dmrid, const std::vector<unsigned int>& pins, unsigned int i2cAddress, bool pwm, unsigned int pwmPin, unsigned int pwmBright, unsigned int pwmDim, bool displayClock, bool utc, bool duplex) :
CDisplay(),
m_rows(rows),
m_cols(cols),
m_callsign(callsign),
m_dmrid(dmrid),
m_rb(pins.at(0U)),
m_strb(pins.at(1U)),
m_d0(pins.at(2U)),
m_d1(pins.at(3U)),
m_d2(pins.at(4U)),
m_d3(pins.at(5U)),
m_i2cAddress(i2cAddress),
m_pwm(pwm),
m_pwmPin(pwmPin),
m_pwmBright(pwmBright),
m_pwmDim(pwmDim),
m_displayClock(displayClock),
m_utc(utc),
m_duplex(duplex),
//m_duplex(true),                      // uncomment to force duplex display for testing!
m_fd(-1),
m_dmr(false),
m_clockDisplayTimer(1000U, 0U, 250U),   // Update the clock display every 250ms
m_rssiCount1(0U), 
m_rssiCount2(0U)
{
	assert(rows > 1U);
	assert(cols > 15U);
}

// Text-based custom character for "from"
unsigned char fmChar[8] = 
{ 
	0b11100,
	0b10000,
	0b11000,
	0b10000,
	0b00101,
	0b00111,
	0b00101,
	0b00101
};

// Text-based custom character for "to"
unsigned char toChar[8] = 
{	
	0b11100,
	0b01000,
	0b01000,
	0b01000,
	0b00010,
	0b00101,
	0b00101,
	0b00010
};

// Icon-based custom character for RF traffic
unsigned char rfChar[8] =
{
	0b11111,
	0b10101,
	0b01110,
	0b00100,
	0b00100,
	0b00100,
	0b00100,
	0b00000
};

// Icon-based custom character for network traffic
unsigned char ipChar[8] =
{
	0b00000,
	0b01110,
	0b10001,
	0b00100,
	0b01010,
	0b00000,
	0b00100,
	0b00000
};

// Icon-based custom character for call to talkgroup
unsigned char tgChar[8] =
{
	0b01110,
	0b10001,
	0b10001,
	0b10001,
	0b01010,
	0b01100,
	0b10000,
	0b00000
};

// Icon-based custom character for private call
unsigned char privChar[8] =
{
	0b00100,
	0b00000,
	0b11111,
	0b01110,
	0b01110,
	0b01010,
	0b01010,
	0b00000
};

CHD44780::~CHD44780()
{
}

bool CHD44780::open()
{
	::wiringPiSetup();

	if (m_pwm) {
		if (m_pwmPin != 1U) {
			::softPwmCreate(m_pwmPin, 0, 100);
			::softPwmWrite(m_pwmPin, m_pwmDim);
		} else {
			::pinMode(m_pwmPin, PWM_OUTPUT);
			::pwmWrite(m_pwmPin, (m_pwmDim / 100) * 1024);
		}
	}

#ifdef ADAFRUIT_DISPLAY
  adafruitLCDSetup();
#endif

#ifdef PCF8574_DISPLAY
	pcf8574LCDSetup();
#endif

	m_fd = ::lcdInit(m_rows, m_cols, 4, m_rb, m_strb, m_d0, m_d1, m_d2, m_d3, 0, 0, 0, 0);
	if (m_fd == -1) {
		LogError("Unable to open the HD44780");
		return false;
	}
	
	::lcdDisplay(m_fd, 1);
	::lcdCursor(m_fd, 0);
	::lcdCursorBlink(m_fd, 0);
	::lcdCharDef(m_fd, 0, fmChar);
	::lcdCharDef(m_fd, 1, toChar);
	::lcdCharDef(m_fd, 2, rfChar);
	::lcdCharDef(m_fd, 3, ipChar);
	::lcdCharDef(m_fd, 4, privChar);
	::lcdCharDef(m_fd, 5, tgChar);

	return true;
}

#ifdef ADAFRUIT_DISPLAY
void CHD44780::adafruitLCDSetup()
{
    // The other control pins are initialised with lcdInit()
    ::mcp23017Setup(AF_BASE, m_i2cAddress);

    // Backlight LEDs    
    ::pinMode(AF_RED,   OUTPUT);
    ::pinMode(AF_GREEN, OUTPUT);
    ::pinMode(AF_BLUE,  OUTPUT);

    // Control signals
    ::pinMode(AF_RW, OUTPUT);
    ::digitalWrite(AF_RW, LOW);

		m_rb   = AF_RS;
		m_strb = AF_E;
		m_d0   = AF_D0;
		m_d1   = AF_D1;
		m_d2   = AF_D2;
		m_d3   = AF_D3;
}

void CHD44780::adafruitLCDColour(ADAFRUIT_COLOUR colour)
{
	switch (colour) {
		 case AC_OFF:
		 		::digitalWrite(AF_RED, AF_OFF);
		 		::digitalWrite(AF_GREEN, AF_OFF);
		 		::digitalWrite(AF_BLUE, AF_OFF);
		 		break;
		 case AC_WHITE:
		 		::digitalWrite(AF_RED, AF_ON);
		 		::digitalWrite(AF_GREEN, AF_ON);
		 		::digitalWrite(AF_BLUE, AF_ON);
		 		break;
		 case AC_RED:
		 		::digitalWrite(AF_RED, AF_ON);
		 		::digitalWrite(AF_GREEN, AF_OFF);
		 		::digitalWrite(AF_BLUE, AF_OFF);
		 		break;
		 case AC_GREEN:
		 		::digitalWrite(AF_RED, AF_OFF);
		 		::digitalWrite(AF_GREEN, AF_ON);
		 		::digitalWrite(AF_BLUE, AF_OFF);
		 		break;
		 case AC_BLUE:
		 		::digitalWrite(AF_RED, AF_OFF);
		 		::digitalWrite(AF_GREEN, AF_OFF);
		 		::digitalWrite(AF_BLUE, AF_ON);
		 		break;
		 case AC_PURPLE:
		 		::digitalWrite(AF_RED, AF_ON);
		 		::digitalWrite(AF_GREEN, AF_OFF);
		 		::digitalWrite(AF_BLUE, AF_ON);
		 		break;
		 case AC_YELLOW:
		 		::digitalWrite(AF_RED, AF_ON);
		 		::digitalWrite(AF_GREEN, AF_ON);
		 		::digitalWrite(AF_BLUE, AF_OFF);
		 		break;
		 case AC_ICE:
		 		::digitalWrite(AF_RED, AF_OFF);
		 		::digitalWrite(AF_GREEN, AF_ON);
		 		::digitalWrite(AF_BLUE, AF_ON);
		 		break;
		 default:
		 	break;
	}
}
#endif

#ifdef PCF8574_DISPLAY
void CHD44780::pcf8574LCDSetup()
{
	// Initalize PFC8574
	::pcf8574Setup(AF_BASE, m_i2cAddress);

	// Turn on backlight
	::pinMode (AF_BL, OUTPUT);
	::digitalWrite (AF_BL, 1);

	// Set LCD to write mode.
	::pinMode (AF_RW, OUTPUT);
	::digitalWrite (AF_RW, 0);

	m_rb   = AF_RS;
	m_strb = AF_E;
	m_d0   = AF_D0;
	m_d1   = AF_D1;
	m_d2   = AF_D2;
	m_d3   = AF_D3;
}
#endif

void CHD44780::setIdleInt()
{
	m_clockDisplayTimer.start();          // Start the clock display in IDLE only
	::lcdClear(m_fd);
	
#ifdef ADAFRUIT_DISPLAY
  adafruitLCDColour(AC_WHITE);
#endif

	if (m_pwm) {
		if (m_pwmPin != 1U)
			::softPwmWrite(m_pwmPin, m_pwmDim);
		else
			::pwmWrite(m_pwmPin, (m_pwmDim / 100) * 1024);
	}

	// Print callsign and ID at on top row for all screen sizes
	::lcdPosition(m_fd, 0, 0);
	::lcdPrintf(m_fd, "%-6s", m_callsign.c_str());
	::lcdPosition(m_fd, m_cols - 7, 0);
	::lcdPrintf(m_fd, "%7u", m_dmrid);

	// Print MMDVM and Idle on bottom row for all screen sizes
	::lcdPosition(m_fd, 0, m_rows - 1);
	::lcdPuts(m_fd, "MMDVM");
	::lcdPosition(m_fd, m_cols - 4, m_rows - 1);
	::lcdPuts(m_fd, "Idle");              // Gets overwritten by clock on 2 line screen

	m_dmr = false;
}

void CHD44780::setErrorInt(const char* text)
{
	assert(text != NULL);

#ifdef ADAFRUIT_DISPLAY
  adafruitLCDColour(AC_RED);
#endif

	m_clockDisplayTimer.stop();           // Stop the clock display
	::lcdClear(m_fd);

	if (m_pwm) {
		if (m_pwmPin != 1U)
			::softPwmWrite(m_pwmPin, m_pwmBright);
		else
			::pwmWrite(m_pwmPin, (m_pwmBright / 100) * 1024);
	}

	::lcdPosition(m_fd, 0, 0);
	::lcdPuts(m_fd, "MMDVM");

	::lcdPosition(m_fd, 0, 1);
	::lcdPrintf(m_fd, "%s ERROR", text);

	m_dmr = false;
}

void CHD44780::setLockoutInt()
{
#ifdef ADAFRUIT_DISPLAY
	adafruitLCDColour(AC_RED);
#endif

	m_clockDisplayTimer.stop();           // Stop the clock display
	::lcdClear(m_fd);

	if (m_pwm) {
		if (m_pwmPin != 1U)
			::softPwmWrite(m_pwmPin, m_pwmBright);
		else
			::pwmWrite(m_pwmPin, (m_pwmBright / 100) * 1024);
	}

	::lcdPosition(m_fd, 0, 0);
	::lcdPuts(m_fd, "MMDVM");

	::lcdPosition(m_fd, 0, 1);
	::lcdPuts(m_fd, "Lockout");

	m_dmr = false;
}

void CHD44780::writeDStarInt(const char* my1, const char* my2, const char* your, const char* type, const char* reflector)
{
	assert(my1 != NULL);
	assert(my2 != NULL);
	assert(your != NULL);
	assert(type != NULL);
	assert(reflector != NULL);

#ifdef ADAFRUIT_DISPLAY
		adafruitLCDColour(AC_RED);
#endif

	m_clockDisplayTimer.stop();           // Stop the clock display
	::lcdClear(m_fd);

	if (m_pwm) {
		if (m_pwmPin != 1U)
			::softPwmWrite(m_pwmPin, m_pwmBright);
		else
			::pwmWrite(m_pwmPin, (m_pwmBright / 100) * 1024);
	}

	if (m_rows > 2U) {
		::lcdPosition(m_fd, 0, (m_rows / 2) - 2);
		::sprintf(m_buffer1, "%s%s", "D-Star", DEADSPACE);
		::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);
	}

	::lcdPosition(m_fd, 0, (m_rows / 2) - 1);
	::lcdPutchar(m_fd, 0);
	::lcdPrintf(m_fd, " %.8s/%.4s", my1, my2);
	::lcdPosition(m_fd, m_cols - 1, (m_rows / 2) - 1);

	if (strcmp(type, "R") == 0) {
		::lcdPutchar(m_fd, 2);
	} else {
		::lcdPutchar(m_fd, 3);
	}

	::sprintf(m_buffer1, "%.8s", your);
	
	char *p = m_buffer1;
	for (; *p; ++p) {
		if (*p == ' ')
			*p = '_';
	}

	if (strcmp(reflector, "        ") != 0) {
		if (m_rows == 2 && m_cols == 40) {
			::sprintf(m_buffer3, " via %.8s", reflector);
			strcat(m_buffer1, m_buffer3);
		} else if (m_rows > 2) {
			::sprintf(m_buffer3, "via %.8s", reflector);
			::lcdPosition(m_fd, 0, (m_rows / 2) + 1);
			::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer3);
		}
	}

	::lcdPosition(m_fd, 0, (m_rows / 2));
	::lcdPutchar(m_fd, 1);
	::lcdPrintf(m_fd, " %.*s", m_cols, m_buffer1);

	m_dmr = false;
  m_rssiCount1 = 0U; 
} 
 
void CHD44780::writeDStarRSSIInt(unsigned char rssi) 
{ 
  if (m_rssiCount1 == 0U && m_rows > 2) { 
		::lcdPosition(m_fd, 0, 3);
		::lcdPrintf(m_fd, "-%3udBm", rssi);
  } 
 
  m_rssiCount1++; 
  if (m_rssiCount1 >= DSTAR_RSSI_COUNT) 
    m_rssiCount1 = 0U; 
}

void CHD44780::clearDStarInt()
{
#ifdef ADAFRUIT_DISPLAY
	adafruitLCDColour(AC_PURPLE);
#endif

	m_clockDisplayTimer.stop();           // Stop the clock display
	::lcdClear(m_fd);

	::lcdPosition(m_fd, 0, (m_rows / 2) - 1);
	::sprintf(m_buffer2, "%s%s", "D-Star", DEADSPACE);
	::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer2);
	::lcdPosition(m_fd, 0, (m_rows / 2));
	::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);
}

void CHD44780::writeDMRInt(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type)
{
	assert(type != NULL);

	if (!m_dmr) {
		m_clockDisplayTimer.stop();          // Stop the clock display
		::lcdClear(m_fd);

#ifdef ADAFRUIT_DISPLAY
		adafruitLCDColour(AC_GREEN);
#endif

		if (m_pwm) {
			if (m_pwmPin != 1U)
				::softPwmWrite(m_pwmPin, m_pwmBright);
			else
				::pwmWrite(m_pwmPin, (m_pwmBright / 100) * 1024);
		}

		if (m_duplex) {
			if (m_rows > 2U) {
				::lcdPosition(m_fd, 0, (m_rows / 2) - 2);
				::sprintf(m_buffer1, "%s%s", "DMR", DEADSPACE);
				::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);
			}

			if (slotNo == 1U) {
				//m_dmrScrollTimer2.stop();
				::lcdPosition(m_fd, 0, (m_rows / 2));
				::lcdPrintf(m_fd, "2 %.*s", m_cols - 2U, LISTENING);
			} else {
				//m_dmrScrollTimer1.stop();
				::lcdPosition(m_fd, 0, (m_rows / 2) - 1);
				::lcdPrintf(m_fd, "1 %.*s", m_cols - 2U, LISTENING);
			}
		} else {
			//m_dmrScrollTimer2.stop();

			if (m_rows > 2U) {
				::lcdPosition(m_fd, 0, (m_rows / 2) - 2);
				::sprintf(m_buffer1, "%s", DEADSPACE);
				::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);
			}

			::lcdPosition(m_fd, 0, (m_rows / 2) - 1);
			::sprintf(m_buffer1, "%s%s", "DMR", DEADSPACE);
			::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);
			::lcdPosition(m_fd, 0, (m_rows / 2));
			::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);
		}
	}

#ifdef ADAFRUIT_DISPLAY
	adafruitLCDColour(AC_RED);
#endif

	if (m_duplex) {
		if (m_rows > 2U) {
			::lcdPosition(m_fd, 0, (m_rows / 2) - 2);
			::sprintf(m_buffer1, "%s%s", "DMR", DEADSPACE);
			::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);
		}

		if (slotNo == 1U) {
			::lcdPosition(m_fd, 0, (m_rows / 2) - 1);
			::lcdPuts(m_fd, "1 ");
			if (m_cols > 16 )
				::sprintf(m_buffer1, "%s > %s%s%s", src.c_str(), group ? "TG" : "", dst.c_str(), DEADSPACE);
			else
				::sprintf(m_buffer1, "%s>%s%s", src.c_str(), dst.c_str(), DEADSPACE);
			::lcdPrintf(m_fd, "%.*s", m_cols - 2U, m_buffer1);

			::lcdPosition(m_fd, m_cols - 3U, (m_rows / 2) - 1);
			::lcdPuts(m_fd, " ");

			if (group) {
				::lcdPutchar(m_fd, 5);
			} else {
				::lcdPutchar(m_fd, 4);
			}

			if (strcmp(type, "R") == 0) {
				::lcdPutchar(m_fd, 2);
			} else {
				::lcdPutchar(m_fd, 3);
			}
		} else {
			::lcdPosition(m_fd, 0, (m_rows / 2));
			::lcdPuts(m_fd, "2 ");
			if (m_cols > 16 )
				::sprintf(m_buffer2, "%s > %s%s%s", src.c_str(), group ? "TG" : "", dst.c_str(), DEADSPACE);
			else
				::sprintf(m_buffer2, "%s>%s%s", src.c_str(), dst.c_str(), DEADSPACE);
			::lcdPrintf(m_fd, "%.*s", m_cols - 2U, m_buffer2);

			::lcdPosition(m_fd, m_cols - 3U, (m_rows / 2));
			::lcdPuts(m_fd, " ");

			if (group) {
				::lcdPutchar(m_fd, 5);
			} else {
				::lcdPutchar(m_fd, 4);
			}

			if (strcmp(type, "R") == 0) {
				::lcdPutchar(m_fd, 2);
			} else {
				::lcdPutchar(m_fd, 3);
			}
		}
	} else {
		if (m_rows > 2U) {
			::lcdPosition(m_fd, 0, (m_rows / 2) - 2);
			::sprintf(m_buffer1, "%s%s", "DMR", DEADSPACE);
			::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);
		}

		::lcdPosition(m_fd, 0, (m_rows / 2) - 1);
		::lcdPutchar(m_fd, 0);
		::sprintf(m_buffer2, " %s%s", src.c_str(), DEADSPACE);
		::lcdPrintf(m_fd, "%.*s", m_cols - 4U, m_buffer2);
		::lcdPosition(m_fd, m_cols - 1U, (m_rows / 2) - 1);

		if (strcmp(type, "R") == 0) {
			::lcdPutchar(m_fd, 2);
		} else {
			::lcdPutchar(m_fd, 3);
		}

		::lcdPosition(m_fd, 0, (m_rows / 2));
		::lcdPutchar(m_fd, 1);
		::sprintf(m_buffer2, " %s%s%s", group ? "TG" : "", dst.c_str(), DEADSPACE);
		::lcdPrintf(m_fd, "%.*s", m_cols - 4U, m_buffer2);
		::lcdPosition(m_fd, m_cols - 1U, (m_rows / 2));

		if (group) {
			::lcdPutchar(m_fd, 5);
		} else {
			::lcdPutchar(m_fd, 4);
		}
	}
	m_dmr = true;
	m_rssiCount1 = 0U; 
  m_rssiCount2 = 0U; 
} 
 
void CHD44780::writeDMRRSSIInt(unsigned int slotNo, unsigned char rssi) 
{ 
	if (m_rows > 2) {
		if (slotNo == 1U) { 
			if (m_rssiCount1 == 0U) { 
				::lcdPosition(m_fd, 0, 3);
				::lcdPrintf(m_fd, "-%3udBm", rssi);
			} 

			m_rssiCount1++; 
			if (m_rssiCount1 >= DMR_RSSI_COUNT) 
				m_rssiCount1 = 0U; 
		} else { 
			if (m_rssiCount2 == 0U) { 
				::lcdPosition(m_fd, (m_cols / 2), 3);
				::lcdPrintf(m_fd, "-%3udBm", rssi);
			} 

		m_rssiCount2++; 
		if (m_rssiCount2 >= DMR_RSSI_COUNT) 
			m_rssiCount2 = 0U; 
		}
	}
}

void CHD44780::clearDMRInt(unsigned int slotNo)
{
#ifdef ADAFRUIT_DISPLAY
	adafruitLCDColour(AC_PURPLE);
#endif

	m_clockDisplayTimer.stop();           // Stop the clock display

	if (m_duplex) {
		if (slotNo == 1U) {
			::lcdPosition(m_fd, 0, (m_rows / 2) - 1);
			::lcdPrintf(m_fd, "1 %.*s", m_cols - 2U, LISTENING);

			if (m_rows > 2) { // clear slot 1 RSSI
				::lcdPosition(m_fd, 0, 3);
				::lcdPrintf(m_fd, "%.*s", m_cols / 2, DEADSPACE);
			}
		} else {
			::lcdPosition(m_fd, 0, (m_rows / 2));
			::lcdPrintf(m_fd, "2 %.*s", m_cols - 2U, LISTENING);

			if (m_rows > 2) { // cleat slot 2 RSSI
				::lcdPosition(m_fd, m_cols / 2, 3);
				::lcdPrintf(m_fd, "%.*s", m_cols / 2, DEADSPACE);
			}
		}
	} else {

		if (m_rows > 2U) {
			::lcdPosition(m_fd, 0, (m_rows / 2) - 2);
			::sprintf(m_buffer1, "%s", DEADSPACE);
			::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);
		}

		::lcdPosition(m_fd, 0, (m_rows / 2) - 1);
		::sprintf(m_buffer2, "%s%s", "DMR", DEADSPACE);
		::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer2);
		::lcdPosition(m_fd, 0, (m_rows / 2));
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);
	}
}

void CHD44780::writeFusionInt(const char* source, const char* dest, const char* type, const char* origin)
{
	assert(source != NULL);
	assert(dest != NULL);
	assert(type != NULL);
	assert(origin != NULL);

#ifdef ADAFRUIT_DISPLAY
		adafruitLCDColour(AC_RED);
#endif

	m_clockDisplayTimer.stop();           // Stop the clock display
	::lcdClear(m_fd);

	if (m_pwm) {
		if (m_pwmPin != 1U)
			::softPwmWrite(m_pwmPin, m_pwmBright);
		else
			::pwmWrite(m_pwmPin, (m_pwmBright / 100) * 1024);
	}

	::lcdPosition(m_fd, 0, 0);
	::lcdPuts(m_fd, "System Fusion");

	if (m_rows == 2U && m_cols == 16U) {
		char m_buffer1[16U];
		::sprintf(m_buffer1, "%.10s >", source);
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);
	} else if (m_rows == 4U && m_cols == 16U) {
		char m_buffer1[16U];
		::sprintf(m_buffer1, "%.10s >", source);
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);

		::sprintf(m_buffer1, "%.10s", dest);
		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);
	} else if (m_rows == 4U && m_cols == 20U) {
		char m_buffer1[20U];
		::sprintf(m_buffer1, "%.10s >", source);
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);

		::sprintf(m_buffer1, "%.10s", dest);
		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);
	} else if (m_rows == 2 && m_cols == 40U) {
		char m_buffer1[40U];
		::sprintf(m_buffer1, "%.10s > %.10s", source, dest);

		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);
	}

	m_dmr = false;
  m_rssiCount1 = 0U; 
} 
 
void CHD44780::writeFusionRSSIInt(unsigned char rssi) 
{ 
  if (m_rssiCount1 == 0U && m_rows > 2) { 
		::lcdPosition(m_fd, 0, 3);
		::lcdPrintf(m_fd, "-%3udBm", rssi);
  } 
 
  m_rssiCount1++; 
  if (m_rssiCount1 >= YSF_RSSI_COUNT) 
    m_rssiCount1 = 0U; 
}

void CHD44780::clearFusionInt()
{
#ifdef ADAFRUIT_DISPLAY
	adafruitLCDColour(AC_PURPLE);
#endif

	m_clockDisplayTimer.stop();           // Stop the clock display

	if (m_rows == 2U && m_cols == 16U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);
	} else if (m_rows == 4U && m_cols == 16U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);

		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, "                    ");

		::lcdPosition(m_fd, 0, 3);
		::lcdPrintf(m_fd, "%.*s", m_cols, "                    ");
	} else if (m_rows == 4U && m_cols == 20U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);

		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, "                    ");

		::lcdPosition(m_fd, 0, 3);
		::lcdPrintf(m_fd, "%.*s", m_cols, "                    ");
	} else if (m_rows == 2 && m_cols == 40U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);
	}
}

void CHD44780::writeP25Int(const char* source, bool group, unsigned int dest, const char* type)
{
	assert(source != NULL);
	assert(type != NULL);

#ifdef ADAFRUIT_DISPLAY
		adafruitLCDColour(AC_RED);
#endif

	m_clockDisplayTimer.stop();           // Stop the clock display
	::lcdClear(m_fd);

	if (m_pwm) {
		if (m_pwmPin != 1U)
			::softPwmWrite(m_pwmPin, m_pwmBright);
		else
			::pwmWrite(m_pwmPin, (m_pwmBright / 100) * 1024);
	}

	::lcdPosition(m_fd, 0, 0);
	::lcdPuts(m_fd, "P25");

	if (m_rows == 2U && m_cols == 16U) {
		char m_buffer1[16U];
		::sprintf(m_buffer1, "%.10s >", source);
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);
	} else if (m_rows == 4U && m_cols == 16U) {
		char m_buffer1[16U];
		::sprintf(m_buffer1, "%.10s >", source);
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);

		::sprintf(m_buffer1, "%s%u", group ? "TG" : "", dest);
		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);
	} else if (m_rows == 4U && m_cols == 20U) {
		char m_buffer1[20U];
		::sprintf(m_buffer1, "%.10s >", source);
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);

		::sprintf(m_buffer1, "%s%u", group ? "TG" : "", dest);
		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);
	} else if (m_rows == 2 && m_cols == 40U) {
		char m_buffer1[40U];
		::sprintf(m_buffer1, "%.10s > %s%u", source, group ? "TG" : "", dest);

		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);
	}

	m_dmr = false;
  m_rssiCount1 = 0U; 
} 
 
void CHD44780::writeP25RSSIInt(unsigned char rssi) 
{ 
  if (m_rssiCount1 == 0U && m_rows > 2) { 
		::lcdPosition(m_fd, 0, 3);
		::lcdPrintf(m_fd, "-%3udBm", rssi);
  } 
 
  m_rssiCount1++; 
  if (m_rssiCount1 >= P25_RSSI_COUNT) 
    m_rssiCount1 = 0U; 
}

void CHD44780::clearP25Int()
{
#ifdef ADAFRUIT_DISPLAY
	adafruitLCDColour(AC_PURPLE);
#endif

	m_clockDisplayTimer.stop();           // Stop the clock display

	if (m_rows == 2U && m_cols == 16U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);
	} else if (m_rows == 4U && m_cols == 16U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);

		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, "                    ");

		::lcdPosition(m_fd, 0, 3);
		::lcdPrintf(m_fd, "%.*s", m_cols, "                    ");
	} else if (m_rows == 4U && m_cols == 20U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);

		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, "                    ");

		::lcdPosition(m_fd, 0, 3);
		::lcdPrintf(m_fd, "%.*s", m_cols, "                    ");
	} else if (m_rows == 2 && m_cols == 40U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);
	}
}

void CHD44780::writeNXDNInt(const char* source, bool group, unsigned int dest, const char* type)
{
	assert(source != NULL);
	assert(type != NULL);

#ifdef ADAFRUIT_DISPLAY
		adafruitLCDColour(AC_RED);
#endif

	m_clockDisplayTimer.stop();           // Stop the clock display
	::lcdClear(m_fd);

	if (m_pwm) {
		if (m_pwmPin != 1U)
			::softPwmWrite(m_pwmPin, m_pwmBright);
		else
			::pwmWrite(m_pwmPin, (m_pwmBright / 100) * 1024);
	}

	::lcdPosition(m_fd, 0, 0);
	::lcdPuts(m_fd, "NXDN");

	if (m_rows == 2U && m_cols == 16U) {
		char m_buffer1[16U];
		::sprintf(m_buffer1, "%.10s >", source);
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);
	} else if (m_rows == 4U && m_cols == 16U) {
		char m_buffer1[16U];
		::sprintf(m_buffer1, "%.10s >", source);
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);

		::sprintf(m_buffer1, "%s%u", group ? "TG" : "", dest);
		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);
	} else if (m_rows == 4U && m_cols == 20U) {
		char m_buffer1[20U];
		::sprintf(m_buffer1, "%.10s >", source);
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);

		::sprintf(m_buffer1, "%s%u", group ? "TG" : "", dest);
		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);
	} else if (m_rows == 2 && m_cols == 40U) {
		char m_buffer1[40U];
		::sprintf(m_buffer1, "%.10s > %s%u", source, group ? "TG" : "", dest);

		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, m_buffer1);
	}

	m_dmr = false;
  m_rssiCount1 = 0U; 
} 
 
void CHD44780::writeNXDNRSSIInt(unsigned char rssi) 
{ 
  if (m_rssiCount1 == 0U && m_rows > 2) { 
		::lcdPosition(m_fd, 0, 3);
		::lcdPrintf(m_fd, "-%3udBm", rssi);
  } 
 
  m_rssiCount1++; 
  if (m_rssiCount1 >= NXDN_RSSI_COUNT) 
    m_rssiCount1 = 0U; 
}

void CHD44780::clearNXDNInt()
{
#ifdef ADAFRUIT_DISPLAY
	adafruitLCDColour(AC_PURPLE);
#endif

	m_clockDisplayTimer.stop();           // Stop the clock display

	if (m_rows == 2U && m_cols == 16U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);
	} else if (m_rows == 4U && m_cols == 16U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);

		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, "                    ");

		::lcdPosition(m_fd, 0, 3);
		::lcdPrintf(m_fd, "%.*s", m_cols, "                    ");
	} else if (m_rows == 4U && m_cols == 20U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);

		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, "                    ");

		::lcdPosition(m_fd, 0, 3);
		::lcdPrintf(m_fd, "%.*s", m_cols, "                    ");
	} else if (m_rows == 2 && m_cols == 40U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);
	}
}

void CHD44780::writePOCSAGInt(uint32_t ric, const std::string& message)
{
	::lcdPosition(m_fd, m_cols - 5, m_rows - 1);
	::lcdPuts(m_fd, "POCSAG TX");
}

void CHD44780::clearPOCSAGInt()
{
	::lcdPosition(m_fd, m_cols - 5, m_rows - 1);
	::lcdPuts(m_fd, " Idle");
}

void CHD44780::writeCWInt()
{
	::lcdPosition(m_fd, m_cols - 5, m_rows - 1);
	::lcdPuts(m_fd, "CW TX");
}

void CHD44780::clearCWInt()
{
	::lcdPosition(m_fd, m_cols - 5, m_rows - 1);
	::lcdPuts(m_fd, " Idle");
}

void CHD44780::clockInt(unsigned int ms)
{
	m_clockDisplayTimer.clock(ms);

	// Idle clock display 
	if (m_displayClock && m_clockDisplayTimer.isRunning() && m_clockDisplayTimer.hasExpired()) {
			time_t currentTime;
			struct tm *Time;
			time(&currentTime);

			if (m_utc) {
				Time = gmtime(&currentTime);
			} else {
				Time = localtime(&currentTime);
			}
			
			setlocale(LC_TIME,"");
			strftime(m_buffer1, 128, "%X", Time);  // Time
			strftime(m_buffer2, 128, "%x", Time);  // Date

			if (m_cols == 16U && m_rows == 2U) {
				::lcdPosition(m_fd, m_cols - 10, 1);
				::lcdPrintf(m_fd, "%s%.*s", strlen(m_buffer1) > 8 ? "" : "  ", 10, m_buffer1);
			} else {
				::lcdPosition(m_fd, (m_cols - (strlen(m_buffer1) == 8 ? 8 : 10)) / 2, m_rows == 2 ? 1 : 2);
				::lcdPrintf(m_fd, "%.*s", strlen(m_buffer1) == 8 ? 8 : 10, m_buffer1);
				::lcdPosition(m_fd, (m_cols - strlen(m_buffer2)) / 2, m_rows == 2 ? 0 : 1);
				::lcdPrintf(m_fd, "%s", m_buffer2);
			}

			m_clockDisplayTimer.start();
	}
}

void CHD44780::close()
{
}
