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

#ifndef	MobileGPS_H
#define	MobileGPS_H

#include "DMRNetwork.h"
#include "UDPSocket.h"
#include "Timer.h"

#include <string>

#if !defined(_WIN32) && !defined(_WIN64)
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#else
#include <winsock.h>
#endif

class CMobileGPS {
public:
	CMobileGPS(const std::string& address, unsigned int port, CDMRNetwork* network);
	~CMobileGPS();

	bool open();

	void clock(unsigned int ms);

	void close();

private:
	CTimer       m_idTimer;
	in_addr      m_address;
	unsigned int m_port;
	CUDPSocket   m_socket;
	CDMRNetwork* m_network;

	bool pollGPS();
	void sendReport();
};

#endif
