/*
*   Copyright (C) 2018 by Jonathan Naylor G4KLX
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

#include "DelayBuffer.h"

#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

CDelayBuffer::CDelayBuffer(const std::string& name, unsigned int blockSize, unsigned int blockTime, unsigned int jitterTime, bool debug) :
m_name(name),
m_blockSize(blockSize),
m_blockTime(blockTime),
m_debug(debug),
m_timer(1000U, 0U, jitterTime),
m_stopWatch(),
m_running(false),
m_buffer(1000U, name.c_str()),
m_outputCount(0U),
m_lastData(NULL),
m_lastDataLength(0U)
{
	assert(blockSize > 0U);
	assert(blockTime > 0U);
	assert(jitterTime > 0U);

	m_lastData = new unsigned char[m_blockSize];

	reset();
}

CDelayBuffer::~CDelayBuffer()
{
	delete[] m_lastData;
}

bool CDelayBuffer::addData(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);
	assert(length == m_blockSize);

	if (m_debug)
		LogDebug("%s, DelayBuffer: appending data", m_name.c_str());

	m_buffer.addData(data, length);

	if (!m_timer.isRunning()) {
		LogDebug("%s, DelayBuffer: starting the timer from append", m_name.c_str());
		m_timer.start();
	}

	return true;
}

B_STATUS CDelayBuffer::getData(unsigned char* data, unsigned int& length)
{
	assert(data != NULL);

	if (!m_running)
		return BS_NO_DATA;

	unsigned int needed = m_stopWatch.elapsed() / m_blockTime + 2U;
	if (needed <= m_outputCount)
		return BS_NO_DATA;

	if (!m_buffer.isEmpty()) {
		if (m_debug)
			LogDebug("%s, DelayBuffer: returning data, elapsed=%ums", m_name.c_str(), m_stopWatch.elapsed());

		length = m_buffer.getData(data, m_blockSize);

		// Save this data in case no more data is available next time
		::memcpy(m_lastData, data, length);
		m_lastDataLength = length;

		m_outputCount++;

		return BS_DATA;
	}

	LogDebug("%s, DelayBuffer: no data available, elapsed=%ums", m_name.c_str(), m_stopWatch.elapsed());

	// Return the last data frame if we have it
	if (m_lastDataLength > 0U) {
		LogDebug("%s, DelayBuffer: returning the last received frame", m_name.c_str());
		::memcpy(data, m_lastData, m_lastDataLength);
		length = m_lastDataLength;

		m_outputCount++;

		return BS_MISSING;
	}

	return BS_NO_DATA;
}

void CDelayBuffer::reset()
{
	m_buffer.clear();

	m_lastDataLength = 0U;

	m_outputCount = 0U;

	m_timer.stop();

	m_running = false;
}

void CDelayBuffer::clock(unsigned int ms)
{
	m_timer.clock(ms);
	if (m_timer.isRunning() && m_timer.hasExpired()) {
		if (!m_running) {
			m_stopWatch.start();
			m_running = true;
		}
	}
}
