/*
*   Copyright (C) 2016,2021 by Jonathan Naylor G4KLX
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

#ifndef ModemPort_H
#define ModemPort_H

class IModemPort {
public:
	virtual ~IModemPort() = 0;

	virtual bool open() = 0;

	virtual int read(unsigned char* buffer, unsigned int length) = 0;

	virtual int write(const unsigned char* buffer, unsigned int length) = 0;

	virtual void close() = 0;
#if defined(__APPLE__)
	virtual int setNonblock(bool nonblock) = 0;
#endif

private:
};

#endif
