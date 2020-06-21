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

#ifndef	AX25Network_H
#define	AX25Network_H

#include "SerialController.h"

#include <cstdint>
#include <string>

class CAX25Network {
public:
	CAX25Network(const std::string& port, unsigned int speed, bool debug);
	~CAX25Network();

	bool open();

	void enable(bool enabled);

	bool write(const unsigned char* data, unsigned int length);

	unsigned int read(unsigned char* data, unsigned int length);

	void reset();

	void close();

private:
	CSerialController m_serial;
	unsigned char*    m_txData;
	unsigned int      m_txLength;
	unsigned int      m_txOffset;
	unsigned char*    m_rxData;
	unsigned int      m_rxLength;
	bool              m_rxComplete;
	bool              m_debug;
	bool              m_enabled;
};

#endif
