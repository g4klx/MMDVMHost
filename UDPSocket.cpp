/*
 *   Copyright (C) 2006-2016 by Jonathan Naylor G4KLX
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

#include "UDPSocket.h"
#include "Log.h"

#include <cassert>

#if !defined(_WIN32) && !defined(_WIN64)
#include <cerrno>
#include <cstring>
#endif


CUDPSocket::CUDPSocket(const std::string& address, unsigned int port) :
m_address(address),
m_port(port),
m_fd(-1)
{
#if defined(_WIN32) || defined(_WIN64)
	WSAData data;
	int wsaRet = ::WSAStartup(MAKEWORD(2, 2), &data);
	if (wsaRet != 0)
		LogError("Error from WSAStartup");
#endif
}

CUDPSocket::CUDPSocket(unsigned int port) :
m_address(),
m_port(port),
m_fd(-1)
{
#if defined(_WIN32) || defined(_WIN64)
	WSAData data;
	int wsaRet = ::WSAStartup(MAKEWORD(2, 2), &data);
	if (wsaRet != 0)
		LogError("Error from WSAStartup");
#endif
}

CUDPSocket::~CUDPSocket()
{
#if defined(_WIN32) || defined(_WIN64)
	::WSACleanup();
#endif
}

int CUDPSocket::lookup(const std::string& hostname, unsigned int port, sockaddr_storage &addr, unsigned int &address_length)
{
	struct addrinfo hints;

	::memset(&hints, 0, sizeof(hints));

	return lookup(hostname, port, addr, address_length, hints);
}

int CUDPSocket::lookup(const std::string& hostname, unsigned int port, sockaddr_storage &addr, unsigned int &address_length, struct addrinfo &hints)
{
	int err;
	std::string portstr = std::to_string(port);
	struct addrinfo *res;

	/* port is always digits, no needs to lookup service */
	hints.ai_flags |= AI_NUMERICSERV;

	err = getaddrinfo(hostname.empty() ? NULL : hostname.c_str(), portstr.c_str(), &hints, &res);
	if (err) {
		sockaddr_in *paddr = (sockaddr_in *)&addr;
		::memset(paddr, 0, address_length = sizeof(sockaddr_in));
		paddr->sin_family = AF_INET;
		paddr->sin_port = htons(port);
		paddr->sin_addr.s_addr = htonl(INADDR_NONE);
		LogError("Cannot find address for host %s", hostname.c_str());
		return err;
	}

	::memcpy(&addr, res->ai_addr, address_length = res->ai_addrlen);

	freeaddrinfo(res);
	return 0;
}

bool CUDPSocket::match(const sockaddr_storage &addr1, const sockaddr_storage &addr2)
{
	if (addr1.ss_family != addr2.ss_family)
		return false;

	switch (addr1.ss_family) {
	case AF_INET:
		struct sockaddr_in *in_1, *in_2;
		in_1 = (struct sockaddr_in *)&addr1;
		in_2 = (struct sockaddr_in *)&addr2;
		return ( (in_1->sin_addr.s_addr == in_2->sin_addr.s_addr) &&
			 (in_1->sin_port == in_2->sin_port) );
	case AF_INET6:
		struct sockaddr_in6 *in6_1, *in6_2;
		in6_1 = (struct sockaddr_in6 *)&addr1;
		in6_2 = (struct sockaddr_in6 *)&addr2;
		return ( IN6_ARE_ADDR_EQUAL(&in6_1->sin6_addr, &in6_2->sin6_addr) &&
			 (in6_1->sin6_port == in6_2->sin6_port) );
	default:
		return false;
	}
}

bool CUDPSocket::isnone(const sockaddr_storage &addr)
{
	struct sockaddr_in *in = (struct sockaddr_in *)&addr;

	return ( (addr.ss_family == AF_INET) &&
		 (in->sin_addr.s_addr == htonl(INADDR_NONE)) );
}

bool CUDPSocket::open()
{
	return open(AF_UNSPEC);
}

