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

class DMRAccessControl {
public:
	static bool DstIdBlacklist(unsigned int did,unsigned int slot);
	static bool DstIdWhitelist(unsigned int did,unsigned int slot,bool gt4k);
	
 	static void init(const std::vector<unsigned int>& DstIdBlacklistSlot1, const std::vector<unsigned int>& DstIdWhitelistSlot1, const std::vector<unsigned int>& DstIdBlacklistSlot2, const std::vector<unsigned int>& DstIdWhitelistSlot2);


	
private:
	static std::vector<unsigned int> m_dstBlackListSlot1;
	static std::vector<unsigned int> m_dstBlackListSlot2;
	static std::vector<unsigned int> m_dstWhiteListSlot1;
	static std::vector<unsigned int> m_dstWhiteListSlot2;


	
};

#endif