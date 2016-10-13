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
	static bool validateAccess(unsigned int srcId, unsigned int dstId, unsigned int slot, bool network);

	static void init(const std::vector<unsigned int>& dstIdBlacklistSlot1RF, const std::vector<unsigned int>& dstIdWhitelistSlot1RF, const std::vector<unsigned int>& dstIdBlacklistSlot2RF, const std::vector<unsigned int>& dstIdWhitelistSlot2RF, const std::vector<unsigned int>& dstIdBlacklistSlot1NET, const std::vector<unsigned int>& dstIdWhitelistSlot1NET, const std::vector<unsigned int>& dstIdBlacklistSlot2NET, const std::vector<unsigned int>& dstIdWhitelistSlot2NET, const std::vector<unsigned int>& srcIdBlacklist, bool selfOnly, const std::vector<unsigned int>& prefixes,unsigned int id,unsigned int callHang, bool tgRewrteSlot1, bool tgRewrteSlot2, bool m_bmAutoRewrite, bool m_bmRewriteReflectorVoicePrompts);
	
	static unsigned int dstIdRewrite(unsigned int id, unsigned int sid, unsigned int slot, bool network, CDMRLC* dmrLC);

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

	static std::vector<unsigned int> m_srcIdBlacklist;

	static std::vector<unsigned int> m_prefixes;
	
	static int m_callHang;

	static bool m_selfOnly;
	static unsigned int m_id;

	static bool dstIdBlacklist(unsigned int did, unsigned int slot, bool network);
	static bool dstIdWhitelist(unsigned int did, unsigned int slot, bool gt4k, bool network);

	static bool validateSrcId(unsigned int id);

	static time_t m_time;

	static unsigned int m_dstRewriteID;
	static unsigned int m_srcID;

	static bool m_tgRewriteSlot1;
	static bool m_tgRewriteSlot2;
	static bool m_bmAutoRewrite;
	static bool m_bmRewriteReflectorVoicePrompts;

	static CDMRLC* m_lastdmrLC;
};

#endif
