/*
 *   Copyright (C) 2016,2025 by Jonathan Naylor G4KLX
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

#include "RSSIInterpolator.h"

#include "Log.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

CRSSIInterpolator::CRSSIInterpolator() :
m_map()
{
}

CRSSIInterpolator::~CRSSIInterpolator()
{
	m_map.clear();
}

bool CRSSIInterpolator::load(const std::string& filename)
{
	FILE* fp = ::fopen(filename.c_str(), "rt");
	if (fp == nullptr) {
		LogWarning("Cannot open the RSSI data file - %s", filename.c_str());
		return false;
	}
	
	char buffer[100U];
	while (::fgets(buffer, 100, fp) != nullptr) {
		if (buffer[0U] == '#')
			continue;

		char* p1 = ::strtok(buffer, " \t\r\n");
		char* p2 = ::strtok(nullptr,   " \t\r\n");

		if (p1 != nullptr && p2 != nullptr) {
			uint16_t raw = uint16_t(::atoi(p1));
			int     rssi = ::atoi(p2);
			m_map.insert(std::pair<uint16_t, int>(raw, rssi));
		}
	}

	::fclose(fp);

	LogInfo("Loaded %u RSSI data mapping points from %s", m_map.size(), filename.c_str());
	
	return true;
}

int CRSSIInterpolator::interpolate(uint16_t val) const
{
	if (m_map.empty())
		return 0;

	std::map<uint16_t, int>::const_iterator it = m_map.lower_bound(val);

	if (it == m_map.end())
		return m_map.rbegin()->second;

	if (it == m_map.begin())
		return it->second;

	uint16_t x2 = it->first;
	int      y2 = it->second;

	--it;
	uint16_t x1 = it->first;
	int      y1 = it->second;

	float p = float(val - x1) / float(x2 - x1);

	return int((1.0F - p) * float(y1) + p * float(y2));
}
