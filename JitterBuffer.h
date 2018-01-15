/*
*   Copyright (C) 2017,2018 by Jonathan Naylor G4KLX
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

#if !defined(JITTERBUFFER_H)
#define	JITTERBUFFER_H

#include "StopWatch.h"
#include "Timer.h"

#include <string>

enum JB_STATUS {
	JBS_NO_DATA,
	JBS_DATA,
	JBS_MISSING
};

class CJitterBuffer {
public:
	CJitterBuffer(const std::string& name, unsigned int blockSize, unsigned int blockTime, unsigned int jitterTime, unsigned int topSequenceNumber, bool debug);
	~CJitterBuffer();

	bool addData(const unsigned char* data, unsigned int length, unsigned int sequenceNumber);
	bool appendData(const unsigned char* data, unsigned int length);

	JB_STATUS getData(unsigned char* data, unsigned int& length);
	
	void reset();
	
	void clock(unsigned int ms);
	
private:
	std::string  m_name;
	unsigned int m_blockSize;
	unsigned int m_blockTime;
	unsigned int m_topSequenceNumber;
	bool         m_debug;
	unsigned int m_blockCount;
	CTimer       m_timer;
	CStopWatch   m_stopWatch;
	bool         m_running;

	struct JitterEntry
	{
		unsigned char* m_data;
		unsigned int   m_length;
	};

	JitterEntry* m_buffer;
	unsigned int m_headSequenceNumber;

	unsigned int m_appendSequenceNumber;

	unsigned char* m_lastData;
	unsigned int   m_lastDataLength;
};

#endif
