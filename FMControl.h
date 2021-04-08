/*
 *   Copyright (C) 2020,2021 by Jonathan Naylor G4KLX
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
// Toplay the file : ffplay -autoexit -f u16be -ar 8000 audiodump.bin
// #define DUMP_RF_AUDIO

class CFMControl {
public:
	CFMControl(CFMNetwork* network, float txAudioGain, float rxAudioGain, bool preEmphasisOn, bool deEmphasisOn);
	~CFMControl();

	bool writeModem(const unsigned char* data, unsigned int length);

	unsigned int readModem(unsigned char* data, unsigned int space);

	void clock(unsigned int ms);

	void enable(bool enabled);

private:
	CFMNetwork* m_network;
	float       m_txAudioGain;
	float       m_rxAudioGain;
	bool        m_preEmphasisOn;
	bool        m_deEmphasisOn;
    bool        m_enabled;
	CRingBuffer<unsigned char> m_incomingRFAudio;
	CIIRDirectForm1Filter* m_preEmphasis;
	CIIRDirectForm1Filter* m_deEmphasis;
	CIIRDirectForm1Filter* m_filterStage1;
	CIIRDirectForm1Filter* m_filterStage2;
	CIIRDirectForm1Filter* m_filterStage3;
};

#endif
