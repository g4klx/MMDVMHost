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

#if !defined(USERDB_H)
#define USERDB_H

#include "UserDBentry.h"
#include "Mutex.h"

class CUserDB {
public:
	CUserDB();
	~CUserDB();

	bool lookup(unsigned int id, class CUserDBentry *entry);
	bool load(std::string const& filename);

private:
	bool makeindex(char* buf, std::unordered_map<std::string, int>& index);
	void parse(char* buf, std::unordered_map<std::string, int>& index);
	void toupper_string(char* str);
	char* tokenize(char* str, char** next);

	std::unordered_map<unsigned int, class CUserDBentry>    m_table;
	CMutex                                                  m_mutex;
};
	
#endif
