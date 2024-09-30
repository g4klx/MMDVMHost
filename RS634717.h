/*
*   Copyright (C) 2016,2024 by Jonathan Naylor G4KLX
*	Copyright (C) 2018,2023 by Bryan Biedenkapp <gatekeep@gmail.com> N2PLL
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

#if !defined(RS634717_H)
#define	RS634717_H

class CRS634717
{
public:
	CRS634717();
	~CRS634717();

	bool decode241213(unsigned char* data);
	bool decode24169(unsigned char* data);
	bool decode362017(unsigned char* data);

	void encode241213(unsigned char* data);
	void encode24169(unsigned char* data);
	void encode362017(unsigned char* data);

private:
	unsigned char gf6Mult(unsigned char a, unsigned char b) const;
};

#endif
