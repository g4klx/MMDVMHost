/*
 *   Copyright (C) 2009-2011,2013,2015,2016,2020,2024 by Jonathan Naylor G4KLX
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

#if !defined(UDPSocket_H)
#define UDPSocket_H

#include <string>

#if !defined(_WIN32) && !defined(_WIN64)
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#else
#include <ws2tcpip.h>
#include <Winsock2.h>
#endif

enum IPMATCHTYPE {
	IMT_ADDRESS_AND_PORT,
	IMT_ADDRESS_ONLY
};

class CUDPSocket {
public:
	CUDPSocket(const std::string& address, unsigned short port = 0U);
	CUDPSocket(unsigned short port = 0U);
	~CUDPSocket();

	bool open();
	bool open(const sockaddr_storage& address);

	int  read(unsigned char* buffer, unsigned int length, sockaddr_storage& address, unsigned int &addressLength);
	bool write(const unsigned char* buffer, unsigned int length, const sockaddr_storage& address, unsigned int addressLength);

	void close();

	static void startup();
	static void shutdown();

	static int lookup(const std::string& hostName, unsigned short port, sockaddr_storage& address, unsigned int& addressLength);
	static int lookup(const std::string& hostName, unsigned short port, sockaddr_storage& address, unsigned int& addressLength, struct addrinfo& hints);

	static bool match(const sockaddr_storage& addr1, const sockaddr_storage& addr2, IPMATCHTYPE type = IMT_ADDRESS_AND_PORT);

	static bool isNone(const sockaddr_storage& addr);

private:
	std::string    m_localAddress;
	unsigned short m_localPort;
#if defined(_WIN32) || defined(_WIN64)
	SOCKET         m_fd;
	int            m_af;
#else
	int            m_fd;
	sa_family_t    m_af;
#endif
};

#endif
