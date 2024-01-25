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

#if !defined(AX25Control_H)
#define	AX25Control_H

#include "AX25Network.h"

#include <string>

class CAX25Control {
public:
	CAX25Control(CAX25Network* network, bool trace);
	~CAX25Control();

	bool writeModem(unsigned char* data, unsigned int len);

	unsigned int readModem(unsigned char* data);

	void enable(bool enabled);

private:
	CAX25Network* m_network;
	bool          m_trace;
	bool          m_enabled;
	FILE*         m_fp;

	void decode(const unsigned char* data, unsigned int length);
	bool decodeAddress(const unsigned char* data, std::string& text, bool isDigi = false) const;
	bool openFile();
	bool writeFile(const unsigned char* data, unsigned int length);
	void closeFile();
};

#endif
