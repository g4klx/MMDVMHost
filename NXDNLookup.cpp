/*
*   Copyright (C) 2016,2017,2018 by Jonathan Naylor G4KLX
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

#include "NXDNLookup.h"
#include "Timer.h"
#include "Log.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

CNXDNLookup::CNXDNLookup(const std::string& filename, unsigned int reloadTime) :
CThread(),
m_filename(filename),
m_reloadTime(reloadTime),
m_table(),
m_stop(false)
{
}

CNXDNLookup::~CNXDNLookup()
{
}

bool CNXDNLookup::read()
{
	bool ret = m_table.load(m_filename);

	if (m_reloadTime > 0U)
		run();

	return ret;
}

void CNXDNLookup::entry()
{
	LogInfo("Started the NXDN Id lookup reload thread");

	CTimer timer(1U, 3600U * m_reloadTime);
	timer.start();

	while (!m_stop) {
		sleep(1000U);

		timer.clock();
		if (timer.hasExpired()) {
			m_table.load(m_filename);
			timer.start();
		}
	}

	LogInfo("Stopped the NXDN Id lookup reload thread");
}

void CNXDNLookup::stop()
{
	if (m_reloadTime == 0U) {
		delete this;
		return;
	}

	m_stop = true;

	wait();
}

void CNXDNLookup::findWithName(unsigned int id, class CUserDBentry *entry)
{
	if (id == 0xFFFFU) {
		entry->clear();
		entry->set(keyCALLSIGN, "ALL");
		return;
	}

	if (m_table.lookup(id, entry)) {
		LogDebug("FindWithName =%s %s", entry->get(keyCALLSIGN).c_str(), entry->get(keyFIRST_NAME).c_str());
	} else {
		entry->clear();

		char text[10U];
		::snprintf(text, sizeof(text), "%u", id);
		entry->set(keyCALLSIGN, text);
	}

	return;
}

std::string CNXDNLookup::find(unsigned int id)
{
	std::string callsign;

	if (id == 0xFFFFU)
		return std::string("ALL");

	class CUserDBentry entry;
	if (m_table.lookup(id, &entry)) {
		callsign = entry.get(keyCALLSIGN);
	} else {
		char text[10U];
		::snprintf(text, sizeof(text), "%u", id);
		callsign = std::string(text);
	}

	return callsign;
}

bool CNXDNLookup::exists(unsigned int id)
{
	return m_table.lookup(id, NULL);
}
