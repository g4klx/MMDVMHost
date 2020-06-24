/*
*   Copyright (C) 2018,2019 by Jonathan Naylor G4KLX
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

#include <cstdint>

#include <string>
#include <deque>

struct POCSAGData {
	unsigned int         m_ric;
	std::string          m_text;
	std::deque<uint32_t> m_buffer;
};

class CPOCSAGControl {
public:
	CPOCSAGControl(CPOCSAGNetwork* network, CDisplay* display);
	~CPOCSAGControl();

	void sendPage(unsigned int ric, const std::string& text);

	unsigned int readModem(unsigned char* data);

	void clock(unsigned int ms);

	void enable(bool enabled);

private:
	CPOCSAGNetwork*            m_network;
	CDisplay*                  m_display;
	CRingBuffer<unsigned char> m_queue;
	unsigned int               m_frames;
	unsigned int               m_count;

	enum POCSAG_STATE {
		PS_NONE,
		PS_WAITING,
		PS_SENDING,
		PS_ENDING
	};

	std::deque<uint32_t>       m_output;
	std::deque<uint32_t>       m_buffer;
	unsigned int               m_ric;
	std::deque<POCSAGData*>    m_data;
	POCSAG_STATE               m_state;
	bool                       m_enabled;
	FILE*                      m_fp;

	bool readNetwork();
	void writeQueue();
	bool processData();
	void addAddress(unsigned char functional, unsigned int ric, std::deque<uint32_t>& buffer) const;
	void packASCII(const std::string& text, std::deque<uint32_t>& buffer) const;
	void packNumeric(const std::string& text, std::deque<uint32_t>& buffer) const;
	void addBCHAndParity(uint32_t& word) const;
	bool openFile();
	bool writeFile(const unsigned char* data);
	void closeFile();
};

#endif
