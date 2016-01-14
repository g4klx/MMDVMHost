/*
 *   Copyright (C) 2015 by Jonathan Naylor G4KLX
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

#if !defined(YSFECHO_H)
#define	YSFECHO_H

#include "RingBuffer.h"
#include "Timer.h"

class CYSFEcho {
public:
	CYSFEcho(unsigned int delay, unsigned int space);
	~CYSFEcho();

	unsigned int readData(unsigned char* data);

	bool writeData(const unsigned char* data, unsigned int length);

	bool hasData();

	void clock(unsigned int ms);

private:
	CRingBuffer<unsigned char> m_buffer;
	CTimer                     m_timer;
};

#endif
