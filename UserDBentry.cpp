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
#include "UserDBentry.h"
#include <algorithm>

CUserDBentry::CUserDBentry() :
m_db()
{
}

CUserDBentry::~CUserDBentry()
{
}

const std::vector<std::string> CUserDBentry::keyList {
	keyRADIO_ID, keyCALLSIGN, keyFIRST_NAME, keyLAST_NAME,
	keyCITY, keySTATE, keyCOUNTRY,
};

bool CUserDBentry::isValidKey(const std::string key)
{
	auto it = std::find(keyList.begin(), keyList.end(), key);
	return it != keyList.end();
}

void CUserDBentry::set(const std::string key, const std::string value)
{
	if (!value.empty() && isValidKey(key))
		m_db[key] = value;
}

const std::string CUserDBentry::get(const std::string key) const
{
	try {
		return m_db.at(key);
	} catch (...) {
		return "";
	}
}

void CUserDBentry::clear(void)
{
	m_db.clear();
}
