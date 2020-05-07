/*
 *   Copyright (C) 2020 by Jonathan Naylor G4KLX
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

#include "FMControl.h"

#include <string>
#include <ctime>

CFMControl::CFMControl(CFMNetwork* network) :
m_network(network),
m_queue(3000U, "FM Control"),
m_enabled(false),
m_fp(NULL)
{
    assert(network != NULL);
}

CFMControl::~CFMControl()
{
}

bool CFMControl::writeModem(unsigned char* data, unsigned int len)
{
    assert(data != NULL);
    assert(len > 0U);

    // Unpack serial data (12-bit unsigned values)

    // De-emphasise the data

    // Repack the data (16-bit unsigned values)

    return true;
}

unsigned int CFMControl::readModem(unsigned char* data, unsigned int space)
{
    assert(data != NULL);
    assert(space > 0U);

    // Unpack serial data (16-bit unsigned values)

    // Pre-emphasise the data

    // Repack the data (12-bit unsigned values)

    return 0U;
}

void CFMControl::clock(unsigned int ms)
{
    // May not be needed
}

void CFMControl::enable(bool enabled)
{
    // May not be needed
}

bool CFMControl::openFile()
{
    if (m_fp != NULL)
        return true;

    time_t t;
    ::time(&t);

    struct tm* tm = ::localtime(&t);

    char name[100U];
    ::sprintf(name, "FM_%04d%02d%02d_%02d%02d%02d.ambe", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    m_fp = ::fopen(name, "wb");
    if (m_fp == NULL)
        return false;

    ::fwrite("FM", 1U, 2U, m_fp);

    return true;
}

bool CFMControl::writeFile(const unsigned char* data, unsigned int length)
{
    if (m_fp == NULL)
        return false;

    ::fwrite(data, 1U, length, m_fp);

    return true;
}

void CFMControl::closeFile()
{
    if (m_fp != NULL) {
        ::fclose(m_fp);
        m_fp = NULL;
    }
}
