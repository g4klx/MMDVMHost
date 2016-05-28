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

CHD44780::CHD44780(unsigned int rows, unsigned int cols, const std::string& callsign, unsigned int dmrid, const std::vector<unsigned int>& pins, bool pwm, unsigned int pwmPin, unsigned int pwmBright, unsigned int pwmDim, bool displayClock, bool utc, bool duplex) :
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
m_pwm(pwm),
m_pwmPin(pwmPin),
m_pwmBright(pwmBright),
m_pwmDim(pwmDim),
m_displayClock(displayClock),
m_utc(utc),
m_duplex(duplex),
//m_duplex(true), // uncomment to force duplex display for testing!
m_fd(-1),
m_dmr(false),
m_clockDisplayTimer(1000U, 0U, 75U)		// Update the clock display every 75ms
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

// Custom "M" character used in MMDVM logo
unsigned char mChar[8] = 
{	
	0b10001,
	0b11011,
	0b10101,
	0b10001,
	0b10001,
	0b00000,
	0b11111,
	0b11111
};

// Custom "D" character used in MMDVM logo
unsigned char dChar[8] = 
{	
	0b11110,
	0b10001,
	0b10001,
	0b10001,
	0b11110,
	0b00000,
	0b11111,
	0b11111
};

// Custom "V" character used in MMDVM logo
unsigned char vChar[8] = 
{	
	0b10001,
	0b10001,
	0b10001,
	0b01010,
	0b00100,
	0b00000,
	0b11111,
	0b11111
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
	::lcdCharDef(m_fd, 2, mChar);
	::lcdCharDef(m_fd, 3, dChar);
	::lcdCharDef(m_fd, 4, vChar);

	/* 
		TG, private call, RF and network icons defined as needed - ran out of CGRAM locations
		on the HD44780!  Theoretically, we now have infinite custom characters to play with,
		just be mindful of the slow speed of CGRAM hence the lcdPosition call to delay just 
		long enough so the CGRAM can be written before we try to read it.
	*/

	return true;
}

#ifdef ADAFRUIT_DISPLAY
void CHD44780::adafruitLCDSetup()
{
    // The other control pins are initialised with lcdInit()
    ::mcp23017Setup(AF_BASE, MCP23017);

    // Backlight LEDs    
    ::pinMode(AF_RED,   OUTPUT);
    ::pinMode(AF_GREEN, OUTPUT);
    ::pinMode(AF_BLUE,  OUTPUT);

    // Control signals
    ::pinMode(AF_RW, OUTPUT);
    ::digitalWrite(AF_RW, LOW);
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

void CHD44780::setIdleInt()
{
	m_clockDisplayTimer.start(); // Start the clock display in IDLE only
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

	::lcdPosition(m_fd, 0, 0);
	::lcdPrintf(m_fd, "%-6s", m_callsign.c_str());
	::lcdPosition(m_fd, m_cols - 7, 0);
	::lcdPrintf(m_fd, "%7u", m_dmrid);

	::lcdPosition(m_fd, 0, 1);
	::lcdPutchar(m_fd, 2);
	::lcdPutchar(m_fd, 2);
	::lcdPutchar(m_fd, 3);
	::lcdPutchar(m_fd, 4);
	::lcdPutchar(m_fd, 2);
	if (!m_displayClock || m_cols > 16)
		::lcdPuts(m_fd, " Idle");

	m_dmr = false;
}

void CHD44780::setErrorInt(const char* text)
{
	assert(text != NULL);

#ifdef ADAFRUIT_DISPLAY
  adafruitLCDColour(AC_RED);
#endif

	m_clockDisplayTimer.stop(); // Stop the clock display
	::lcdClear(m_fd);

	if (m_pwm) {
		if (m_pwmPin != 1U)
			::softPwmWrite(m_pwmPin, m_pwmBright);
		else
			::pwmWrite(m_pwmPin, (m_pwmBright / 100) * 1024);
	}

	::lcdPosition(m_fd, 0, 0);
	::lcdPutchar(m_fd, 2);
	::lcdPutchar(m_fd, 2);
	::lcdPutchar(m_fd, 3);
	::lcdPutchar(m_fd, 4);
	::lcdPutchar(m_fd, 2);

	::lcdPosition(m_fd, 0, 1);
	::lcdPrintf(m_fd, "%s ERROR", text);

	m_dmr = false;
}

void CHD44780::setLockoutInt()
{
#ifdef ADAFRUIT_DISPLAY
	adafruitLCDColour(AC_RED);
#endif

	m_clockDisplayTimer.stop(); // Stop the clock display
	::lcdClear(m_fd);

	if (m_pwm) {
		if (m_pwmPin != 1U)
			::softPwmWrite(m_pwmPin, m_pwmBright);
		else
			::pwmWrite(m_pwmPin, (m_pwmBright / 100) * 1024);
	}

	::lcdPosition(m_fd, 0, 0);
	::lcdPutchar(m_fd, 2);
	::lcdPutchar(m_fd, 2);
	::lcdPutchar(m_fd, 3);
	::lcdPutchar(m_fd, 4);
	::lcdPutchar(m_fd, 2);

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

	m_clockDisplayTimer.stop(); // Stop the clock display
	::lcdClear(m_fd);

	if (m_pwm) {
		if (m_pwmPin != 1U)
			::softPwmWrite(m_pwmPin, m_pwmBright);
		else
			::pwmWrite(m_pwmPin, (m_pwmBright / 100) * 1024);
	}

	::lcdPosition(m_fd, 0, 0);
	::lcdPuts(m_fd, "D-Star");

	if (m_rows == 2U && m_cols == 16U) {
		char buffer[16U];
		::sprintf(buffer, "%s %.8s/%.4s", type, my1, my2);
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, buffer);
	} else if (m_rows == 4U && m_cols == 16U) {
		char buffer[16U];
		::sprintf(buffer, "%s %.8s/%.4s", type, my1, my2);
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, buffer);

		if (strcmp(reflector, "        ") == 0)
			::sprintf(buffer, "%.8s", your);
		else
			::sprintf(buffer, "%.8s<%.8s", your, reflector);

		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, buffer);
	} else if (m_rows == 4U && m_cols == 20U) {
		char buffer[20U];
		::sprintf(buffer, "%s %.8s/%.4s >", type, my1, my2);
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, buffer);

		if (strcmp(reflector, "        ") == 0)
			::sprintf(buffer, "%.8s", your);
		else
			::sprintf(buffer, "%.8s <- %.8s", your, reflector);

		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, buffer);
	} else if (m_rows == 2 && m_cols == 40U) {
		char buffer[40U];
		if (strcmp(reflector, "        ") == 0)
			::sprintf(buffer, "%s %.8s/%.4s > %.8s", type, my1, my2, your);
		else
			::sprintf(buffer, "%s %.8s/%.4s > %.8s via %.8s", type, my1, my2, your, reflector);

		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, buffer);
	}

	m_dmr = false;
}

