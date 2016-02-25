/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
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

#if !defined(YSFControl_H)
#define	YSFControl_H

#include "YSFDefines.h"
#include "RingBuffer.h"
#include "YSFParrot.h"
#include "Display.h"
#include "Defines.h"
#include "Timer.h"
#include "Modem.h"

#include <string>

class CYSFControl {
public:
	CYSFControl(const std::string& callsign, IDisplay* display, unsigned int timeout, bool duplex, bool parrot);
	~CYSFControl();

	bool writeModem(unsigned char* data);

	unsigned int readModem(unsigned char* data);

	void clock(unsigned int ms);

private:
	IDisplay*                  m_display;
	bool                       m_duplex;
	CRingBuffer<unsigned char> m_queue;
	RPT_RF_STATE               m_state;
	CTimer                     m_timeoutTimer;
	unsigned int               m_frames;
	CYSFParrot*                m_parrot;
	FILE*                      m_fp;

	void writeQueue(const unsigned char* data);
	void writeParrot(const unsigned char* data);

	void writeEndOfTransmission();

	bool openFile();
	bool writeFile(const unsigned char* data);
	void closeFile();
};

#endif
