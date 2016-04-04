/*
 *   Copyright (C) 2009-2011,2013,2015,2016 by Jonathan Naylor G4KLX
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

#ifndef UDPSocket_H
#define UDPSocket_H

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

class CUDPSocket {
public:
	CUDPSocket(const std::string& address, unsigned int port = 0U);
	CUDPSocket(unsigned int port = 0U);
	~CUDPSocket();

	bool open();

	int  read(unsigned char* buffer, unsigned int length, in_addr& address, unsigned int& port);
	bool write(const unsigned char* buffer, unsigned int length, const in_addr& address, unsigned int port);

	void close();

	static in_addr lookup(const std::string& hostName);

private:
	std::string    m_address;
	unsigned short m_port;
	int            m_fd;
};

#endif
