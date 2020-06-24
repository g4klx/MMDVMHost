/*
 *   Copyright (C) 2015,2016,2017 by Jonathan Naylor G4KLX
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

#ifndef DMRDataHeader_H
#define DMRDataHeader_H

class CDMRDataHeader
{
public:
	CDMRDataHeader();
	~CDMRDataHeader();

	bool put(const unsigned char* bytes);

	void get(unsigned char* bytes) const;

	bool          getGI() const;

	unsigned int  getSrcId() const;
	unsigned int  getDstId() const;

	unsigned int  getBlocks() const;

	CDMRDataHeader& operator=(const CDMRDataHeader& header);

private:
	unsigned char* m_data;
	bool           m_GI;
	bool           m_A;
	unsigned int   m_srcId;
	unsigned int   m_dstId;
	unsigned int   m_blocks;
	bool           m_F;
	bool           m_S;
	unsigned char  m_Ns;
};

#endif

