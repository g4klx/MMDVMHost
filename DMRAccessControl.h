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
#if !defined(DMRAccessControl_H)
#define	DMRAccessControl_H

#include <vector>

class CDMRAccessControl {
public:
	static bool validateSrcId(unsigned int id);

	static bool validateTGId(unsigned int slotNo, bool group, unsigned int id);

	static void init(const std::vector<unsigned int>& blacklist, const std::vector<unsigned int>& whitelist, const std::vector<unsigned int>& slot1TGWhitelist, const std::vector<unsigned int>& slot2TGWhitelist, bool selfOnly, const std::vector<unsigned int>& prefixes, unsigned int id);
	
private:
	static std::vector<unsigned int> m_blackList;
	static std::vector<unsigned int> m_whiteList;

	static std::vector<unsigned int> m_prefixes;

	static std::vector<unsigned int> m_slot1TGWhiteList;
	static std::vector<unsigned int> m_slot2TGWhiteList;

	static bool m_selfOnly;
	static unsigned int m_id;
};

#endif
