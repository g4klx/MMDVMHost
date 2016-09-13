/*
*   Copyright (C) 2016 by Jonathan Naylor G4KLX
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

#if !defined(P25Data_H)
#define  P25Data_H

class CP25Data {
public:
	CP25Data();
	~CP25Data();

	void processHeader(unsigned char* data);

	void processLDU1(unsigned char* data);

	void processLDU2(unsigned char* data);

	void processTerminator(unsigned char* data);

	unsigned int getSource() const;

	bool getGroup() const;
	unsigned int getDest() const;

	void reset();

private:
	unsigned int m_source;
	bool         m_group;
	unsigned int m_dest;

	void decodeLDUHamming(const unsigned char* raw, unsigned char* data);
	void encodeLDUHamming(unsigned char* data, const unsigned char* raw);
};

#endif
