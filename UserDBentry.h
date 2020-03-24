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

#if !defined(USERDBENTRY_H)
#define USERDBENTRY_H

#include <string>
#include <unordered_map>
#include <vector>

#define keyRADIO_ID	"RADIO_ID"
#define keyCALLSIGN	"CALLSIGN"
#define keyFIRST_NAME	"FIRST_NAME"
#define keyLAST_NAME	"LAST_NAME"
#define keyCITY		"CITY"
#define keySTATE	"STATE"
#define keyCOUNTRY	"COUNTRY"

class CUserDBentry {
public:
	CUserDBentry();
	~CUserDBentry();

	static const std::vector<std::string> keyList;
	static bool isValidKey(const std::string key);

	void set(const std::string key, const std::string value);
	const std::string get(const std::string key) const;
	void clear(void);

private:
	std::unordered_map<std::string, std::string>    m_db;
};

#endif
