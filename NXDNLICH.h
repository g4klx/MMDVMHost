/*
 *   Copyright (C) 2018 by Jonathan Naylor G4KLX
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

#if !defined(NXDNLICH_H)
#define  NXDNLICH_H

class CNXDNLICH {
public:
	CNXDNLICH(const CNXDNLICH& lich);
	CNXDNLICH();
	~CNXDNLICH();

	bool decode(const unsigned char* bytes);

	void encode(unsigned char* bytes);

	unsigned char getRFCT() const;
	unsigned char getFCT() const;
	unsigned char getOption() const;
	unsigned char getDirection() const;
	unsigned char getRaw() const;
	
	void setRFCT(unsigned char rfct);
	void setFCT(unsigned char usc);
	void setOption(unsigned char option);
	void setDirection(unsigned char direction);
	void setRaw(unsigned char lich);

	CNXDNLICH& operator=(const CNXDNLICH& lich);

private:
	unsigned char* m_lich;

	bool getParity() const;
};

#endif
