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

#if !defined(POCSAGControl_H)
#define	POCSAGControl_H

#include "POCSAGNetwork.h"
#include "POCSAGDefines.h"
#include "RingBuffer.h"
#include "Display.h"
#include "Defines.h"

#include <string>

class CPOCSAGControl {
public:
	CPOCSAGControl(CPOCSAGNetwork* network, CDisplay* display);
	~CPOCSAGControl();

	unsigned int readModem(unsigned char* data);

	void clock(unsigned int ms);

private:
	CPOCSAGNetwork*            m_network;
	CDisplay*                  m_display;
	CRingBuffer<unsigned char> m_queue;
	unsigned int               m_frames;
	FILE*                      m_fp;

	void writeNetwork();

	bool openFile();
	bool writeFile(const unsigned char* data);
	void closeFile();
};

#endif
