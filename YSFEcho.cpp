/*
 *   Copyright (C) 2015 by Jonathan Naylor G4KLX
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

#include "YSFEcho.h"

#include "YSFDefines.h"

CYSFEcho::CYSFEcho(unsigned int delay, unsigned int space) :
m_buffer(space, "YSF Echo"),
m_timer(1000U, delay)
{
}

CYSFEcho::~CYSFEcho()
{
}

unsigned int CYSFEcho::readData(unsigned char* data)
{
	if (!hasData())
		return 0U;

	unsigned char len;
	m_buffer.getData(&len, 1U);

	m_buffer.getData(data, len);

	// If the FICH is valid, regenerate the sync
	if ((data[1U] & 0x01U) == 0x01U) {
		data[2U] = YSF_SYNC_BYTES[0U];
		data[3U] = YSF_SYNC_BYTES[1U];
		data[4U] = YSF_SYNC_BYTES[2U];
		data[5U] = YSF_SYNC_BYTES[3U];
		data[6U] = YSF_SYNC_BYTES[4U];
	}

	if (!hasData())
		m_timer.stop();

	return len;
}

bool CYSFEcho::writeData(const unsigned char* data, unsigned int length)
{
	bool ret = m_buffer.hasSpace(length + 1U);
	if (!ret)
		return false;

	unsigned char len = length;
	m_buffer.addData(&len, 1U);

	m_buffer.addData(data, length);

	m_timer.start();

	return true;
}

bool CYSFEcho::hasData()
{
	if (m_timer.isRunning() && m_timer.hasExpired())
		return m_buffer.hasData();
	else
		return false;
}

void CYSFEcho::clock(unsigned int ms)
{
	m_timer.clock(ms);
}
