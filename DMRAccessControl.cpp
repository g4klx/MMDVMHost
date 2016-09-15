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

std::vector<unsigned int> DMRAccessControl::m_dstBlackListSlot1RF;
std::vector<unsigned int> DMRAccessControl::m_dstBlackListSlot2RF;
std::vector<unsigned int> DMRAccessControl::m_dstWhiteListSlot1RF;
std::vector<unsigned int> DMRAccessControl::m_dstWhiteListSlot2RF;

std::vector<unsigned int> DMRAccessControl::m_dstBlackListSlot1NET;
std::vector<unsigned int> DMRAccessControl::m_dstBlackListSlot2NET;
std::vector<unsigned int> DMRAccessControl::m_dstWhiteListSlot1NET;
std::vector<unsigned int> DMRAccessControl::m_dstWhiteListSlot2NET;

std::vector<unsigned int> DMRAccessControl::m_SrcIdBlacklist;

std::vector<unsigned int> DMRAccessControl::m_prefixes;

bool DMRAccessControl::m_selfOnly = false;

unsigned int DMRAccessControl::m_id = 0U;

unsigned int DMRAccessControl::m_dstRewriteID = 0U;

std::time_t DMRAccessControl::m_time;

void DMRAccessControl::init(const std::vector<unsigned int>& DstIdBlacklistSlot1RF, const std::vector<unsigned int>& DstIdWhitelistSlot1RF, const std::vector<unsigned int>& DstIdBlacklistSlot2RF, const std::vector<unsigned int>& DstIdWhitelistSlot2RF, const std::vector<unsigned int>& DstIdBlacklistSlot1NET, const std::vector<unsigned int>& DstIdWhitelistSlot1NET, const std::vector<unsigned int>& DstIdBlacklistSlot2NET, const std::vector<unsigned int>& DstIdWhitelistSlot2NET, const std::vector<unsigned int>& SrcIdBlacklist, bool selfOnly, const std::vector<unsigned int>& prefixes,unsigned int id)
{
	m_dstBlackListSlot1RF  = DstIdBlacklistSlot1RF;
	m_dstWhiteListSlot1RF  = DstIdWhitelistSlot1RF;
	m_dstBlackListSlot2RF  = DstIdBlacklistSlot2RF;
	m_dstWhiteListSlot2RF  = DstIdWhitelistSlot2RF;
	m_dstBlackListSlot1NET = DstIdBlacklistSlot1NET;
	m_dstWhiteListSlot1NET = DstIdWhitelistSlot1NET;
	m_dstBlackListSlot2NET = DstIdBlacklistSlot2NET;
	m_dstWhiteListSlot2NET = DstIdWhitelistSlot2NET;
}
 
bool DMRAccessControl::DstIdBlacklist(unsigned int did, unsigned int slot, bool network)
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

bool DMRAccessControl::DstIdWhitelist(unsigned int did, unsigned int slot, bool gt4k, bool network)
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
			    else if (did >= 4000) {
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
			    else if (did >= 4000) {
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

bool DMRAccessControl::validateAccess (unsigned int src_id, unsigned int dst_id, unsigned int slot, bool network) 
{
    // source ID validation is only applied to RF traffic 
    if (!network && !DMRAccessControl::validateSrcId(src_id)) {
		LogMessage("DMR Slot %u, invalid access attempt from %u (blacklisted)", slot, src_id);
		return false;
    } else if (DMRAccessControl::DstIdBlacklist(dst_id, slot, network)) {
		LogMessage("DMR Slot %u, invalid access attempt to TG%u (TG blacklisted)", slot, dst_id);
		return false;
    } else if (!DMRAccessControl::DstIdWhitelist(dst_id, slot, true, network)) {
		LogMessage("DMR Slot %u, invalid access attempt to TG%u (TG not in whitelist)", slot, dst_id);
		return false;
	} else {
		return true;
	}
}

unsigned int DMRAccessControl::DstIdRewrite (unsigned int id, bool network) 
{
  // record current time 
  std::time_t currenttime = std::time(nullptr);
  
   // if the traffic is from the network
  if (network) {
	m_dstRewriteID = id;
	// record current time
	m_time = std::time(nullptr);
	// if the ID is a talkgroup, log and rewrite to TG9 
	if (id < 4000 && id > 0) {
	  LogMessage("Rewrite DST ID (TG) of of inbound network traffic from %u to 9", id);
	  return 9;
	} else {
	    return 0;
	}
  } else {
	// if less than 30 seconds has passed since we last saw traffic from network 
	if (m_dstRewriteID == id && (m_time + 30) > currenttime) {
	      LogMessage("Inbound DST ID (TG) rewrite seen in last 30 seconds");
	      LogMessage("Rewrite DST ID (TG) of outbound network traffic from 9 to %u", id);
	      m_time = std::time(nullptr);
	      return(id);
	} else {
	    return(0);
	}
  }

}	
