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
#include <ctime>

std::vector<unsigned int> CDMRAccessControl::m_dstBlackListSlot1RF;
std::vector<unsigned int> CDMRAccessControl::m_dstBlackListSlot2RF;
std::vector<unsigned int> CDMRAccessControl::m_dstWhiteListSlot1RF;
std::vector<unsigned int> CDMRAccessControl::m_dstWhiteListSlot2RF;

std::vector<unsigned int> CDMRAccessControl::m_dstBlackListSlot1NET;
std::vector<unsigned int> CDMRAccessControl::m_dstBlackListSlot2NET;
std::vector<unsigned int> CDMRAccessControl::m_dstWhiteListSlot1NET;
std::vector<unsigned int> CDMRAccessControl::m_dstWhiteListSlot2NET;

std::vector<unsigned int> CDMRAccessControl::m_srcIdBlacklist;

std::vector<unsigned int> CDMRAccessControl::m_prefixes;

bool CDMRAccessControl::m_selfOnly = false;

unsigned int CDMRAccessControl::m_id = 0U;

unsigned int CDMRAccessControl::m_dstRewriteID = 0U;
unsigned int CDMRAccessControl::m_srcID = 0U;

std::time_t CDMRAccessControl::m_time;

unsigned int CDMRAccessControl::m_callHang = 0U;

bool CDMRAccessControl::m_tgRewriteSlot1 = false;
bool CDMRAccessControl::m_tgRewriteSlot2 = false;


void CDMRAccessControl::init(const std::vector<unsigned int>& dstIdBlacklistSlot1RF, const std::vector<unsigned int>& dstIdWhitelistSlot1RF, const std::vector<unsigned int>& dstIdBlacklistSlot2RF, const std::vector<unsigned int>& dstIdWhitelistSlot2RF, const std::vector<unsigned int>& dstIdBlacklistSlot1NET, const std::vector<unsigned int>& dstIdWhitelistSlot1NET, const std::vector<unsigned int>& dstIdBlacklistSlot2NET, const std::vector<unsigned int>& dstIdWhitelistSlot2NET, const std::vector<unsigned int>& srcIdBlacklist, bool selfOnly, const std::vector<unsigned int>& prefixes, unsigned int id, unsigned int callHang, bool tgRewriteSlot1, bool tgRewriteSlot2)
{
	m_dstBlackListSlot1RF  = dstIdBlacklistSlot1RF;
	m_dstWhiteListSlot1RF  = dstIdWhitelistSlot1RF;
	m_dstBlackListSlot2RF  = dstIdBlacklistSlot2RF;
	m_dstWhiteListSlot2RF  = dstIdWhitelistSlot2RF;
	m_dstBlackListSlot1NET = dstIdBlacklistSlot1NET;
	m_dstWhiteListSlot1NET = dstIdWhitelistSlot1NET;
	m_dstBlackListSlot2NET = dstIdBlacklistSlot2NET;
	m_dstWhiteListSlot2NET = dstIdWhitelistSlot2NET;
	m_callHang             = callHang;
	m_tgRewriteSlot1       = tgRewriteSlot1;
	m_tgRewriteSlot2       = tgRewriteSlot2;
}
 
bool CDMRAccessControl::dstIdBlacklist(unsigned int did, unsigned int slot, bool network)
{
	static std::vector<unsigned int> blacklist;
	  
	if (slot == 1U) {
	    if (network)
			blacklist = m_dstBlackListSlot1NET;
	    else
			blacklist = m_dstBlackListSlot1RF;
	} else {
	    if (network)
			blacklist = m_dstBlackListSlot2NET;
	    else
			blacklist = m_dstBlackListSlot2RF;
	}

	return std::find(blacklist.begin(), blacklist.end(), did) != blacklist.end();
}

