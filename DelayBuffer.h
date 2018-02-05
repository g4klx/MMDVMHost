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

#if !defined(DELAYBUFFER_H)
#define	DELAYBUFFER_H

#include "RingBuffer.h"
#include "StopWatch.h"
#include "Defines.h"
#include "Timer.h"

#include <string>

class CDelayBuffer {
public:
	CDelayBuffer(const std::string& name, unsigned int blockSize, unsigned int blockTime, unsigned int jitterTime, bool debug);
	~CDelayBuffer();

	bool addData(const unsigned char* data, unsigned int length);

	B_STATUS getData(unsigned char* data, unsigned int& length);

	void reset();

	void clock(unsigned int ms);

private:
	std::string  m_name;
	unsigned int m_blockSize;
	unsigned int m_blockTime;
	bool         m_debug;
	CTimer       m_timer;
	CStopWatch   m_stopWatch;
	bool         m_running;
	CRingBuffer<unsigned char> m_buffer;
	unsigned int m_outputCount;

	unsigned char* m_lastData;
	unsigned int   m_lastDataLength;
};

#endif
