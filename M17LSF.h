/*
 *   Copyright (C) 2020,2021 by Jonathan Naylor G4KLX
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

#if !defined(M17LSF_H)
#define  M17LSF_H

#include <string>

class CM17LSF {
public:
	CM17LSF(const CM17LSF& lsf);
	CM17LSF();
	~CM17LSF();

	void getNetwork(unsigned char* data) const;
	void setNetwork(const unsigned char* data);

	std::string getSource() const;
	void setSource(const std::string& callsign);

	std::string getDest() const;
	void setDest(const std::string& callsign);

	unsigned char getPacketStream() const;
	void setPacketStream(unsigned char ps);

	unsigned char getDataType() const;
	void setDataType(unsigned char type);

	unsigned char getEncryptionType() const;
	void setEncryptionType(unsigned char type);

	unsigned char getEncryptionSubType() const;
	void setEncryptionSubType(unsigned char type);

	unsigned char getCAN() const;
	void setCAN(unsigned char can);

	void getMeta(unsigned char* data) const;
	void setMeta(const unsigned char* data);

	void reset();
	bool isValid() const;

	void getLinkSetup(unsigned char* data) const;
	void setLinkSetup(const unsigned char* data);

	void getFragment(unsigned char* data, unsigned int n) const;
	void setFragment(const unsigned char* data, unsigned int n);

	CM17LSF& operator=(const CM17LSF& lsf);

private:
	unsigned char* m_lsf;
	bool           m_valid;
};

#endif