void CHD44780::clearDStarInt()
{
#ifdef ADAFRUIT_DISPLAY
	adafruitLCDColour(AC_PURPLE);
#endif

	m_clockDisplayTimer.stop(); // Stop the clock display

	if (m_rows == 2U && m_cols == 16U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);
	} else if (m_rows == 4U && m_cols == 16U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);

		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, "                    ");
	} else if (m_rows == 4U && m_cols == 20U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);

		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, "                    ");
	} else if (m_rows == 2 && m_cols == 40U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);
	}
}

void CHD44780::writeDMRInt(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type)
{
	char buffer[128]; // force 128 char buffer - we're never getting that far but stops us overflowing it!
	assert(type != NULL);

	if (!m_dmr) {
		m_clockDisplayTimer.stop(); // Stop the clock display
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
				::sprintf(buffer, "%s%s", "DMR", DEADSPACE);
				::lcdPrintf(m_fd, "%.*s", m_cols, buffer);
			}

			if (slotNo == 1U) {
				::lcdPosition(m_fd, 0, (m_rows / 2));
				::lcdPrintf(m_fd, "2 %.*s", m_cols - 2U, LISTENING);
			} else {
				::lcdPosition(m_fd, 0, (m_rows / 2) - 1);
				::lcdPrintf(m_fd, "1 %.*s", m_cols - 2U, LISTENING);
			}
		} else {
			::lcdPosition(m_fd, 0, (m_rows / 2) - 1);
			::sprintf(buffer, "%s%s", "DMR", DEADSPACE);
			::lcdPrintf(m_fd, "%.*s", m_cols, buffer);
			::lcdPosition(m_fd, 0, (m_rows / 2));
			::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);
		}
	}

#ifdef ADAFRUIT_DISPLAY
	adafruitLCDColour(AC_RED);
