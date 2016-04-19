/*
*   Copyright (C) 2016 by Jonathan Naylor G4KLX
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

	void process(unsigned char* bytes, unsigned char fi, unsigned char fn, unsigned char ft, unsigned char dt);

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

	void processHeader(unsigned char* bytes);
	void processVDMode1(unsigned char* bytes, unsigned char fn);
	void processVDMode2(unsigned char* bytes, unsigned char fn);
	void processDataFRMode(unsigned char* bytes, unsigned char fn);
	void processVoiceFRMode(unsigned char* bytes);
};

#endif
