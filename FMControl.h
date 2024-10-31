/*
 *   Copyright (C) 2020,2021,2024 by Jonathan Naylor G4KLX
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

#include "RingBuffer.h"
#include "FMNetwork.h"
#include "Defines.h"
#include "IIR.h"
#include "FIR.h"

// Uncomment this to dump audio to a raw audio file
// The file will be written in same folder as executable
// Toplay the file : ffplay -autoexit -f u16be -ar 8000 audiodump.bin
// #define DUMP_RF_AUDIO

class CFMControl {
public:
	CFMControl(IFMNetwork* network, float txAudioGain, float rxAudioGain, bool preEmphasisOn, bool deEmphasisOn);
	~CFMControl();

	bool writeModem(const unsigned char* data, unsigned int length);

	unsigned int readModem(unsigned char* data, unsigned int space);

	void clock(unsigned int ms);

	void enable(bool enabled);

private:
	IFMNetwork* m_network;
	float       m_txAudioGain;
	float       m_rxAudioGain;
	bool        m_preEmphasisOn;
	bool        m_deEmphasisOn;
	bool        m_enabled;
	bool        m_begin;

	CRingBuffer<unsigned char> m_incomingRFAudio;
	CIIR        m_preEmphasis;
	CIIR        m_deEmphasis;
	CFIR        m_filter;
};

#endif
