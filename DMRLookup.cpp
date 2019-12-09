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
m_mutex(),
m_stop(false)
{
}

CDMRLookup::~CDMRLookup()
{
}

bool CDMRLookup::read()
{
	bool ret = (m_filename.rfind(".csv") == std::string::npos) ?
		load() : loadcsv();

	if (m_reloadTime > 0U)
		run();

	return ret;
}

void CDMRLookup::entry()
{
	LogInfo("Started the DMR Id lookup reload thread");

	CTimer timer(1U, 3600U * m_reloadTime);
	timer.start();

	while (!m_stop) {
		sleep(1000U);

		timer.clock();
		if (timer.hasExpired()) {
			load();
			timer.start();
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

std::string CDMRLookup::findWithName(unsigned int id)
{
	std::string callsign, name;

	if (id == 0xFFFFFFU)
		return std::string("ALL");

	m_mutex.lock();

	try {
		callsign = m_table.at(id);
		LogDebug("FindWithName =%s",callsign.c_str());
		
	} catch (...) {
		char text[10U];
		::sprintf(text, "%u", id);
		callsign = std::string(text);
	}

	try {
		name = " " + m_tableName.at(id);
	} catch (...) {
		name = "";
	}

	m_mutex.unlock();

	return callsign + name;
}
std::string CDMRLookup::find(unsigned int id)
{
	std::string callsign;
	std::string b;
	
	
	if (id == 0xFFFFFFU)
		return std::string("ALL");

	m_mutex.lock();

	try {
		b = m_table.at(id);
		size_t n = b.find(" ");
		if (n > 0) {
			callsign = b.substr(0,n);
			
		} else {
			LogDebug("b=%s",b.c_str());
			callsign = b;
		}
		
	} catch (...) {
		char text[10U];
		::sprintf(text, "%u", id);
		callsign = std::string(text);
	}

	m_mutex.unlock();

	return callsign;
}

void CDMRLookup::findUserInfo(unsigned int id, std::string& name, std::string& city, std::string& state, std::string& country)
{
	m_mutex.lock();

	try {
		name = m_tableName.at(id);
	} catch (...) {
		name = "";
	}

	try {
		city = m_tableCity.at(id);
	} catch (...) {
		city = "";
	}

	try {
		state = m_tableState.at(id);
	} catch (...) {
		state = "";
	}

	try {
		country = m_tableCountry.at(id);
	} catch (...) {
		country = "";
	}

	m_mutex.unlock();
}

bool CDMRLookup::exists(unsigned int id)
{
	m_mutex.lock();

	bool found = m_table.count(id) == 1U;

	m_mutex.unlock();

	return found;
}

bool CDMRLookup::load()
{
	FILE* fp = ::fopen(m_filename.c_str(), "rt");
	if (fp == NULL) {
		LogWarning("Cannot open the DMR Id lookup file - %s", m_filename.c_str());
		return false;
	}

	m_mutex.lock();

	// Remove the old entries
	m_table.clear();
	m_tableName.clear();
	m_tableCity.clear();
	m_tableState.clear();
	m_tableCountry.clear();

	char buffer[100U];
	while (::fgets(buffer, 100U, fp) != NULL) {
		if (buffer[0U] == '#')
			continue;

		char* p1 = ::strtok(buffer, " \t\r\n");
		char* p2 = ::strtok(NULL, " \r\n");  // tokenize to eol to capture name as well

		if (p1 != NULL && p2 != NULL) {
			unsigned int id = (unsigned int)::atoi(p1);
			for (char* p = p2; *p != 0x00U; p++) {
				
				if(*p == 0x09U) 
					*p = 0x20U;
				
				else 
					*p = ::toupper(*p);
				
			}
			m_table[id] = std::string(p2);
		}
	}

	m_mutex.unlock();

	::fclose(fp);

	size_t size = m_table.size();
	if (size == 0U)
		return false;

	LogInfo("Loaded %u Ids to the DMR callsign lookup table", size);

	return true;
}

bool CDMRLookup::loadcsv()
{
	FILE* fp = ::fopen(m_filename.c_str(), "r");
	if (fp == NULL) {
		LogWarning("Cannot open the DMR Id lookup file - %s", m_filename.c_str());
		return false;
	}

	m_mutex.lock();

	// Remove the old entries
	m_table.clear();
	m_tableName.clear();
	m_tableCity.clear();
	m_tableState.clear();
	m_tableCountry.clear();

	char buffer[256U];

	// set index for entries
	if (::fgets(buffer, sizeof(buffer), fp) == NULL) {
		LogWarning("DMR Id lookup file has no entry - %s", m_filename.c_str());
		m_mutex.unlock();
		::fclose(fp);
		return false;
	}

	char *p1, *p2;
	int n, ixId, ixCall, ixFName, ixLName, ixCity, ixState, ixCountry;
	ixId = ixCall = ixFName = ixLName = ixCity = ixState = ixCountry = -1;
	for (n = 0, p1 = tokenize(buffer, &p2); p1 != NULL;
	     n++, p1 = tokenize(p2, &p2)) {
		if (!::strcmp("RADIO_ID", p1)) ixId = n;
		else if (!::strcmp("CALLSIGN", p1)) ixCall = n;
		else if (!::strcmp("FIRST_NAME", p1)) ixFName = n;
		else if (!::strcmp("LAST_NAME", p1)) ixLName = n;
		else if (!::strcmp("CITY", p1)) ixCity = n;
		else if (!::strcmp("STATE", p1)) ixState = n;
		else if (!::strcmp("COUNTRY", p1)) ixCountry = n;
	}

	// register entries
	char *pId, *pCall, *pFName, *pLName, *pCity, *pState, *pCountry;
	while (::fgets(buffer, sizeof(buffer), fp) != NULL) {
		pId = pCall = pFName = pLName = pCity = pState = pCountry = NULL;
		for (n = 0, p1 = tokenize(buffer, &p2); p1 != NULL;
		     n++, p1 = tokenize(p2, &p2)) {
			if (n == ixId) pId = p1;
			else if (n == ixCall) pCall = p1;
			else if (n == ixFName) pFName = p1;
			else if (n == ixLName) pLName = p1;
			else if (n == ixCity) pCity = p1;
			else if (n == ixState) pState = p1;
			else if (n == ixCountry) pCountry = p1;
		}

		if (pId != NULL && pCall != NULL) {
			unsigned int id = (unsigned int)::atoi(pId);
			for (char *p = pCall; *p; p++) *p = ::toupper(*p);
			m_table[id] = std::string(pCall);
			if (pFName != NULL && pLName != NULL) m_tableName[id] = std::string(pFName) + " " + std::string(pLName);
			if (pCity != NULL) m_tableCity[id] = std::string(pCity);
			if (pState != NULL) m_tableState[id] = std::string(pState);
			if (pCountry != NULL) m_tableCountry[id] = std::string(pCountry);
		}
	}

	m_mutex.unlock();

	::fclose(fp);

	size_t size = m_table.size();
	if (size == 0U)
		return false;

	LogInfo("Loaded %u Ids to the DMR callsign lookup table", size);

	return true;
}

char* CDMRLookup::tokenize(char *str, char **next)
{
	if (*str == '\0') return NULL;

	char* p = ::strpbrk(str, ",\t\r\n");
	if (p == NULL) {
		*next = str + ::strlen(str);
	} else {
		*p = '\0';
		*next = p + 1;
	}

	return str;
}
