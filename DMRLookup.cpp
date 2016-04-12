/*
*   Copyright (C) 2016 by Jonathan Naylor G4KLX
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

#include "DMRLookup.h"
#include "Log.h"

#include <cstdio>
#include <cstring>
#include <cctype>

CDMRLookup::CDMRLookup(const std::string& filename) :
m_filename(filename)
{
}

CDMRLookup::~CDMRLookup()
{
}

bool CDMRLookup::read()
{
	FILE* fp = ::fopen(m_filename.c_str(), "rt");
	if (fp == NULL) {
		LogWarning("Cannot open the DMR Id lookup file - %s", m_filename.c_str());
		return false;
	}

	char buffer[100U];
	while (::fgets(buffer, 100U, fp) != NULL) {
		if (buffer[0U] == '#')
			continue;

		char* p1 = ::strtok(buffer, " \t\r\n");
		char* p2 = ::strtok(NULL, " \t\r\n");

		if (p1 != NULL && p2 != NULL) {
			unsigned int id = (unsigned int)::atoi(p1);
			for (char* p = p2; *p != 0x00U; p++)
				*p = ::toupper(*p);

			m_table[id] = std::string(p2);
		}
	}

	::fclose(fp);

	size_t size = m_table.size();
	if (size == 0U)
		return false;

	LogInfo("Loaded %u DMR Ids to the callsign lookup table", size);

	return true;
}

std::string CDMRLookup::find(unsigned int id) const
{
	std::string callsign;

	try {
		callsign = m_table.at(id);
	} catch (...) {
		char text[10U];
		::sprintf(text, "%u", id);
		callsign = std::string(text);
	}

	return callsign;
}
