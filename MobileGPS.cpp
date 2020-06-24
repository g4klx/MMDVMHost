/*
 *   Copyright (C) 2018 by Jonathan Naylor G4KLX
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

#include "MobileGPS.h"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <cmath>

CMobileGPS::CMobileGPS(const std::string& address, unsigned int port, CDMRNetwork* network) :
m_idTimer(1000U, 60U),
m_address(),
m_port(port),
m_socket(),
m_network(network)
{
	assert(!address.empty());
	assert(port > 0U);
	assert(network != NULL);

	m_address = CUDPSocket::lookup(address);
}

CMobileGPS::~CMobileGPS()
{
}

bool CMobileGPS::open()
{
	bool ret = m_socket.open();
	if (!ret)
		return false;

	m_idTimer.start();

	return true;
}

void CMobileGPS::clock(unsigned int ms)
{
	m_idTimer.clock(ms);

	if (m_idTimer.hasExpired()) {
		pollGPS();
		m_idTimer.start();
	}

	sendReport();
}

void CMobileGPS::close()
{
	m_socket.close();
}

bool CMobileGPS::pollGPS()
{
	return m_socket.write((unsigned char*)"MMDVMHost", 9U, m_address, m_port);
}

void CMobileGPS::sendReport()
{
	// Grab GPS data if it's available
	unsigned char buffer[200U];
	in_addr address;
	unsigned int port;
	int ret = m_socket.read(buffer, 200U, address, port);
	if (ret <= 0)
		return;

	buffer[ret] = '\0';

	// Parse the GPS data
	char* pLatitude  = ::strtok((char*)buffer, ",\n");	// Latitude
	char* pLongitude = ::strtok(NULL, ",\n");		// Longitude

	if (pLatitude == NULL || pLongitude == NULL)
		return;

	float latitude  = float(::atof(pLatitude));
	float longitude = float(::atof(pLongitude));

	m_network->writeHomePosition(latitude, longitude);
}

