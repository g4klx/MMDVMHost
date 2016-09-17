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
#if !defined(DMRAccessControl_H)
#define	DMRAccessControl_H

#include <vector>
#include <ctime>
#include "DMRLC.h"

class DMRAccessControl {
public:
	static bool validateAccess (unsigned int src_id, unsigned int dst_id, unsigned int slot, bool network);

	static void init(const std::vector<unsigned int>& DstIdBlacklistSlot1RF, const std::vector<unsigned int>& DstIdWhitelistSlot1RF, const std::vector<unsigned int>& DstIdBlacklistSlot2RF, const std::vector<unsigned int>& DstIdWhitelistSlot2RF, const std::vector<unsigned int>& DstIdBlacklistSlot1NET, const std::vector<unsigned int>& DstIdWhitelistSlot1NET, const std::vector<unsigned int>& DstIdBlacklistSlot2NET, const std::vector<unsigned int>& DstIdWhitelistSlot2NET, const std::vector<unsigned int>& SrcIdBlacklist, bool selfOnly, const std::vector<unsigned int>& prefixes,unsigned int id,unsigned int callHang, bool TGRewrteSlot1, bool TGRewrteSlot2);
	
	static unsigned int DstIdRewrite(unsigned int id, unsigned int sid,unsigned int slot, bool network, CDMRLC* dmrLC);
	static void setOverEndTime();

private:
	static std::vector<unsigned int> m_dstBlackListSlot1RF;
	static std::vector<unsigned int> m_dstBlackListSlot2RF;
	static std::vector<unsigned int> m_dstWhiteListSlot1RF;
	static std::vector<unsigned int> m_dstWhiteListSlot2RF;

	static std::vector<unsigned int> m_dstBlackListSlot1NET;
	static std::vector<unsigned int> m_dstBlackListSlot2NET;
	static std::vector<unsigned int> m_dstWhiteListSlot1NET;
	static std::vector<unsigned int> m_dstWhiteListSlot2NET;

	static std::vector<unsigned int> m_SrcIdBlacklist;

	static std::vector<unsigned int> m_prefixes;
	
	static unsigned int m_callHang;

	static bool m_selfOnly;
	static unsigned int m_id;

	static bool DstIdBlacklist(unsigned int did,unsigned int slot, bool network);
	static bool DstIdWhitelist(unsigned int did,unsigned int slot,bool gt4k, bool network);

	static bool validateSrcId(unsigned int id);
	
	static std::time_t m_time;
	
	static unsigned int m_dstRewriteID;
	static unsigned int m_SrcID;
	
	static bool m_TGRewriteSlot1;
	static bool m_TGRewriteSlot2;
	
	

};

#endif
