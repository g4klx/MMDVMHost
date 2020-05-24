/*
 *   Copyright (C) 2020 by Jonathan Naylor G4KLX
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

#if !defined(FMControl_H)
#define	FMControl_H

#include "FMNetwork.h"
#include "Defines.h"
#include "IIRDirectForm1Filter.h"

// Uncomment this to dump audio to a raw audio file
// The file will be written in same folder as executable
// Toplay the file : aplay -f FLOAT_LE -c1 -r8000 -t raw audiodump.bin
//#define DUMP_RF_AUDIO

class CFMControl {
public:
	CFMControl(CFMNetwork* network);
	~CFMControl();

	bool writeModem(const unsigned char* data, unsigned int length);

	unsigned int readModem(unsigned char* data, unsigned int space);

	void clock(unsigned int ms);

	void enable(bool enabled);

private:
	CFMNetwork* m_network;
    bool        m_enabled;
	CRingBuffer<unsigned char> m_incomingRFAudio;
	CIIRDirectForm1Filter m_preemphasis;
	CIIRDirectForm1Filter m_deemphasis;
};

#endif
