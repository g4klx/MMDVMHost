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

#ifndef	DMRLookup_H
#define	DMRLookup_H

#include "Thread.h"
#include "Mutex.h"

#include <string>
#include <unordered_map>

class CDMRLookup : public CThread {
public:
	CDMRLookup(const std::string& filename, unsigned int reloadTime);
	virtual ~CDMRLookup();

	bool read();

	virtual void entry();

	std::string find(unsigned int id);
	std::string findWithName(unsigned int id);

	bool exists(unsigned int id);

	void stop();

private:
	std::string                                   m_filename;
	unsigned int                                  m_reloadTime;
	std::unordered_map<unsigned int, std::string> m_table;
	CMutex                                        m_mutex;
	bool                                          m_stop;

	bool load();
};

#endif
