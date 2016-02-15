/*
 *   Copyright (C) 2006-2009,2012,2013,2015,2016 by Jonathan Naylor G4KLX
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

#ifndef RingBuffer_H
#define RingBuffer_H

#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

template<class T> class CRingBuffer {
public:
	CRingBuffer(unsigned int length, const char* name) :
	m_length(length),
	m_name(name),
	m_buffer(NULL),
	m_iPtr(0U),
	m_oPtr(0U),
	m_full(false)
	{
		assert(length > 0U);
		assert(name != NULL);

		m_buffer = new T[length];

		::memset(m_buffer, 0x00, m_length * sizeof(T));
	}

	~CRingBuffer()
	{
		delete[] m_buffer;
	}

	bool addData(const T* buffer, unsigned int nSamples)
	{
		for (unsigned int i = 0U; i < nSamples; i++) {
			if (m_full) {
				LogError("**** Overflow in %s ring buffer", m_name);
				return false;
			}

			m_buffer[m_iPtr++] = buffer[i];

			if (m_iPtr == m_length)
				m_iPtr = 0U;

			if (m_iPtr == m_oPtr)
				m_full = true;
		}

		return true;
	}

	unsigned int getData(T* buffer, unsigned int nSamples)
	{
		unsigned int data = dataSize();

		if (data < nSamples)
			nSamples = data;

		for (unsigned int i = 0U; i < nSamples; i++) {
			m_full = false;

			buffer[i] = m_buffer[m_oPtr++];

			if (m_oPtr == m_length)
				m_oPtr = 0U;
		}

		return nSamples;
	}

	unsigned int peek(T* buffer, unsigned int nSamples)
	{
		unsigned int data = dataSize();

		if (data < nSamples)
			nSamples = data;

		unsigned int ptr = m_oPtr;
		for (unsigned int i = 0U; i < nSamples; i++) {
			buffer[i] = m_buffer[ptr++];

			if (ptr == m_length)
				ptr = 0U;
		}

		return nSamples;
	}

	void clear()
	{
		m_iPtr = 0U;
		m_oPtr = 0U;
		m_full = false;

		::memset(m_buffer, 0x00, m_length * sizeof(T));
	}

	unsigned int freeSpace() const
	{
		if (m_oPtr == m_iPtr && !m_full)
			return m_length;

		if (m_oPtr == m_iPtr && m_full)
			return 0U;

		if (m_iPtr > m_oPtr)
			return m_iPtr - m_oPtr;

		return (m_length + m_iPtr) - m_oPtr;
	}

	unsigned int dataSize() const
	{
		return m_length - freeSpace();
	}

	bool hasSpace(unsigned int length) const
	{
		return freeSpace() > length;
	}

	bool hasData() const
	{
		return m_oPtr != m_iPtr || (m_oPtr == m_iPtr && m_full);
	}

	bool isEmpty() const
	{
		return m_oPtr == m_iPtr && !m_full;
	}

private:
	unsigned int          m_length;
	const char*           m_name;
	T*                    m_buffer;
	volatile unsigned int m_iPtr;
	volatile unsigned int m_oPtr;
	volatile bool         m_full;
};

#endif
