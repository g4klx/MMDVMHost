/*
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
 */

#include "DMRAccessControl.h"
#include "Log.h"

#include <algorithm>
#include <vector>
#include <cstring>

std::vector<unsigned int> CDMRAccessControl::m_blackList;
std::vector<unsigned int> CDMRAccessControl::m_whiteList;

std::vector<unsigned int> CDMRAccessControl::m_prefixes;

bool CDMRAccessControl::m_selfOnly = false;

unsigned int CDMRAccessControl::m_id = 0U;

void CDMRAccessControl::init(const std::vector<unsigned int>& blacklist, const std::vector<unsigned int>& whitelist, bool selfOnly, const std::vector<unsigned int>& prefixes, unsigned int id)
{
	m_blackList = blacklist;
	m_whiteList = whitelist;
}
 
bool CDMRAccessControl::validateId(unsigned int id)
{
	if (m_selfOnly) {
		return id == m_id;
	} else {
		if (std::find(m_blackList.begin(), m_blackList.end(), id) != m_blackList.end())
			return false;

		unsigned int prefix = id / 10000U;
		if (prefix == 0U || prefix > 999U)
			return false;

		if (!m_prefixes.empty()) {
			bool ret = std::find(m_prefixes.begin(), m_prefixes.end(), prefix) == m_prefixes.end();
			if (!ret)
				return false;
		}

		if (!m_whiteList.empty())
			return std::find(m_whiteList.begin(), m_whiteList.end(), id) != m_whiteList.end();

		return true;
	}
}
