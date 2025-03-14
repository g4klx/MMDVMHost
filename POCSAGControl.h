/*
*   Copyright (C) 2018,2019,2020,2023,2025 by Jonathan Naylor G4KLX
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
#include "Defines.h"

#if defined(USE_POCSAG)

#include <cstdint>

#include <string>
#include <deque>

#include <nlohmann/json.hpp>

struct POCSAGData {
	unsigned int         m_ric;
	std::string          m_text;
	std::string          m_display;
	std::deque<uint32_t> m_buffer;
};

class CPOCSAGControl {
public:
	CPOCSAGControl(CPOCSAGNetwork* network);
	~CPOCSAGControl();

	void sendPage(unsigned int ric, const std::string& text);
	void sendPageBCD(unsigned int ric, const std::string& text);
	void sendPageAlert1(unsigned int ric);
	void sendPageAlert2(unsigned int ric, const std::string& text);

	unsigned int readModem(unsigned char* data);

	void clock(unsigned int ms);

	void enable(bool enabled);

private:
	CPOCSAGNetwork*            m_network;
	CRingBuffer<unsigned char> m_queue;
	unsigned int               m_frames;
	unsigned int               m_count;

	enum class POCSAG_STATE {
		NONE,
		WAITING,
		SENDING,
		ENDING
	};

	std::deque<uint32_t>       m_output;
	std::deque<uint32_t>       m_buffer;
	unsigned int               m_ric;
	std::deque<POCSAGData*>    m_data;
	POCSAG_STATE               m_state;
	bool                       m_enabled;

	bool readNetwork();
	void writeQueue();
	bool processData();
	void addAddress(unsigned char functional, unsigned int ric, std::deque<uint32_t>& buffer) const;
	void packASCII(const std::string& text, std::deque<uint32_t>& buffer) const;
	void packNumeric(const std::string& text, std::deque<uint32_t>& buffer) const;
	void addBCHAndParity(uint32_t& word) const;

	void decodeROT1(const std::string& in, unsigned int start, std::string& out) const;

	void writeJSON(const char* source, unsigned int ric, const char* functional);
	void writeJSON(const char* source, unsigned int ric, const char* functional, const std::string& message);
};

#endif

#endif