#endif

	if (m_duplex) {
		if (slotNo == 1U) {
			::lcdPosition(m_fd, 0, (m_rows / 2) - 1);
			::lcdPuts(m_fd, "1 ");
			::sprintf(buffer, "%s > %s%s", src.c_str(), dst.c_str(), DEADSPACE);
			::lcdPrintf(m_fd, "%.*s", m_cols - 2U, buffer);

			if (m_cols > 16) {
				::lcdCharDef(m_fd, 6, group ? tgChar : privChar);
				::lcdCharDef(m_fd, 5, strcmp(type, "R") == 0 ? rfChar : ipChar);
				::lcdPosition(m_fd, m_cols - 3U, (m_rows / 2) - 1);
				::lcdPuts(m_fd, " ");
				::lcdPutchar(m_fd, 6);
				::lcdPutchar(m_fd, 5);
			}
		} else {
			::lcdPosition(m_fd, 0, (m_rows / 2));
			::lcdPuts(m_fd, "2 ");
			::sprintf(buffer, "%s > %s%s", src.c_str(), dst.c_str(), DEADSPACE);
			::lcdPrintf(m_fd, "%.*s", m_cols - 2U, buffer);

			if (m_cols > 16) {
				::lcdCharDef(m_fd, 6, group ? tgChar : privChar);
				::lcdCharDef(m_fd, 5, strcmp(type, "R") == 0 ? rfChar : ipChar);
				::lcdPosition(m_fd, m_cols - 3U, (m_rows / 2));
				::lcdPuts(m_fd, " ");
				::lcdPutchar(m_fd, 6);
				::lcdPutchar(m_fd, 5);
			}
		}
	} else {
		::lcdPosition(m_fd, 0, (m_rows / 2) - 1);
		::lcdPutchar(m_fd, 0);
		::sprintf(buffer, " %s%s", src.c_str(), DEADSPACE);
		::lcdPrintf(m_fd, "%.*s", m_cols - 4U, buffer);
		::lcdCharDef(m_fd, 5, strcmp(type, "R") == 0 ? rfChar : ipChar);
		::lcdPosition(m_fd, m_cols - 1U, (m_rows / 2) - 1);
		::lcdPutchar(m_fd, 5);

		::lcdPosition(m_fd, 0, (m_rows / 2));
		::lcdPutchar(m_fd, 1);
		::sprintf(buffer, " %s%s", dst.c_str(), DEADSPACE);
		::lcdPrintf(m_fd, "%.*s", m_cols - 4U, buffer);
		::lcdCharDef(m_fd, 6, group ? tgChar : privChar);
		::lcdPosition(m_fd, m_cols - 1U, (m_rows / 2));
		::lcdPutchar(m_fd, 6);
	}
	m_dmr = true;
}

void CHD44780::clearDMRInt(unsigned int slotNo)
{
	char buffer[128]; // force 128 char buffer - we're never getting that far but stops us overflowing it!

#ifdef ADAFRUIT_DISPLAY
	adafruitLCDColour(AC_PURPLE);
#endif

	m_clockDisplayTimer.stop(); // Stop the clock display
	if (m_duplex) {
		if (slotNo == 1U) {
			::lcdPosition(m_fd, 0, 0);
			::lcdPrintf(m_fd, "1 %.*s", m_cols - 2U, LISTENING);
		} else {
			::lcdPosition(m_fd, 0, 1);
			::lcdPrintf(m_fd, "2 %.*s", m_cols - 2U, LISTENING);
		}
	} else {
			::lcdPosition(m_fd, 0, (m_rows / 2) - 1);
			::sprintf(buffer, "%s%s", "DMR", DEADSPACE);
			::lcdPrintf(m_fd, "%.*s", m_cols, buffer);
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

	m_clockDisplayTimer.stop(); // Stop the clock display
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
		char buffer[16U];
		::sprintf(buffer, "%.10s >", source);
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, buffer);
	} else if (m_rows == 4U && m_cols == 16U) {
		char buffer[16U];
		::sprintf(buffer, "%.10s >", source);
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, buffer);

		::sprintf(buffer, "%.10s", dest);
		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, buffer);
	} else if (m_rows == 4U && m_cols == 20U) {
		char buffer[20U];
		::sprintf(buffer, "%.10s >", source);
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, buffer);

		::sprintf(buffer, "%.10s", dest);
		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, buffer);
	} else if (m_rows == 2 && m_cols == 40U) {
		char buffer[40U];
		::sprintf(buffer, "%.10s > %.10s", source, dest);

		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, buffer);
	}

	m_dmr = false;
}

void CHD44780::clearFusionInt()
{
#ifdef ADAFRUIT_DISPLAY
	adafruitLCDColour(AC_PURPLE);
#endif

	m_clockDisplayTimer.stop(); // Stop the clock display

	if (m_rows == 2U && m_cols == 16U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);
	} else if (m_rows == 4U && m_cols == 16U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);

		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, "                    ");
	} else if (m_rows == 4U && m_cols == 20U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);

		::lcdPosition(m_fd, 0, 2);
		::lcdPrintf(m_fd, "%.*s", m_cols, "                    ");
	} else if (m_rows == 2 && m_cols == 40U) {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "%.*s", m_cols, LISTENING);
	}
}

void CHD44780::clockInt(unsigned int ms)
{
	// Update the clock display in IDLE mode every 75ms
	m_clockDisplayTimer.clock(ms);
	if (m_displayClock && m_clockDisplayTimer.isRunning() && m_clockDisplayTimer.hasExpired()) {
			time_t currentTime;
			struct tm *Time;
			time(&currentTime);                   // Get the current time

			if (m_utc){
				Time = gmtime(&currentTime);
			} else {
				Time = localtime(&currentTime);
			}
			
			//int Day    = Time->tm_mday;
			//int Month  = Time->tm_mon + 1;
			//int Year   = Time->tm_year + 1900;
			int Hour   = Time->tm_hour;
			int Min    = Time->tm_min;
			int Sec    = Time->tm_sec;
			
			::lcdPosition(m_fd, m_cols - 8, 1);
			::lcdPrintf(m_fd, "%02d:%02d:%02d", Hour, Min, Sec);
			m_clockDisplayTimer.start(); // restart the clock display timer
	}
}

void CHD44780::close()
{
}
