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

#include "Mutex.h"

#if defined(_WIN32) || defined(_WIN64)

CMutex::CMutex() :
m_handle()
{
	m_handle = ::CreateMutex(NULL, FALSE, NULL);
}

CMutex::~CMutex()
{
	::CloseHandle(m_handle);
}

void CMutex::lock()
{
	::WaitForSingleObject(m_handle, INFINITE);
}

void CMutex::unlock()
{
	::ReleaseMutex(m_handle);
}

#else

CMutex::CMutex() :
m_mutex(PTHREAD_MUTEX_INITIALIZER)
{
}

CMutex::~CMutex()
{
}

void CMutex::lock()
{
	::pthread_mutex_lock(&m_mutex);
}

void CMutex::unlock()
{
	::pthread_mutex_unlock(&m_mutex);
}

#endif
