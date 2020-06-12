/*
 *   Copyright (C) 2020 by SASANO Takayoshi JG1UAA
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
#include "UserDB.h"
#include "Log.h"

#include <cstdio>
#include <cstring>
#include <cctype>

CUserDB::CUserDB() :
m_table(),
m_mutex()
{
}

CUserDB::~CUserDB()
{
}

bool CUserDB::lookup(unsigned int id, class CUserDBentry *entry)
{
	bool rv;

	m_mutex.lock();

	try {
		if (entry != NULL)
			*entry = m_table.at(id);
		else
			(void)m_table.at(id);

		rv = true;
	} catch (...) {
		rv = false;
	}

	m_mutex.unlock();

	return rv;
}

bool CUserDB::load(std::string const& filename)
{
	FILE* fp = ::fopen(filename.c_str(), "r");
	if (fp == NULL) {
		LogWarning("Cannot open ID lookup file - %s", filename.c_str());
		return false;
	}

	m_mutex.lock();

	// Remove the old entries;
	m_table.clear();

	// set index for entries
	char buffer[256U];
	if (::fgets(buffer, sizeof(buffer), fp) == NULL) {
		LogWarning("ID lookup file has no entry - %s", filename.c_str());
		m_mutex.unlock();
		::fclose(fp);
		return false;
	}

	// no index - set default
	std::unordered_map<std::string, int> index;
	if (!makeindex(buffer, index)) {
		::strncpy(buffer, keyRADIO_ID "," keyCALLSIGN "," keyFIRST_NAME, sizeof(buffer));
		makeindex(buffer, index);
		::rewind(fp);
	}

	while (::fgets(buffer, sizeof(buffer), fp) != NULL) {
		if (buffer[0U] != '#')
			parse(buffer, index);
	}

	::fclose(fp);

	size_t size = m_table.size();
	m_mutex.unlock();

	LogInfo("Loaded %u IDs to lookup table - %s", size, filename.c_str());

	return size != 0U;
}

bool CUserDB::makeindex(char* buf, std::unordered_map<std::string, int>& index)
{
	int i;
	char *p1, *p2;

	// Remove the old index
	index.clear();

	for (i = 0, p1 = tokenize(buf, &p2); p1 != NULL;
	     i++, p1 = tokenize(p2, &p2)) {

		// create [column keyword] - [column number] table
		if (CUserDBentry::isValidKey(p1))
			index[p1] = i;
	}

	try {
		(void)index.at(keyRADIO_ID);
		(void)index.at(keyCALLSIGN);
		return true;
	} catch (...) {
		return false;
	}
}

void CUserDB::parse(char* buf, std::unordered_map<std::string, int>& index)
{
	int i;
	char *p1, *p2;
	std::unordered_map<std::string, char*> ptr;
	unsigned int id;

	for (i = 0, p1 = tokenize(buf, &p2); p1 != NULL;
	     i++, p1 = tokenize(p2, &p2)) {

		for (auto it = index.begin(); it != index.end(); it++) {
			// first: column keyword, second: column number
			if (it->second == i) {
				ptr[it->first] = p1;
				break;
			}
		}
	}

	try {
		(void)ptr.at(keyRADIO_ID);
		(void)ptr.at(keyCALLSIGN);
	} catch (...) {
		return;
	}

	id = (unsigned int)::atoi(ptr[keyRADIO_ID]);
	toupper_string(ptr[keyCALLSIGN]);

	for (auto it = ptr.begin(); it != ptr.end(); it++) {
		// no need to regist radio ID
		if (it->first == keyRADIO_ID)
			continue;

		m_table[id].set(it->first, it->second);
	}
}

void CUserDB::toupper_string(char* str)
{
	while (*str != '\0') {  
		*str = ::toupper(*str);
		str++;
	}
}

char* CUserDB::tokenize(char* str, char** next)
{
	if (*str == '\0')
		return NULL;

	char* p = ::strpbrk(str, ",\t\r\n");
	if (p == NULL) {
		*next = str + ::strlen(str);
	} else {
		*p = '\0';
		*next = p + 1;
	}

	return str;
}
