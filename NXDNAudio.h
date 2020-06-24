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

#if !defined(NXDNAudio_H)
#define	NXDNAudio_H

class CNXDNAudio {
public:
	CNXDNAudio();
	~CNXDNAudio();

	void encode(const unsigned char* in, unsigned char* out) const;

	void decode(const unsigned char* in, unsigned char* out) const;

private:
	void encode(const unsigned char* in, unsigned char* out, unsigned int offset) const;

	void decode(const unsigned char* in, unsigned char* out, unsigned int offset) const;
};

#endif
