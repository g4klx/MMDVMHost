/*
*   Copyright (C) 2017,2018 by Jonathan Naylor G4KLX
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

#include "JitterBuffer.h"

#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

CJitterBuffer::CJitterBuffer(const std::string& name, unsigned int blockSize, unsigned int blockTime, unsigned int jitterTime, unsigned int topSequenceNumber, bool debug) :
m_name(name),
m_blockSize(blockSize),
m_blockTime(blockTime),
m_topSequenceNumber(topSequenceNumber),
m_debug(debug),
m_blockCount(0U),
m_timer(1000U, 0U, jitterTime),
m_stopWatch(),
m_running(false),
m_buffer(NULL),
m_headSequenceNumber(0U),
m_appendSequenceNumber(0U),
m_lastData(NULL),
m_lastDataLength(0U)
{
	assert(blockSize > 0U);
	assert(blockTime > 0U);
	assert(jitterTime > 0U);
	assert(topSequenceNumber > 0U);

	m_blockCount = topSequenceNumber / 2U;
	
	m_buffer = new JitterEntry[m_blockCount];

	for (unsigned int i = 0U; i < m_blockCount; i++)
		m_buffer[i].m_data = new unsigned char[m_blockSize];

	m_lastData = new unsigned char[m_blockSize];

	reset();
}

CJitterBuffer::~CJitterBuffer()
{
	for (unsigned int i = 0U; i < m_blockCount; i++)
		delete[] m_buffer[i].m_data;

	delete[] m_buffer;
	delete[] m_lastData;
}

bool CJitterBuffer::addData(const unsigned char* data, unsigned int length, unsigned int sequenceNumber)
{
	assert(data != NULL);
	assert(length > 0U);
	assert(length <= m_blockSize);

	unsigned int headSequenceNumber =  m_headSequenceNumber % m_topSequenceNumber;
	unsigned int tailSequenceNumber = (m_headSequenceNumber + m_blockCount) % m_topSequenceNumber;

	// Is the data out of sequence?
	if (headSequenceNumber < tailSequenceNumber) {
		if (sequenceNumber < headSequenceNumber || sequenceNumber >= tailSequenceNumber) {
			LogDebug("%s, JitterBuffer: rejecting frame with seqNo=%u, raw=%u, head=%u, tail=%u", m_name.c_str(), sequenceNumber, m_headSequenceNumber, headSequenceNumber, tailSequenceNumber);
			return false;
		}
	} else {
		if (sequenceNumber >= tailSequenceNumber && sequenceNumber < headSequenceNumber) {
			LogDebug("%s, JitterBuffer: rejecting frame with seqNo=%u, raw=%u, head=%u, tail=%u", m_name.c_str(), sequenceNumber, m_headSequenceNumber, headSequenceNumber, tailSequenceNumber);
			return false;
		}
	}

	unsigned int number;
	if (sequenceNumber >= headSequenceNumber)
		number = sequenceNumber - headSequenceNumber;
	else
		number = (sequenceNumber + m_blockCount) - headSequenceNumber;;

	unsigned int index = (m_headSequenceNumber + number) % m_blockCount;

	// Do we already have the data?
	if (m_buffer[index].m_length > 0U) {
		LogDebug("%s, JitterBuffer: rejecting duplicate frame with seqNo=%u, pos=%u, raw=%u, head=%u, tail=%u", m_name.c_str(), sequenceNumber, index, m_headSequenceNumber, headSequenceNumber, tailSequenceNumber);
		return false;
	}

	if (m_debug)
		LogDebug("%s, JitterBuffer: adding frame with seqNo=%u, pos=%u, raw=%u, head=%u, tail=%u", m_name.c_str(), sequenceNumber, index, m_headSequenceNumber, headSequenceNumber, tailSequenceNumber);

	::memcpy(m_buffer[index].m_data, data, length);
	m_buffer[index].m_length = length;

	if (!m_timer.isRunning()) {
		LogDebug("%s, JitterBuffer: starting the timer", m_name.c_str());
		m_timer.start();
	}
	
	return true;
}

bool CJitterBuffer::appendData(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);
	assert(length <= m_blockSize);

	unsigned int headSequenceNumber = m_headSequenceNumber % m_topSequenceNumber;
	unsigned int tailSequenceNumber = (m_headSequenceNumber + m_blockCount) % m_topSequenceNumber;

	// Is the data out of sequence?
	if (headSequenceNumber < tailSequenceNumber) {
		if (m_appendSequenceNumber < headSequenceNumber || m_appendSequenceNumber >= tailSequenceNumber) {
			LogDebug("%s, JitterBuffer: rejecting append frame with seqNo=%u, raw=%u, head=%u, tail=%u", m_name.c_str(), m_appendSequenceNumber, m_headSequenceNumber, headSequenceNumber, tailSequenceNumber);
			return false;
		}
	} else {
		if (m_appendSequenceNumber >= tailSequenceNumber && m_appendSequenceNumber < headSequenceNumber) {
			LogDebug("%s, JitterBuffer: rejecting append frame with seqNo=%u, raw=%u, head=%u, tail=%u", m_name.c_str(), m_appendSequenceNumber, m_headSequenceNumber, headSequenceNumber, tailSequenceNumber);
			return false;
		}
	}

	unsigned int number;
	if (m_appendSequenceNumber >= headSequenceNumber)
		number = m_appendSequenceNumber - headSequenceNumber;
	else
		number = (m_appendSequenceNumber + m_blockCount) - headSequenceNumber;;

	unsigned int index = (m_headSequenceNumber + number) % m_blockCount;

	if (m_debug)
		LogDebug("%s, JitterBuffer: appending frame with seqNo=%u, pos=%u, raw=%u, head=%u, tail=%u", m_name.c_str(), m_appendSequenceNumber, index, m_headSequenceNumber, headSequenceNumber, tailSequenceNumber);

	::memcpy(m_buffer[index].m_data, data, length);
	m_buffer[index].m_length = length;

	if (!m_timer.isRunning()) {
		LogDebug("%s, JitterBuffer: starting the timer from append", m_name.c_str());
		m_timer.start();
	}

	m_appendSequenceNumber++;

	return true;
}

B_STATUS CJitterBuffer::getData(unsigned char* data, unsigned int& length)
{
	assert(data != NULL);

	if (!m_running)
		return BS_NO_DATA;

	unsigned int sequenceNumber = m_stopWatch.elapsed() / m_blockTime + 2U;
	if (m_headSequenceNumber > sequenceNumber)
		return BS_NO_DATA;
	
	unsigned int head = m_headSequenceNumber % m_blockCount;

	m_headSequenceNumber++;

	if (m_buffer[head].m_length > 0U) {
		if (m_debug)
			LogDebug("%s, JitterBuffer: returning data, elapsed=%ums, raw=%u, head=%u", m_name.c_str(), m_stopWatch.elapsed(), m_headSequenceNumber - 1U, head);

		::memcpy(data, m_buffer[head].m_data, m_buffer[head].m_length);
		length = m_buffer[head].m_length;

		// Save this data in case no more data is available next time
		::memcpy(m_lastData, m_buffer[head].m_data, m_buffer[head].m_length);
		m_lastDataLength = m_buffer[head].m_length;

		m_buffer[head].m_length = 0U;

		return BS_DATA;
	}

	m_buffer[head].m_length = 0U;

	LogDebug("%s, JitterBuffer: no data available, elapsed=%ums, raw=%u, head=%u", m_name.c_str(), m_stopWatch.elapsed(), m_headSequenceNumber - 1U, head);

	// Return the last data frame if we have it
	if (m_lastDataLength > 0U) {
		LogDebug("%s, JitterBuffer: returning the last received frame", m_name.c_str());
		::memcpy(data, m_lastData, m_lastDataLength);
		length = m_lastDataLength;

		return BS_MISSING;
	}

	return BS_NO_DATA;
}

void CJitterBuffer::reset()
{
	for (unsigned int i = 0U; i < m_blockCount; i++)
		m_buffer[i].m_length = 0U;

	m_headSequenceNumber = 0U;
	m_appendSequenceNumber = 0U;

	m_lastDataLength = 0U;
	
	m_timer.stop();

	m_running = false;
}

void CJitterBuffer::clock(unsigned int ms)
{
	m_timer.clock(ms);
	if (m_timer.isRunning() && m_timer.hasExpired()) {
		if (!m_running) {
			m_stopWatch.start();
			m_running = true;
		}
	}
}