bool CUDPSocket::open(const unsigned int af)
{
	int err;
	sockaddr_storage addr;
	unsigned int addrlen;
	struct addrinfo hints;

	::memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = af;

	/* to determine protocol family, call lookup() first. */
	err = lookup(m_address, m_port, addr, addrlen, hints);
	if (err) {
		LogError("The local address is invalid - %s", m_address.c_str());
		return false;
	}

	m_fd = ::socket(addr.ss_family, SOCK_DGRAM, 0);
	if (m_fd < 0) {
#if defined(_WIN32) || defined(_WIN64)
		LogError("Cannot create the UDP socket, err: %lu", ::GetLastError());
#else
		LogError("Cannot create the UDP socket, err: %d", errno);
#endif
		return false;
	}

	if (m_port > 0U) {
		int reuse = 1;
		if (::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) == -1) {
#if defined(_WIN32) || defined(_WIN64)
			LogError("Cannot set the UDP socket option, err: %lu", ::GetLastError());
#else
			LogError("Cannot set the UDP socket option, err: %d", errno);
#endif
			return false;
		}

		if (::bind(m_fd, (sockaddr*)&addr, addrlen) == -1) {
#if defined(_WIN32) || defined(_WIN64)
			LogError("Cannot bind the UDP address, err: %lu", ::GetLastError());
#else
			LogError("Cannot bind the UDP address, err: %d", errno);
#endif
			return false;
		}
	}

	return true;
}

int CUDPSocket::read(unsigned char* buffer, unsigned int length, sockaddr_storage& address, unsigned int &address_length)
{
	assert(buffer != NULL);
	assert(length > 0U);

	// Check that the readfrom() won't block
	fd_set readFds;
	FD_ZERO(&readFds);
#if defined(_WIN32) || defined(_WIN64)
	FD_SET((unsigned int)m_fd, &readFds);
#else
	FD_SET(m_fd, &readFds);
#endif

	// Return immediately
	timeval tv;
	tv.tv_sec  = 0L;
	tv.tv_usec = 0L;

	int ret = ::select(m_fd + 1, &readFds, NULL, NULL, &tv);
	if (ret < 0) {
#if defined(_WIN32) || defined(_WIN64)
		LogError("Error returned from UDP select, err: %lu", ::GetLastError());
#else
		LogError("Error returned from UDP select, err: %d", errno);
#endif
		return -1;
	}

	if (ret == 0)
		return 0;

#if defined(_WIN32) || defined(_WIN64)
	int size = sizeof(sockaddr_storage);
#else
	socklen_t size = sizeof(sockaddr_storage);
#endif

#if defined(_WIN32) || defined(_WIN64)
	int len = ::recvfrom(m_fd, (char*)buffer, length, 0, (sockaddr *)&address, &size);
#else
	ssize_t len = ::recvfrom(m_fd, (char*)buffer, length, 0, (sockaddr *)&address, &size);
#endif
	if (len <= 0) {
#if defined(_WIN32) || defined(_WIN64)
		LogError("Error returned from recvfrom, err: %lu", ::GetLastError());
#else
		LogError("Error returned from recvfrom, err: %d", errno);
#endif
		return -1;
	}

	address_length = size;
	return len;
}

bool CUDPSocket::write(const unsigned char* buffer, unsigned int length, const sockaddr_storage& address, unsigned int address_length)
{
	assert(buffer != NULL);
	assert(length > 0U);

#if defined(_WIN32) || defined(_WIN64)
	int ret = ::sendto(m_fd, (char *)buffer, length, 0, (sockaddr *)&address, address_length);
#else
	ssize_t ret = ::sendto(m_fd, (char *)buffer, length, 0, (sockaddr *)&address, address_length);
#endif
	if (ret < 0) {
#if defined(_WIN32) || defined(_WIN64)
		LogError("Error returned from sendto, err: %lu", ::GetLastError());
#else
		LogError("Error returned from sendto, err: %d", errno);
#endif
		return false;
	}

#if defined(_WIN32) || defined(_WIN64)
	if (ret != int(length))
		return false;
#else
	if (ret != ssize_t(length))
		return false;
#endif

	return true;
}

void CUDPSocket::close()
{
#if defined(_WIN32) || defined(_WIN64)
	::closesocket(m_fd);
#else
	::close(m_fd);
#endif
}
