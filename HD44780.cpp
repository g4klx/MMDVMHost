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

#include <cstdio>
#include <cassert>
#include <cstring>

const char* LISTENING = "Listening                               ";

CHD44780::CHD44780(unsigned int rows, unsigned int cols, const std::string& callsign, unsigned int dmrid, const std::vector<unsigned int>& pins, bool pwm, unsigned int pwmPin, unsigned int pwmBright, unsigned int pwmDim) :
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
m_fd(-1),
m_dmr(false)
{
	assert(rows > 1U);
	assert(cols > 15U);
}

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
		 		::digitalWrite(AF_RED, HIGH);
		 		::digitalWrite(AF_GREEN, HIGH);
		 		::digitalWrite(AF_BLUE, HIGH);
		 		break;
		 case AC_WHITE:
		 		::digitalWrite(AF_RED, LOW);
		 		::digitalWrite(AF_GREEN, LOW);
		 		::digitalWrite(AF_BLUE, LOW);
		 		break;
		 case AC_RED:
		 		::digitalWrite(AF_RED, LOW);
		 		::digitalWrite(AF_GREEN, HIGH);
		 		::digitalWrite(AF_BLUE, HIGH);
		 		break;
		 case AC_GREEN:
		 		::digitalWrite(AF_RED, HIGH);
		 		::digitalWrite(AF_GREEN, LOW);
		 		::digitalWrite(AF_BLUE, HIGH);
		 		break;
		 case AC_BLUE:
		 		::digitalWrite(AF_RED, HIGH);
		 		::digitalWrite(AF_GREEN, HIGH);
		 		::digitalWrite(AF_BLUE, LOW);
		 		break;
		 case AC_PURPLE:
		 		::digitalWrite(AF_RED, LOW);
		 		::digitalWrite(AF_GREEN, HIGH);
		 		::digitalWrite(AF_BLUE, LOW);
		 		break;
		 case AC_YELLOW:
		 		::digitalWrite(AF_RED, LOW);
		 		::digitalWrite(AF_GREEN, LOW);
		 		::digitalWrite(AF_BLUE, HIGH);
		 		break;
		 case AC_ICE:
		 		::digitalWrite(AF_RED, HIGH);
		 		::digitalWrite(AF_GREEN, LOW);
		 		::digitalWrite(AF_BLUE, LOW);
		 		break;
		 default:
		 	break;
	}
}
#endif

void CHD44780::setIdle()
{
	::lcdClear(m_fd);
	
#ifdef ADAFRUIT_DISPLAY
  adafruittLCDColour(AC_WHITE);
#endif

	if (m_pwm) {
		if (m_pwmPin != 1U)
			::softPwmWrite(m_pwmPin, m_pwmDim);
		else
			::pwmWrite(m_pwmPin, (m_pwmDim / 100) * 1024);
	}

	::lcdPosition(m_fd, 0, 0);
	::lcdPrintf(m_fd, "%-6s / %u", m_callsign.c_str(), m_dmrid);

	::lcdPosition(m_fd, 0, 1);
	::lcdPuts(m_fd, "MMDVM Idle");

	m_dmr = false;
}

