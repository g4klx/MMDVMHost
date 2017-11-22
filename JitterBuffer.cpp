/*
*   Copyright (C) 2017 by Jonathan Naylor G4KLX
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

#include <cstdio>
#include <cassert>

CJitterBuffer::CJitterBuffer(unsigned int blockSize, unsigned int blockTime, unsigned int jitterTime, unsigned int topSequenceNumber) :
m_blockSize(blockSize),
m_blockTime(blockTime),
m_topSequenceNumber(topSequenceNumber),
m_blockCount(0U),
m_timer(1000U, 0U, jitterTime),
m_stopWatch(),
m_buffer(NULL),
m_headSequenceNumber(0U),
m_lastData(NULL),
m_lastDataValid(false)
{
	assert(blockSize > 0U);
	assert(blockTime > 0U);
	assert(jitterTime > 0U);
	assert(topSequenceNumber > 0U);

	m_blockCount = jitterTime / blockTime + 1U;
	
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

bool CJitterBuffer::addData(const unsigned char* data, unsigned int sequenceNumber)
{
	assert(data != NULL);

	unsigned int headSequenceNumber =  m_headSequenceNumber % m_topSequenceNumber;
	unsigned int tailSequenceNumber = (m_headSequenceNumber + m_blockCount) % m_topSequenceNumber;

	// Is the data out of sequence?
	if (headSequenceNumber < tailSequenceNumber) {
		if (sequenceNumber < headSequenceNumber || sequenceNumber >= tailSequenceNumber)
			return false;
	} else {
		if (sequenceNumber >= tailSequenceNumber && sequenceNumber < headSequenceNumber)
			return false;
	}

	unsigned int number;
	if (sequenceNumber >= headSequenceNumber)
		number = sequenceNumber - headSequenceNumber;
	else
		number = (sequenceNumber + m_blockCount) - headSequenceNumber;;

	unsigned int index = (m_headSequenceNumber + number) % m_blockCount;

	// Do we already have the data?
	if (m_buffer[index].m_used)
		return false;

	::memcpy(m_buffer[index].m_data, data, m_blockSize);
	m_buffer[index].m_used = true;

	if (!m_timer.isRunning())
		m_timer.start();
	
	return true;
}

JB_STATUS CJitterBuffer::getData(unsigned char* data)
{
	assert(data != NULL);

	if (!m_timer.isRunning() || !m_timer.hasExpired()) {
		m_stopWatch.start();
		return JBS_NO_DATA;
	}

	unsigned int sequenceNumber = m_stopWatch.elapsed() / m_blockTime;
	if (m_headSequenceNumber > sequenceNumber)
		return JBS_NO_DATA;
	
	unsigned int head = m_headSequenceNumber % m_blockCount;

	if (m_buffer[head].m_used) {
		::memcpy(data, m_buffer[head].m_data, m_blockSize);
		m_buffer[head].m_used = false;

		// Save this data in case no more data is available next time
		::memcpy(m_lastData, m_buffer[head].m_data, m_blockSize);
		m_lastDataValid = true;

		m_headSequenceNumber++;

		return JBS_DATA;
	}

	// Return the last data frame or null data if none exists
	if (m_lastDataValid)
		::memcpy(data, m_lastData, m_blockSize);
	else
		::memset(data, 0x00U, m_blockSize);

	m_headSequenceNumber++;
	
	return JBS_REPEAT;
}

void CJitterBuffer::reset()
{
	for (unsigned int i = 0U; i < m_blockCount; i++)
		m_buffer[i].m_used = false;

	m_headSequenceNumber = 0U;

	m_lastDataValid = false;
	
	m_timer.stop();
}

void CJitterBuffer::clock(unsigned int ms)
{
	m_timer.clock(ms);
}
