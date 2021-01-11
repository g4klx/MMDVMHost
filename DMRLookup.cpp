/*
*   Copyright (C) 2016,2017 by Jonathan Naylor G4KLX
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
#include "Timer.h"
#include "Log.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

CDMRLookup::CDMRLookup(const std::string& filename, unsigned int reloadTime) :
CThread(),
m_filename(filename),
m_reloadTime(reloadTime),
m_table(),
m_stop(false),
m_reload(false)
{
}

CDMRLookup::~CDMRLookup()
{
}

bool CDMRLookup::read()
{
	bool ret = m_table.load(m_filename);

	if (m_reloadTime > 0U)
		run();

	return ret;
}

void CDMRLookup::reload()
{
	if (m_reloadTime == 0U)
		m_table.load(m_filename);
	else
		m_reload = true;	
}

void CDMRLookup::entry()
{
	LogInfo("Started the DMR Id lookup reload thread");

	CTimer timer(1U, 3600U * m_reloadTime);
	timer.start();

	while (!m_stop) {
		sleep(1000U);

		timer.clock();
		if (timer.hasExpired() || m_reload) {
			m_table.load(m_filename);
			timer.start();
			m_reload = false;
		}
	}

	LogInfo("Stopped the DMR Id lookup reload thread");
}

void CDMRLookup::stop()
{
	if (m_reloadTime == 0U) {
		delete this;
		return;
	}

	m_stop = true;

	wait();
}

void CDMRLookup::findWithName(unsigned int id, class CUserDBentry *entry)
{
	if (id == 0xFFFFFFU) {
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

std::string CDMRLookup::find(unsigned int id)
{
	std::string callsign;

	if (id == 0xFFFFFFU)
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

bool CDMRLookup::exists(unsigned int id)
{
	return m_table.lookup(id, NULL);
}
