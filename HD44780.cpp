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

#if defined(RASPBERRY_PI)

#include "HD44780.h"
#include "Log.h"

#include <wiringPi.h>
#include <lcd.h>

#include <cstdio>
#include <cassert>

CHD44780::CHD44780(unsigned int rows, unsigned int cols) :
m_rows(rows),
m_cols(cols),
m_fd(-1)
{
}

CHD44780::~CHD44780()
{
}

bool CHD44780::open()
{
	m_fd = ::lcdInit(m_rows, m_cols, 4, 11, 10, 0, 1, 2, 3, 0, 0, 0, 0);
	if (m_fd == -1) {
		LogError("Unable to open the HD44780");
		return false;
	}

	::lcdDisplay(m_fd, 1);
	::lcdCursor(m_fd, 0);
	::lcdCursorBlink(m_fd, 0);

	return true;
}

void CHD44780::setIdle()
{
	::lcdClear(m_fd);

	::lcdPosition(m_fd, 0, 0);
	::lcdPuts(m_fd, "MMDVM");

	::lcdPosition(m_fd, 0, 1);
	::lcdPuts(m_fd, "Idle");
}

void CHD44780::setLockout()
{
	::lcdClear(m_fd);

	::lcdPosition(m_fd, 0, 0);
	::lcdPuts(m_fd, "MMDVM");

	::lcdPosition(m_fd, 0, 1);
	::lcdPuts(m_fd, "Lockout");
}

void CHD44780::setDStar()
{
	::lcdClear(m_fd);

	::lcdPosition(m_fd, 0, 0);
	::lcdPuts(m_fd, "D-Star");

	::lcdPosition(m_fd, 0, 1);
	::lcdPuts(m_fd, "Listening");
}

void CHD44780::writeDStar(const char* my1, const char* my2, const char* your)
{
	::lcdPosition(m_fd, 0, 1);
	::lcdPrintf(m_fd, "%s/%s > %s", my1, my2, your);
}

void CHD44780::clearDStar()
{
	::lcdPosition(m_fd, 0, 1);
	::lcdPuts(m_fd, "Listening       ");
}

void CHD44780::setDMR()
{
	::lcdClear(m_fd);

	::lcdPosition(m_fd, 0, 0);
	::lcdPuts(m_fd, "1 DMR Listening");

	::lcdPosition(m_fd, 0, 1);
	::lcdPuts(m_fd, "2 DMR Listening");
}

void CHD44780::writeDMR(unsigned int slotNo, unsigned int srcId, bool group, unsigned int dstId, const char* type)
{
	if (slotNo == 1U) {
		::lcdPosition(m_fd, 0, 0);
		::lcdPrintf(m_fd, "1 %s %u > %s%u", type, srcId, group ? "TG" : "", dstId);
	}
	else {
		::lcdPosition(m_fd, 0, 1);
		::lcdPrintf(m_fd, "2 %s %u > %s%u", type, srcId, group ? "TG" : "", dstId);
	}
}

void CHD44780::clearDMR(unsigned int slotNo)
{
	if (slotNo == 1U) {
		::lcdPosition(m_fd, 0, 0);
		::lcdPuts(m_fd, "1 Listening     ");
	} else {
		::lcdPosition(m_fd, 0, 1);
		::lcdPuts(m_fd, "2 Listening     ");
	}
}

void CHD44780::setFusion()
{
	::lcdClear(m_fd);

	::lcdPosition(m_fd, 0, 0);
	::lcdPuts(m_fd, "System Fusion");

	::lcdPosition(m_fd, 0, 1);
	::lcdPuts(m_fd, "Listening");
}

void CHD44780::writeFusion(const char* source, const char* dest)
{
	::lcdPosition(m_fd, 0, 1);
	::lcdPrintf(m_fd, "%s > %s", source, dest);
}

void CHD44780::clearFusion()
{
	::lcdPosition(m_fd, 0, 1);
	::lcdPuts(m_fd, "Listening       ");
}

void CHD44780::close()
{
}

#endif
