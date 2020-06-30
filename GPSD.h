/*
 *   Copyright (C) 2018,2020 by Jonathan Naylor G4KLX
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

#ifndef	GPSD_H
#define	GPSD_H

#if defined(USE_GPSD)

#include "DMRNetwork.h"
#include "Timer.h"

#include <string>

#include <gps.h>

class CGPSD {
public:
	CGPSD(const std::string& address, const std::string& port, CDMRNetwork* network);
	~CGPSD();

	bool open();

	void clock(unsigned int ms);

	void close();

private:
	std::string       m_gpsdAddress;
	std::string       m_gpsdPort;
	CDMRNetwork*      m_network;
	struct gps_data_t m_gpsdData;
	CTimer            m_idTimer;

	void sendReport();
};

#endif

#endif
