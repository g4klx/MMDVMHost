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
#include <string.h>


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
unsigned int DMRAccessControl::m_SrcID = 0U;

CDMRLC* DMRAccessControl::m_lastdmrLC;

std::time_t DMRAccessControl::m_time;

int DMRAccessControl::m_callHang;

bool DMRAccessControl::m_TGRewriteSlot1;
bool DMRAccessControl::m_TGRewriteSlot2;
bool DMRAccessControl::m_BMAutoRewrite;
bool DMRAccessControl::m_BMRewriteReflectorVoicePrompts;

void DMRAccessControl::init(const std::vector<unsigned int>& DstIdBlacklistSlot1RF, const std::vector<unsigned int>& DstIdWhitelistSlot1RF, const std::vector<unsigned int>& DstIdBlacklistSlot2RF, const std::vector<unsigned int>& DstIdWhitelistSlot2RF, const std::vector<unsigned int>& DstIdBlacklistSlot1NET, const std::vector<unsigned int>& DstIdWhitelistSlot1NET, const std::vector<unsigned int>& DstIdBlacklistSlot2NET, const std::vector<unsigned int>& DstIdWhitelistSlot2NET, const std::vector<unsigned int>& SrcIdBlacklist, bool selfOnly, const std::vector<unsigned int>& prefixes,unsigned int id,unsigned int callHang,bool TGRewriteSlot1, bool TGRewriteSlot2, bool BMAutoRewrite, bool BMRewriteReflectorVoicePrompts)
{
	m_dstBlackListSlot1RF  = DstIdBlacklistSlot1RF;
	m_dstWhiteListSlot1RF  = DstIdWhitelistSlot1RF;
	m_dstBlackListSlot2RF  = DstIdBlacklistSlot2RF;
	m_dstWhiteListSlot2RF  = DstIdWhitelistSlot2RF;
	m_dstBlackListSlot1NET = DstIdBlacklistSlot1NET;
	m_dstWhiteListSlot1NET = DstIdWhitelistSlot1NET;
	m_dstBlackListSlot2NET = DstIdBlacklistSlot2NET;
	m_dstWhiteListSlot2NET = DstIdWhitelistSlot2NET;
	m_callHang = callHang;
	m_TGRewriteSlot1 = TGRewriteSlot1;
	m_TGRewriteSlot2 = TGRewriteSlot2;
	m_BMAutoRewrite	= BMAutoRewrite;
	m_BMRewriteReflectorVoicePrompts = BMRewriteReflectorVoicePrompts;
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

unsigned int DMRAccessControl::DstIdRewrite (unsigned int did, unsigned int sid, unsigned int slot, bool network, CDMRLC* dmrLC) 
{
  
  if (slot == 1U && m_TGRewriteSlot1 == false)
    return 0U;
  
  if (slot == 2U && m_TGRewriteSlot2 == false)
    return 0U;
    
   
  std::time_t currenttime = std::time(nullptr);
  
  if (network) {
	m_dstRewriteID = did;
	m_SrcID = sid;
	//not needed at present - for direct dial, which requires change at master end.
	//memcpy(&m_lastdmrLC, &dmrLC, sizeof(dmrLC));
	if (m_BMAutoRewrite && (did < 4000U || did > 5000U) && did > 0U && did != 9U && dmrLC->getFLCO() == FLCO_GROUP ) {
	  LogMessage("DMR Slot %u, Rewrite DST ID (TG) of of inbound network traffic from %u to 9",slot,did);
	  return 9U;
	// rewrite incoming BM voice prompts to TG 9
	} else if (m_BMRewriteReflectorVoicePrompts && (sid >= 4000U && sid <= 5000U) && dmrLC->getFLCO() == FLCO_USER_USER)  {
	    dmrLC->setFLCO(FLCO_GROUP);
	    LogMessage("DMR Slot %u, Rewrite inbound private call to %u to Group Call on TG 9 (BM reflector voice prompt)",slot,did);
	    return 9U;	 
	} else {
	    return 0U;
	}
  } else if (m_BMAutoRewrite && did == 9U && m_dstRewriteID != 9U && m_dstRewriteID != 0U && (m_time + m_callHang) > currenttime && dmrLC->getFLCO() == FLCO_GROUP ) {
	      LogMessage("DMR Slot %u, Rewrite DST ID (TG) of outbound network traffic from %u to %u (return traffic during CallHang)",slot,did,m_dstRewriteID);
	      return(m_dstRewriteID);

  } else if (m_BMAutoRewrite && (did < 4000 || did > 5000) && did > 0 && did !=9 && did < 99999 && dmrLC->getFLCO() == FLCO_USER_USER) {
	      m_dstRewriteID = did;
	      dmrLC->setFLCO(FLCO_GROUP);
	      LogMessage("DMR Slot %u, Rewrite outbound private call to %u Group Call (Connect talkgroup by private call)",slot,did);
	      return did;
  } else if (m_BMAutoRewrite && (did < 4000 || did > 5000) && did > 0 && did !=9 && did > 99999) {
      m_dstRewriteID = did;
  } 
  return 0U;
}


void DMRAccessControl::setOverEndTime() 
{
  m_time = std::time(nullptr);
} 
