/*
 *	Copyright (C) 2020 Jonathan Naylor, G4KLX
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 */

#include "AX25Control.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <ctime>

// #define	DUMP_AX25

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CAX25Control::CAX25Control(CAX25Network* network) :
m_network(network),
m_enabled(true),
m_fp(NULL)
{
}

CAX25Control::~CAX25Control()
{
}

bool CAX25Control::writeModem(unsigned char *data, unsigned int len)
{
	assert(data != NULL);

	if (!m_enabled)
		return false;

	unsigned char type = data[0U];

	return true;
}

bool CAX25Control::openFile()
{
	if (m_fp != NULL)
		return true;

	time_t t;
	::time(&t);

	struct tm* tm = ::localtime(&t);

	char name[100U];
	::sprintf(name, "AX25_%04d%02d%02d_%02d%02d%02d.ambe", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	m_fp = ::fopen(name, "wb");
	if (m_fp == NULL)
		return false;

	::fwrite("AX25", 1U, 4U, m_fp);

	return true;
}

bool CAX25Control::writeFile(const unsigned char* data, unsigned int length)
{
	if (m_fp == NULL)
		return false;

	::fwrite(&length, 1U, sizeof(unsigned int), m_fp);
	::fwrite(data, 1U, length, m_fp);

	return true;
}

void CAX25Control::closeFile()
{
	if (m_fp != NULL) {
		::fclose(m_fp);
		m_fp = NULL;
	}
}

void CAX25Control::enable(bool enabled)
{
	m_enabled = enabled;
}
