/*
 *   Copyright (C) 2015,2016,2018 by Jonathan Naylor G4KLX
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

#include "StopWatch.h"

#if defined(_WIN32) || defined(_WIN64)

CStopWatch::CStopWatch() :
m_frequencyS(),
m_frequencyMS(),
m_start()
{
	::QueryPerformanceFrequency(&m_frequencyS);

	m_frequencyMS.QuadPart = m_frequencyS.QuadPart / 1000ULL;
}

CStopWatch::~CStopWatch()
{
}

unsigned long long CStopWatch::time() const
{
	LARGE_INTEGER now;
	::QueryPerformanceCounter(&now);

	return (unsigned long long)(now.QuadPart / m_frequencyMS.QuadPart);
}

unsigned long long CStopWatch::start()
{
	::QueryPerformanceCounter(&m_start);

	return (unsigned long long)(m_start.QuadPart / m_frequencyS.QuadPart);
}

unsigned int CStopWatch::elapsed()
{
	LARGE_INTEGER now;
	::QueryPerformanceCounter(&now);

	LARGE_INTEGER temp;
	temp.QuadPart = (now.QuadPart - m_start.QuadPart) * 1000;

	return (unsigned int)(temp.QuadPart / m_frequencyS.QuadPart);
}

#else

#include <cstdio>
#include <ctime>

CStopWatch::CStopWatch() :
m_startMS(0ULL)
{
}

CStopWatch::~CStopWatch()
{
}

unsigned long long CStopWatch::time() const
{
	struct timeval now;
	::gettimeofday(&now, NULL);

	return now.tv_sec * 1000ULL + now.tv_usec / 1000ULL;
}

unsigned long long CStopWatch::start()
{
	struct timespec now;
	::clock_gettime(CLOCK_MONOTONIC, &now);

	m_startMS = now.tv_sec * 1000ULL + now.tv_nsec / 1000000ULL;

	return m_startMS;
}

unsigned int CStopWatch::elapsed()
{
	struct timespec now;
	::clock_gettime(CLOCK_MONOTONIC, &now);

	unsigned long long nowMS = now.tv_sec * 1000ULL + now.tv_nsec / 1000000ULL;

	return nowMS - m_startMS;
}

#endif
