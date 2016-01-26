/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
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
m_frequency(),
m_start()
{
	::QueryPerformanceFrequency(&m_frequency);
}

CStopWatch::~CStopWatch()
{
}

unsigned long CStopWatch::start()
{
	::QueryPerformanceCounter(&m_start);

	return (unsigned long)(m_start.QuadPart / m_frequency.QuadPart);
}

unsigned int CStopWatch::elapsed()
{
	LARGE_INTEGER now;
	::QueryPerformanceCounter(&now);

	LARGE_INTEGER temp;
	temp.QuadPart = (now.QuadPart - m_start.QuadPart) * 1000;

	return (unsigned int)(temp.QuadPart / m_frequency.QuadPart);
}

#else

#include <cstdio>

CStopWatch::CStopWatch() :
m_start()
{
}

CStopWatch::~CStopWatch()
{
}

unsigned long CStopWatch::start()
{
	::gettimeofday(&m_start, NULL);

	return m_start.tv_usec;
}

unsigned int CStopWatch::elapsed()
{
	struct timeval now;
	::gettimeofday(&now, NULL);

	unsigned int elapsed = (now.tv_sec - m_start.tv_sec) * 1000U;
	elapsed += now.tv_usec / 1000U;
	elapsed -= m_start.tv_usec / 1000U;

	return elapsed;
}

#endif
