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

#include "YSFParrot.h"
#include "YSFDefines.h"
#include "Defines.h"

#include <cstdio>
#include <cassert>
#include <cstring>

CYSFParrot::CYSFParrot(unsigned int timeout) :
m_data(NULL),
m_length(timeout * 1220U + 1000U),
m_used(0U),
m_ptr(0U),
m_timer(1000U, 2U)
{
	assert(timeout > 0U);

	m_data = new unsigned char[m_length];
}

CYSFParrot::~CYSFParrot()
{
	delete[] m_data;
}

bool CYSFParrot::write(const unsigned char* data)
{
	assert(data != NULL);

	if ((m_length - m_used) < 1000U)
		return false;

	::memcpy(m_data + m_used, data, YSF_FRAME_LENGTH_BYTES + 2U);
	m_used += YSF_FRAME_LENGTH_BYTES + 2U;

	return true;
}

void CYSFParrot::end()
{
	if (m_used > 0U)
		m_timer.start();

	m_ptr = 0U;
}

void CYSFParrot::read(unsigned char* data)
{
	assert(data != NULL);

	if (m_used == 0U)
		return;

	::memcpy(data, m_data + m_ptr, YSF_FRAME_LENGTH_BYTES + 2U);
	m_ptr += YSF_FRAME_LENGTH_BYTES + 2U;

	if (m_ptr >= m_used) {
		m_timer.stop();
		m_used = 0U;
	}
}

bool CYSFParrot::hasData()
{
	if (m_timer.isRunning() && m_timer.hasExpired())
		return true;

	if (m_used == 0U)
		return false;

	return false;
}

void CYSFParrot::clock(unsigned int ms)
{
	m_timer.clock(ms);
}
