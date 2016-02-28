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

#if !defined(YSFPayload_H)
#define	YSFPayload_H

class CYSFPayload {
public:
	CYSFPayload();
	~CYSFPayload();

	bool decode(const unsigned char* bytes, unsigned char fi, unsigned char fn, unsigned char ft, unsigned char dt);

	void encode(unsigned char* bytes);

private:
	unsigned char* m_data;

	bool decodeHeader();
	bool decodeVDMode1(unsigned char fn, unsigned char ft);
	bool decodeVDMode2(unsigned char fn, unsigned char ft);
	bool decodeDataFRMode(unsigned char fn, unsigned char ft);
};

#endif