bool CDMRAccessControl::dstIdWhitelist(unsigned int did, unsigned int slot, bool gt4k, bool network)
{
	if (network) {
	    if (slot == 1U) {
		    if (m_dstWhiteListSlot1NET.size() == 0U)
			    return true;

		    // No reflectors on slot1, so we only allow all IDs over 99999 unless specifically whitelisted.
		    // Allow traffic to TG0 as I think this is a special case - need to confirm
		    if (gt4k) {
			    if (std::find(m_dstWhiteListSlot1NET.begin(), m_dstWhiteListSlot1NET.end(), did) != m_dstWhiteListSlot1NET.end() || did >= 99999U || did == 0)
				    return true;
		    } else {
			    if (std::find(m_dstWhiteListSlot1NET.begin(), m_dstWhiteListSlot1NET.end(), did) != m_dstWhiteListSlot1NET.end() || did == 0)
				    return true;
		    }
	    } else {
		    if (m_dstWhiteListSlot2NET.size() == 0U)
			    return true;

		    // On slot2 we allow reflector control IDs, but not secondary TG IDs unless specifically listed. Also allow echo.
		    if (gt4k) {
			    if (std::find(m_dstWhiteListSlot2NET.begin(), m_dstWhiteListSlot2NET.end(), did) != m_dstWhiteListSlot2NET.end() || did == 0)
					return true;

				// If dstId in secondary TG range or whitelist
			    else if (did >= 4000U) {
				    if (did > 5000U && did < 10000U)
					    return false; 
				    else
					    return true;
			    }
		    } else {
			    if (std::find(m_dstWhiteListSlot2NET.begin(), m_dstWhiteListSlot2NET.end(), did) != m_dstWhiteListSlot2NET.end())
				    return true;
		    }
	    }

	    return false;
	} else {
	    if (slot == 1U) {
		    if (m_dstWhiteListSlot1RF.size() == 0U)
			    return true;

		    // No reflectors on slot1, so we only allow all IDs over 99999 unless specifically whitelisted.
		    // Allow traffic to TG0 as I think this is a special case - need to confirm
		    if (gt4k) {
			    if (std::find(m_dstWhiteListSlot1RF.begin(), m_dstWhiteListSlot1RF.end(), did) != m_dstWhiteListSlot1RF.end() || did >= 99999U || did == 0)
				    return true;
		    } else {
			    if (std::find(m_dstWhiteListSlot1RF.begin(), m_dstWhiteListSlot1RF.end(), did) != m_dstWhiteListSlot1RF.end() || did == 0)
				    return true;
		    }
	    } else {
		    if (m_dstWhiteListSlot2RF.size() == 0U)
			    return true;

		    // On slot2 we allow reflector control IDs, but not secondary TG IDs unless specifically listed. Also allow echo.
		    if (gt4k) {
				if (std::find(m_dstWhiteListSlot2RF.begin(), m_dstWhiteListSlot2RF.end(), did) != m_dstWhiteListSlot2RF.end() || did == 0)
					return true;
			    // If dstId in secondary TG range or whitelist
			    else if (did >= 4000U) {
				    if (did > 5000U && did < 10000U)
					    return false; 
				    else
					    return true;
			    }
		    } else {
			    if (std::find(m_dstWhiteListSlot2RF.begin(), m_dstWhiteListSlot2RF.end(), did) != m_dstWhiteListSlot2RF.end())
				    return true;
		    }
	    }

		return false;
	}
}

bool CDMRAccessControl::validateSrcId(unsigned int id)
{
	if (m_selfOnly) {
		return id == m_id;
	} else {
		if (std::find(m_srcIdBlacklist.begin(), m_srcIdBlacklist.end(), id) != m_srcIdBlacklist.end())
			return false;

		unsigned int prefix = id / 10000U;
		if (prefix == 0U || prefix > 999U)
			return false;

		if (m_prefixes.size() == 0U)
			return true;

		return std::find(m_prefixes.begin(), m_prefixes.end(), prefix) != m_prefixes.end();
	}
}

bool CDMRAccessControl::validateAccess (unsigned int srcId, unsigned int dstId, unsigned int slot, bool network) 
{
    // source ID validation is only applied to RF traffic 
    if (!network && !CDMRAccessControl::validateSrcId(srcId)) {
		LogMessage("DMR Slot %u, invalid access attempt from %u (blacklisted)", slot, srcId);
		return false;
    } else if (CDMRAccessControl::dstIdBlacklist(dstId, slot, network)) {
		LogMessage("DMR Slot %u, invalid access attempt to TG%u (TG blacklisted)", slot, dstId);
		return false;
    } else if (!CDMRAccessControl::dstIdWhitelist(dstId, slot, true, network)) {
		LogMessage("DMR Slot %u, invalid access attempt to TG%u (TG not in whitelist)", slot, dstId);
		return false;
	} else {
		return true;
	}
}

unsigned int CDMRAccessControl::dstIdRewrite(unsigned int did, unsigned int sid, unsigned int slot, bool network) 
{
    if (slot == 1U && !m_tgRewriteSlot1)
		return 0;
  
	if (slot == 2U && !m_tgRewriteSlot2)
		return 0;
   
	std::time_t currenttime = std::time(nullptr);
  
	if (network) {
		m_dstRewriteID = did;
		m_srcID        = sid;

		if (did < 4000U && did > 0 && did != 9U) {
			LogMessage("DMR Slot %u, Rewrite DST ID (TG) of of inbound network traffic from %u to 9", slot, did);
			return 9U;
		} else {
			return 0U;
		}
	} else if (did == 9U && m_dstRewriteID != 9U && m_dstRewriteID != 0U && (m_time + m_callHang) > currenttime) {
		LogMessage("DMR Slot %u, Rewrite DST ID (TG) of outbound network traffic from %u to %u (return traffic during CallHang)",slot,did,m_dstRewriteID);
		return m_dstRewriteID;
	}  else if (did < 4000U && did > 0U && did != 9U) {
		m_dstRewriteID = did;
	} 

	return 0U;
}

void CDMRAccessControl::setOverEndTime() 
{
	m_time = std::time(nullptr);
} 
