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

#if !defined(_WIN32) || !defined(_WIN64)

#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

unsigned int ptr;

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

	if (id == 0xFFFFFFU)
		return std::string("ALL");

	try {
		callsign = m_table.at(id);
	} catch (...) {
		char text[10U];
		::sprintf(text, "%u", id);
		callsign = std::string(text);
	}

	return callsign;
}

#if !defined(_WIN32) || !defined(_WIN64)
void *readfile_thread(void* objpass)
{
	CDMRLookup* obj = (CDMRLookup*)objpass;
	
	struct stat stat_buffer;
	
	int mtime = 0;
	
	LogInfo("Calling DMR Ids read from file in thread - check for changes every 10 minutes.");
	while (1) {
	    int result = stat(obj->m_filename.c_str(), &stat_buffer);
	    if (result != 0)
	    {
		LogWarning("Can't stat DMR IDs file - %s : %s",obj->m_filename.c_str(),strerror(errno));
	    } else if (mtime < stat_buffer.st_mtime || mtime == 0) {
	      
		mtime = stat_buffer.st_mtime;
		
		FILE* fp = ::fopen(obj->m_filename.c_str(), "rt");
		if (fp == NULL) {
			LogWarning("Cannot open the DMR Id lookup file - %s", obj->m_filename.c_str());
		} else {

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

				    obj->m_table[id] = std::string(p2);
			    }
	    
		    }
		    ::fclose(fp);

		    size_t size = obj->m_table.size();
		    if (size == 0U)
			    LogWarning("DMR IDs - No data loaded");
		    else
			    LogInfo("Loaded %u DMR Ids to the callsign lookup table", size);

	      
		}
	    }
	  ::usleep(600000000);
	
	}
	return 0;
}

bool CDMRLookup::threaded()
{
    pthread_t t1;
    if (!::pthread_create(&t1, NULL, &readfile_thread, this)) {
      ::pthread_detach(t1);
      return true;
    } else {
	LogWarning("Could not spawn thread to load DMR IDs");
	return false;
    }
    
}
#endif