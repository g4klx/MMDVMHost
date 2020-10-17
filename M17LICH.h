/*
 *   Copyright (C) 2020 by Jonathan Naylor G4KLX
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
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if !defined(M17LICH_H)
#define  M17LICH_H

#include <string>

class CM17LICH {
public:
	CM17LICH(const CM17LICH& lich);
	CM17LICH();
	~CM17LICH();

	void getNetworkData(unsigned char* data);
	void setNetworkData(const unsigned char* data);

	std::string getSource() const;
	void setSource(const std::string& callsign);

	std::string getDest() const;
	void setDest(const std::string& callsign);

	unsigned char getDataType() const;
	void setDataType(unsigned char type);

	void reset();
	bool isValid() const;

	void getLinkSetup(unsigned char* data) const;
	void setLinkSetup(const unsigned char* data) const;

	void getFragment(unsigned char* data, unsigned short fn);
	void setFragment(const unsigned char* data, unsigned short fn);

	CM17LICH& operator=(const CM17LICH& lich);

private:
	unsigned char* m_lich;
};

#endif
