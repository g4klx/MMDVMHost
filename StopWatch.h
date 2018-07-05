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

#if !defined(STOPWATCH_H)
#define	STOPWATCH_H

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <sys/time.h>
#endif

class CStopWatch
{
public:
	CStopWatch();
	~CStopWatch();

	unsigned long start();
	unsigned int  elapsed();

private:
#if defined(_WIN32) || defined(_WIN64)
	LARGE_INTEGER  m_frequency;
	LARGE_INTEGER  m_start;
#else
	struct timespec m_start;
#endif
};

#endif
