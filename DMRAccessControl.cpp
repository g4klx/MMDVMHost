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

#include <algorithm>
#include <vector>

std::vector<unsigned int> DMRAccessControl::m_dstBlackListSlot1;
std::vector<unsigned int> DMRAccessControl::m_dstBlackListSlot2;
std::vector<unsigned int> DMRAccessControl::m_dstWhiteListSlot1;
std::vector<unsigned int> DMRAccessControl::m_dstWhiteListSlot2;

std::vector<unsigned int> DMRAccessControl::m_SrcIdBlacklist;

std::vector<unsigned int> DMRAccessControl::m_prefixes;
bool DMRAccessControl::m_selfOnly;
unsigned int DMRAccessControl::m_id;

void DMRAccessControl::init(const std::vector<unsigned int>& DstIdBlacklistSlot1, const std::vector<unsigned int>& DstIdWhitelistSlot1, const std::vector<unsigned int>& DstIdBlacklistSlot2, const std::vector<unsigned int>& DstIdWhitelistSlot2, const std::vector<unsigned int>& SrcIdBlacklist, bool selfOnly, const std::vector<unsigned int>& prefixes,unsigned int id)
{

	m_dstBlackListSlot1 = DstIdBlacklistSlot1;
	m_dstWhiteListSlot1 = DstIdWhitelistSlot1;
	m_dstBlackListSlot2 = DstIdBlacklistSlot2;
	m_dstWhiteListSlot2 = DstIdWhitelistSlot2;
}
 
bool DMRAccessControl::DstIdBlacklist(unsigned int did, unsigned int slot)
{
	if (slot == 1U) {
	    if (std::find(m_dstBlackListSlot1.begin(), m_dstBlackListSlot1.end(), did) != m_dstBlackListSlot1.end())
			  return true;
	} else {
	    if (std::find(m_dstBlackListSlot2.begin(), m_dstBlackListSlot2.end(), did) != m_dstBlackListSlot2.end())
			  return true;
	}

	return false;
}

bool DMRAccessControl::DstIdWhitelist(unsigned int did, unsigned int slot, bool gt4k)
{
	if (slot == 1U) {
		if (m_dstWhiteListSlot1.size() == 0U)
			return true;

		// No reflectors on slot1, so we only allow all IDs over 99999 unless specifically whitelisted.
		//Allow traffic to TG0 as I think this is a special case - need to confirm
		if (gt4k) {
			if (std::find(m_dstWhiteListSlot1.begin(), m_dstWhiteListSlot1.end(), did) != m_dstWhiteListSlot1.end() || did >= 99999U || did == 0)
				return true;
		} else {
			if (std::find(m_dstWhiteListSlot1.begin(), m_dstWhiteListSlot1.end(), did) != m_dstWhiteListSlot1.end() || did == 0)
				return true;
		}
	} else {
		if (m_dstWhiteListSlot2.size() == 0U)
			return true;

		//On slot2 we allow reflector control IDs, but not secondary TG IDs unless specifically listed. Also allow echo.
		if (gt4k) {
			if (std::find(m_dstWhiteListSlot2.begin(), m_dstWhiteListSlot2.end(), did) != m_dstWhiteListSlot2.end() || did == 0)
			    return true;
			
			//if dstId in secondary TG range or whitelist  	
			else if (did >= 4000) {
				if (did > 5000U && did < 10000U)
					return false; 
				else
					return true;
			}
		} else { 
			if (std::find(m_dstWhiteListSlot2.begin(), m_dstWhiteListSlot2.end(), did) != m_dstWhiteListSlot2.end())
				return true;
		}
	}

	return false;
}

bool DMRAccessControl::validateSrcId(unsigned int id)
{
	if (m_selfOnly) {
		return id == m_id;
	} else {
		if (std::find(m_SrcIdBlacklist.begin(), m_SrcIdBlacklist.end(), id) != m_SrcIdBlacklist.end())
			return false;

		unsigned int prefix = id / 10000U;
		if (prefix == 0U || prefix > 999U)
			return false;

		if (m_prefixes.size() == 0U)
			return true;

		return std::find(m_prefixes.begin(), m_prefixes.end(), prefix) != m_prefixes.end();
	}
}
