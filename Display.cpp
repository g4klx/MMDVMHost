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

#include "Display.h"
#include "Defines.h"

#include <cstdio>
#include <cassert>
#include <cstring>

CDisplay::CDisplay() :
m_timer1(1000U, 3U),
m_timer2(1000U, 3U),
m_mode1(MODE_IDLE),
m_mode2(MODE_IDLE)
{
}

CDisplay::~CDisplay()
{
}

void CDisplay::setIdle()
{
	m_timer1.stop();
	m_timer2.stop();

	m_mode1 = MODE_IDLE;
	m_mode2 = MODE_IDLE;

	setIdleInt();
}

void CDisplay::setLockout()
{
	m_timer1.stop();
	m_timer2.stop();

	m_mode1 = MODE_IDLE;
	m_mode2 = MODE_IDLE;

	setLockoutInt();
}

void CDisplay::setError(const char* text)
{
	assert(text != NULL);

	m_timer1.stop();
	m_timer2.stop();

	m_mode1 = MODE_IDLE;
	m_mode2 = MODE_IDLE;

	setErrorInt(text);
}

void CDisplay::writeDStar(const char* my1, const char* my2, const char* your, const char* type, const char* reflector)
{
	assert(my1 != NULL);
	assert(my2 != NULL);
	assert(your != NULL);
	assert(type != NULL);
	assert(reflector != NULL);

	m_timer1.start();

	writeDStarInt(my1, my2, your, type, reflector);
}

void CDisplay::clearDStar()
{
	if (m_timer1.hasExpired()) {
		clearDStarInt();
		m_timer1.stop();
	} else {
		m_mode1 = MODE_DSTAR;
	}
}

void CDisplay::writeDMR(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const char* type)
{
	assert(type != NULL);

	if (slotNo == 1U)
		m_timer1.start();
	else
		m_timer2.start();

	writeDMRInt(slotNo, src, group, dst, type);
}

void CDisplay::clearDMR(unsigned int slotNo)
{
	if (slotNo == 1U) {
		if (m_timer1.hasExpired()) {
			clearDMRInt(slotNo);
			m_timer1.stop();
		} else {
			m_mode1 = MODE_DMR;
		}
	} else {
		if (m_timer2.hasExpired()) {
			clearDMRInt(slotNo);
			m_timer2.stop();
		} else {
			m_mode2 = MODE_DMR;
		}
	}
}

void CDisplay::writeFusion(const char* source, const char* dest)
{
	assert(source != NULL);
	assert(dest != NULL);

	m_timer1.start();

	writeFusionInt(source, dest);
}

void CDisplay::clearFusion()
{
	if (m_timer1.hasExpired()) {
		clearFusionInt();
		m_timer1.stop();
	} else {
		m_mode1 = MODE_YSF;
	}
}

void CDisplay::clock(unsigned int ms)
{
	m_timer1.clock(ms);
	if (m_timer1.isRunning() && m_timer1.hasExpired()) {
		switch (m_mode1) {
		case MODE_DSTAR:
			clearDStarInt();
			break;
		case MODE_DMR:
			clearDMRInt(1U);
			break;
		case MODE_YSF:
			clearFusionInt();
			break;
		}

		m_mode1 = MODE_IDLE;
		m_timer1.stop();
	}

	m_timer2.clock(ms);
	if (m_timer2.isRunning() && m_timer2.hasExpired()) {
		switch (m_mode2) {
		case MODE_DSTAR:
			clearDStarInt();
			break;
		case MODE_DMR:
			clearDMRInt(2U);
			break;
		case MODE_YSF:
			clearFusionInt();
			break;
		}

		m_mode2 = MODE_IDLE;
		m_timer2.stop();
	}
}
