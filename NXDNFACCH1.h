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

#if !defined(NXDNFACCH1_H)
#define	NXDNFACCH1_H

class CNXDNFACCH1 {
public:
	CNXDNFACCH1(const CNXDNFACCH1& facch);
	CNXDNFACCH1();
	~CNXDNFACCH1();

	bool decode(const unsigned char* data, unsigned int offset);

	void encode(unsigned char* data, unsigned int offset) const;

	void getData(unsigned char* data) const;
	void getRaw(unsigned char* data) const;

	void setData(const unsigned char* data);
	void setRaw(const unsigned char* data);

	CNXDNFACCH1& operator=(const CNXDNFACCH1& facch);

private:
	unsigned char* m_data;
};

#endif
