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

#ifndef	AX25Network_H
#define	AX25Network_H

#if defined(_WIN32) || defined(_WIN64)
#include "UARTController.h"
#else
#include "PseudoTTYController.h"
#endif

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
#if defined(_WIN32) || defined(_WIN64)
	CUARTController      m_serial;
#else
	CPseudoTTYController m_serial;
#endif
	unsigned char*       m_txData;
	unsigned char*       m_rxData;
	unsigned int         m_rxLength;
	unsigned char        m_rxLastChar;
	bool                 m_debug;
	bool                 m_enabled;
};

#endif
