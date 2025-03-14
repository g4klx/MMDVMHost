/*
 *   Copyright (C) 2015,2016,2020,2022,2023,2025 by Jonathan Naylor G4KLX
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

#include "Log.h"
#include "MQTTConnection.h"

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <ctime>
#include <cassert>
#include <cstring>

CMQTTConnection* m_mqtt = nullptr;

static unsigned int m_mqttLevel = 2U;

static unsigned int m_displayLevel = 2U;

static char LEVELS[] = " DMIWEF";

void LogInitialise(unsigned int displayLevel, unsigned int mqttLevel)
{
	m_mqttLevel    = mqttLevel;
	m_displayLevel = displayLevel;
}

void LogFinalise()
{
	if (m_mqtt != nullptr) {
		m_mqtt->close();
		delete m_mqtt;
		m_mqtt = nullptr;
	}
}

void Log(unsigned int level, const char* fmt, ...)
{
	assert(fmt != nullptr);

	char buffer[501U];
#if defined(_WIN32) || defined(_WIN64)
	SYSTEMTIME st;
	::GetSystemTime(&st);

	::sprintf(buffer, "%c: %04u-%02u-%02u %02u:%02u:%02u.%03u ", LEVELS[level], st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#else
	struct timeval now;
	::gettimeofday(&now, nullptr);

	struct tm* tm = ::gmtime(&now.tv_sec);

	::sprintf(buffer, "%c: %04d-%02d-%02d %02d:%02d:%02d.%03lld ", LEVELS[level], tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, now.tv_usec / 1000LL);
#endif

	va_list vl;
	va_start(vl, fmt);

	::vsnprintf(buffer + ::strlen(buffer), 500 - ::strlen(buffer), fmt, vl);

	va_end(vl);

	if (m_mqtt != nullptr && level >= m_mqttLevel && m_mqttLevel != 0U)
		m_mqtt->publish("log", buffer);

	if (level >= m_displayLevel && m_displayLevel != 0U) {
		::fprintf(stdout, "%s\n", buffer);
		::fflush(stdout);
	}

	if (level == 6U)		// Fatal
		exit(1);
}

void WriteJSON(const std::string& topLevel, nlohmann::json& json)
{
	if (m_mqtt != nullptr) {
		nlohmann::json top;

		top[topLevel] = json;

		m_mqtt->publish("json", top.dump());
	}
}

