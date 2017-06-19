/*
 *   Copyright (C) 2016 by Simon Rune G7RZU
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
 */

#include "DMRAccessControl.h"
#include "Log.h"

#include <algorithm>
#include <vector>
#include <cstring>

std::vector<unsigned int> CDMRAccessControl::m_blackList;
std::vector<unsigned int> CDMRAccessControl::m_whiteList;

std::vector<unsigned int> CDMRAccessControl::m_prefixes;

std::vector<unsigned int> CDMRAccessControl::m_Slot1TGWhiteList;
std::vector<unsigned int> CDMRAccessControl::m_Slot2TGWhiteList;

bool CDMRAccessControl::m_selfOnly = false;

unsigned int CDMRAccessControl::m_id = 0U;

void CDMRAccessControl::init(const std::vector<unsigned int>& blacklist, const std::vector<unsigned int>& whitelist, const std::vector<unsigned int>& Slot1TGWhiteList, const std::vector<unsigned int>& Slot2TGWhiteList, bool selfOnly, const std::vector<unsigned int>& prefixes, unsigned int id)
{
	m_Slot1TGWhiteList = Slot1TGWhiteList;
	m_Slot2TGWhiteList = Slot2TGWhiteList;
	m_blackList        = blacklist;
	m_whiteList        = whitelist;
	m_selfOnly         = selfOnly;
	m_prefixes         = prefixes;
	m_id               = id;
}
 
bool CDMRAccessControl::validateSrcId(unsigned int id)
{
	if (m_selfOnly)
		return id == m_id;

	if (std::find(m_blackList.begin(), m_blackList.end(), id) != m_blackList.end())
		return false;

	unsigned int prefix = id / 10000U;
	if (prefix == 0U || prefix > 999U)
		return false;

	if (!m_prefixes.empty()) {
		bool ret = std::find(m_prefixes.begin(), m_prefixes.end(), prefix) == m_prefixes.end();
		if (ret)
			return false;
	}

	if (!m_whiteList.empty())
		return std::find(m_whiteList.begin(), m_whiteList.end(), id) != m_whiteList.end();

	return true;
}

bool CDMRAccessControl::validateTGId(unsigned int slotNo, bool group, unsigned int id)
{
	if (!group)
		return true;

	// TG0 is never valid
	if (id == 0U)
		return false;
	
	if (slotNo == 1U) {
		if (m_Slot1TGWhiteList.empty())
			return true;

		return std::find(m_Slot1TGWhiteList.begin(), m_Slot1TGWhiteList.end(), id) != m_Slot1TGWhiteList.end();
	} else {
		if (m_Slot2TGWhiteList.empty())
			return true;

		return std::find(m_Slot2TGWhiteList.begin(), m_Slot2TGWhiteList.end(), id) != m_Slot2TGWhiteList.end();
	}
}
