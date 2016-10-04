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
#include <cstdlib>
#include <cstring>
#include <cctype>

#if !defined(_WIN32) || !defined(_WIN64)

#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

pthread_mutex_t lock_x=PTHREAD_MUTEX_INITIALIZER;

unsigned int m_sleep_time;

#endif


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
		LogWarning("Cannot open the Id lookup file - %s", m_filename.c_str());
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

	LogInfo("Loaded %u Ids to the callsign lookup table", size);

	return true;
}

std::string CDMRLookup::find(unsigned int id) const
{
	std::string callsign;

	if (id == 0xFFFFFFU)
		return std::string("ALL");
	
#if !defined(_WIN32) || !defined(_WIN64)
	pthread_mutex_lock(&lock_x);
#endif
	try {
		callsign = m_table.at(id);
	  
	} catch (...) {
		char text[10U];
		::sprintf(text, "%u", id);
		callsign = std::string(text);
	}
#if !defined(_WIN32) || !defined(_WIN64)
	pthread_mutex_unlock(&lock_x);
#endif
	return callsign;
}

#if !defined(_WIN32) || !defined(_WIN64)
void *readfile_thread(void* objpass)
{
	CDMRLookup* obj = (CDMRLookup*)objpass;
	
	std::string filename = obj->get_filename();
	unsigned int sleep_time = obj->get_sleep_time();
	
	
	struct stat stat_buffer;
	
	int mtime = 0;
	
	std::unordered_map<unsigned int, std::string> table_read;
	
	LogInfo("Calling DMR Ids read from file in thread - check for changes every %u minutes.",sleep_time);
	while (1) {
	  LogInfo("Checking for changes in DMR IDs file - %s",filename.c_str());
	    int result = stat(filename.c_str(), &stat_buffer);
	    if (result != 0)
	    {
		LogWarning("Can't stat DMR IDs file - %s : %s",filename.c_str(),strerror(errno));
	    } else if (mtime < stat_buffer.st_mtime || mtime == 0) {
	      
		mtime = stat_buffer.st_mtime;
		
		FILE* fp = ::fopen(filename.c_str(), "rt");
		if (fp == NULL) {
			LogWarning("Cannot open the DMR Id lookup file - %s", filename.c_str());
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

				   // obj->m_table[id] = std::string(p2);
				    table_read[id] = std::string(p2);
			    }
	    
		    }
		    ::fclose(fp);
		    
		    pthread_mutex_lock(&lock_x);
		    size_t size = obj->set_m_table(table_read);
		    pthread_mutex_unlock(&lock_x);
  
		    if (size == 0U)
			    LogWarning("DMR IDs - No data loaded");
		    else
			    LogInfo("Loaded %u DMR Ids to the callsign lookup table", size);

	      
		}
	    }
	  ::usleep(((sleep_time*60)*1000)*1000);
	
	}
	return 0;
}

std::string CDMRLookup::get_filename() const
{
    return m_filename;
}

unsigned int CDMRLookup::get_sleep_time() const
{
    return m_sleep_time;
}

unsigned int CDMRLookup::set_m_table(std::unordered_map<unsigned int, std::string> table)
{
    m_table = table;
    size_t size = m_table.size();
    return size;
}

bool CDMRLookup::periodicRead_thread(unsigned int sleep_time)
{
    m_sleep_time = sleep_time;
    pthread_t t1;
     //pthread_mutex_init(&lock_x, NULL);
    if (!::pthread_create(&t1, NULL, &readfile_thread, this)) {
      ::pthread_detach(t1);
      return true;
    } else {
	LogWarning("Could not spawn thread to load DMR IDs");
	return false;
    }
    
}
#endif