void CHD44780::setError(const char* text)
{
	assert(text != NULL);

#ifdef ADAFRUIT_DISPLAY
  adafruitLCDColour(AC_RED);
#endif

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

void CHD44780::setLockout()
{
#ifdef ADAFRUIT_DISPLAY
	adafruitLCDColour(AC_RED);
#endif

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

void CHD44780::writeDStar(const char* my1, const char* my2, const char* your, const char* type, const char* reflector)
{
	assert(my1 != NULL);
	assert(my2 != NULL);
	assert(your != NULL);
	assert(type != NULL);
	assert(reflector != NULL);

#ifdef ADAFRUIT_DISPLAY
		adafruitLCDColour(AC_RED);
#endif

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

void CHD44780::clearDStar()
{
#ifdef ADAFRUIT_DISPLAY
	adafruitLCDColour(AC_ICE);
#endif

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

void CHD44780::writeDMR(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type)
{
	assert(type != NULL);

	if (!m_dmr) {
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

		if (m_rows == 2U && m_cols == 16U) {
			if (slotNo == 1U) {
				::lcdPosition(m_fd, 0, 1);
				::lcdPrintf(m_fd, "2 %.*s", m_cols - 2U, LISTENING);
			} else {
				::lcdPosition(m_fd, 0, 0);
				::lcdPrintf(m_fd, "1 %.*s", m_cols - 2U, LISTENING);

			}
		} else if (m_rows == 4U && m_cols == 16U) {
			::lcdPosition(m_fd, 0, 0);
			::lcdPuts(m_fd, "DMR");

			if (slotNo == 1U) {
				::lcdPosition(m_fd, 0, 2);
				::lcdPrintf(m_fd, "2 %.*s", m_cols - 2U, LISTENING);
			} else {
				::lcdPosition(m_fd, 0, 1);
				::lcdPrintf(m_fd, "1 %.*s", m_cols - 2U, LISTENING);
			}
		} else if (m_rows == 4U && m_cols == 20U) {
			::lcdPosition(m_fd, 0, 0);
			::lcdPuts(m_fd, "DMR");

			if (slotNo == 1U) {
				::lcdPosition(m_fd, 0, 2);
				::lcdPrintf(m_fd, "2 %.*s", m_cols - 2U, LISTENING);
			} else {
				::lcdPosition(m_fd, 0, 1);
				::lcdPrintf(m_fd, "1 %.*s", m_cols - 2U, LISTENING);
			}
		} else if (m_rows == 2U && m_cols == 40U) {
			if (slotNo == 1U) {
				::lcdPosition(m_fd, 0, 1);
				::lcdPrintf(m_fd, "2 %.*s", m_cols - 2U, LISTENING);
			} else {
				::lcdPosition(m_fd, 0, 0);
				::lcdPrintf(m_fd, "1 %.*s", m_cols - 2U, LISTENING);
			}
		}
	}

	if (m_rows == 2U && m_cols == 16U) {
#ifdef ADAFRUIT_DISPLAY
		adafruitLCDColour(AC_RED);
#endif

		char buffer[16U];
		if (slotNo == 1U) {
			::sprintf(buffer, "%s >%s%s", src.c_str(), group ? "TG" : "", dst.c_str());
			::lcdPosition(m_fd, 0, 0);
			::lcdPrintf(m_fd, "1 %.*s", m_cols - 2U, buffer);
		} else {
			::sprintf(buffer, "%s >%s%s", src.c_str(), group ? "TG" : "", dst.c_str());
			::lcdPosition(m_fd, 0, 1);
			::lcdPrintf(m_fd, "2 %.*s", m_cols - 2U, buffer);
		}
	} else if (m_rows == 4U && m_cols == 16U) {
#ifdef ADAFRUIT_DISPLAY
		adafruitLCDColour(AC_RED);
#endif

		char buffer[16U];
		if (slotNo == 1U) {
			::sprintf(buffer, "%s %s >%s%s", type, src.c_str(), group ? "TG" : "", dst.c_str());
			::lcdPosition(m_fd, 0, 1);
			::lcdPrintf(m_fd, "1 %.*s", m_cols - 2U, buffer);
		} else {
			::sprintf(buffer, "%s %s >%s%s", type, src.c_str(), group ? "TG" : "", dst.c_str());
			::lcdPosition(m_fd, 0, 2);
			::lcdPrintf(m_fd, "2 %.*s", m_cols - 2U, buffer);
		}
	} else if (m_rows == 4U && m_cols == 20U) {
#ifdef ADAFRUIT_DISPLAY
		adafruitLCDColour(AC_RED);
#endif

		char buffer[20U];
		if (slotNo == 1U) {
			::sprintf(buffer, "%s %s > %s%s", type, src.c_str(), group ? "TG" : "", dst.c_str());
			::lcdPosition(m_fd, 0, 1);
			::lcdPrintf(m_fd, "1 %.*s", m_cols - 2U, buffer);
		} else {
			::sprintf(buffer, "%s %s > %s%s", type, src.c_str(), group ? "TG" : "", dst.c_str());
			::lcdPosition(m_fd, 0, 2);
			::lcdPrintf(m_fd, "2 %.*s", m_cols - 2U, buffer);
		}
	} else if (m_rows == 2U && m_cols == 40U) {
#ifdef ADAFRUIT_DISPLAY
		adafruitLCDColour(AC_RED);
#endif

		char buffer[40U];
		if (slotNo == 1U) {
			::sprintf(buffer, "%s %s > %s%s", type, src.c_str(), group ? "TG" : "", dst.c_str());
			::lcdPosition(m_fd, 0, 0);
			::lcdPrintf(m_fd, "1 %.*s", m_cols - 2U, buffer);
		} else {
			::sprintf(buffer, "%s %s > %s%s", type, src.c_str(), group ? "TG" : "", dst.c_str());
			::lcdPosition(m_fd, 0, 1);
			::lcdPrintf(m_fd, "2 %.*s", m_cols - 2U, buffer);
		}
	}

	m_dmr = true;
}

void CHD44780::clearDMR(unsigned int slotNo)
{
#ifdef ADAFRUIT_DISPLAY
	adafruitLCDColour(AC_ICE);
#endif

	if (m_rows == 2U && m_cols == 16U) {
		if (slotNo == 1U) {
			::lcdPosition(m_fd, 0, 0);
			::lcdPrintf(m_fd, "1 %.*s", m_cols - 2U, LISTENING);
		} else {
			::lcdPosition(m_fd, 0, 1);
			::lcdPrintf(m_fd, "2 %.*s", m_cols - 2U, LISTENING);
		}
	} else if (m_rows == 4U && m_cols == 16U) {
		if (slotNo == 1U) {
			::lcdPosition(m_fd, 0, 1);
			::lcdPrintf(m_fd, "1 %.*s", m_cols - 2U, LISTENING);
		} else {
			::lcdPosition(m_fd, 0, 2);
			::lcdPrintf(m_fd, "2 %.*s", m_cols - 2U, LISTENING);
		}
	} else if (m_rows == 4U && m_cols == 20U) {
		if (slotNo == 1U) {
			::lcdPosition(m_fd, 0, 1);
			::lcdPrintf(m_fd, "1 %.*s", m_cols - 2U, LISTENING);
		} else {
			::lcdPosition(m_fd, 0, 2);
			::lcdPrintf(m_fd, "2 %.*s", m_cols - 2U, LISTENING);
		}
	} else if (m_rows == 2U && m_cols == 40U) {
		if (slotNo == 1U) {
			::lcdPosition(m_fd, 0, 0);
			::lcdPrintf(m_fd, "1 %.*s", m_cols - 2U, LISTENING);
		} else {
			::lcdPosition(m_fd, 0, 1);
			::lcdPrintf(m_fd, "2 %.*s", m_cols - 2U, LISTENING);
		}
	}
}

void CHD44780::writeFusion(const char* source, const char* dest)
{
	assert(source != NULL);
	assert(dest != NULL);

#ifdef ADAFRUIT_DISPLAY
		adafruitLCDColour(AC_RED);
#endif

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

void CHD44780::clearFusion()
{
#ifdef ADAFRUIT_DISPLAY
	adafruitLCDColour(AC_ICE);
#endif

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

void CHD44780::close()
{
}
