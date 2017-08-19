/*
*   Copyright (C) 2016,2017 by Jonathan Naylor G4KLX
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

#if !defined(YSFPayload_H)
#define	YSFPayload_H

#include "AMBEFEC.h"

#include <string>

class CYSFPayload {
public:
	CYSFPayload();
	~CYSFPayload();

	bool processHeaderData(unsigned char* bytes);

	bool processVDMode1Data(unsigned char* bytes, unsigned char fn, bool gateway = false);
	unsigned int processVDMode1Audio(unsigned char* bytes);

	bool processVDMode2Data(unsigned char* bytes, unsigned char fn, bool gateway = false);
	unsigned int processVDMode2Audio(unsigned char* bytes);

	bool processDataFRModeData(unsigned char* bytes, unsigned char fn, bool gateway = false);

	unsigned int processVoiceFRModeAudio(unsigned char* bytes);

	void writeHeader(unsigned char* data, const unsigned char* csd1, const unsigned char* csd2);

	void writeDataFRModeData1(const unsigned char* dt, unsigned char* data);
	void writeDataFRModeData2(const unsigned char* dt, unsigned char* data);

	unsigned char* getSource();
	unsigned char* getDest();

	void setUplink(const std::string& callsign);
	void setDownlink(const std::string& callsign);

	void reset();

private:
	unsigned char* m_uplink;
	unsigned char* m_downlink;
	unsigned char* m_source;
	unsigned char* m_dest;
	CAMBEFEC       m_fec;
};

#endif
