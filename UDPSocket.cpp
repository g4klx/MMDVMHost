/*
 *   Copyright (C) 2006-2016,2020,2024 by Jonathan Naylor G4KLX
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

#include <cassert>

#if !defined(_WIN32) && !defined(_WIN64)
#include <cerrno>
#include <cstring>
#endif

#if defined(HAVE_LOG_H)
#include "Log.h"
#else
#define LogMessage(fmt, ...)	::fprintf(stderr, fmt "\n", ## __VA_ARGS__)
#define LogError(fmt, ...)	::fprintf(stderr, fmt "\n", ## __VA_ARGS__)
#define LogInfo(fmt, ...)	::fprintf(stderr, fmt "\n", ## __VA_ARGS__)
#endif

CUDPSocket::CUDPSocket(const std::string& address, unsigned short port) :
m_localAddress(address),
m_localPort(port),
#if defined(_WIN32) || defined(_WIN64)
m_fd(INVALID_SOCKET),
#else
m_fd(-1),
#endif
m_af(AF_UNSPEC)
{
}

CUDPSocket::CUDPSocket(unsigned short port) :
m_localAddress(),
m_localPort(port),
#if defined(_WIN32) || defined(_WIN64)
m_fd(INVALID_SOCKET),
#else
m_fd(-1),
#endif
m_af(AF_UNSPEC)
{
}

CUDPSocket::~CUDPSocket()
{
}

void CUDPSocket::startup()
{
#if defined(_WIN32) || defined(_WIN64)
	WSAData data;
	int wsaRet = ::WSAStartup(MAKEWORD(2, 2), &data);
	if (wsaRet != 0)
		LogError("Error from WSAStartup");
#endif
}

void CUDPSocket::shutdown()
{
#if defined(_WIN32) || defined(_WIN64)
	::WSACleanup();
#endif
}

int CUDPSocket::lookup(const std::string& hostname, unsigned short port, sockaddr_storage& addr, unsigned int& address_length)
{
	struct addrinfo hints;
	::memset(&hints, 0, sizeof(hints));

	return lookup(hostname, port, addr, address_length, hints);
}

int CUDPSocket::lookup(const std::string& hostname, unsigned short port, sockaddr_storage& addr, unsigned int& address_length, struct addrinfo& hints)
{
	std::string portstr = std::to_string(port);
	struct addrinfo *res;

	/* Port is always digits, no needs to lookup service */
	hints.ai_flags |= AI_NUMERICSERV;

	int err = ::getaddrinfo(hostname.empty() ? NULL : hostname.c_str(), portstr.c_str(), &hints, &res);
	if (err != 0) {
		sockaddr_in* paddr = (sockaddr_in*)&addr;
		::memset(paddr, 0x00U, address_length = sizeof(sockaddr_in));
		paddr->sin_family = AF_INET;
		paddr->sin_port = htons(port);
		paddr->sin_addr.s_addr = htonl(INADDR_NONE);
		LogError("Cannot find address for host %s", hostname.c_str());
		return err;
	}

	address_length = (unsigned int)res->ai_addrlen;

	::memcpy(&addr, res->ai_addr, address_length);

	::freeaddrinfo(res);

	return 0;
}

bool CUDPSocket::match(const sockaddr_storage& addr1, const sockaddr_storage& addr2, IPMATCHTYPE type)
{
	if (addr1.ss_family != addr2.ss_family)
		return false;

	if (type == IMT_ADDRESS_AND_PORT) {
		switch (addr1.ss_family) {
			case AF_INET:
				struct sockaddr_in *in_1, *in_2;
				in_1 = (struct sockaddr_in*)&addr1;
				in_2 = (struct sockaddr_in*)&addr2;
				return (in_1->sin_addr.s_addr == in_2->sin_addr.s_addr) && (in_1->sin_port == in_2->sin_port);
			case AF_INET6:
				struct sockaddr_in6 *in6_1, *in6_2;
				in6_1 = (struct sockaddr_in6*)&addr1;
				in6_2 = (struct sockaddr_in6*)&addr2;
				return IN6_ARE_ADDR_EQUAL(&in6_1->sin6_addr, &in6_2->sin6_addr) && (in6_1->sin6_port == in6_2->sin6_port);
			default:
				return false;
		}
	} else if (type == IMT_ADDRESS_ONLY) {
		switch (addr1.ss_family) {
			case AF_INET:
				struct sockaddr_in *in_1, *in_2;
				in_1 = (struct sockaddr_in*)&addr1;
				in_2 = (struct sockaddr_in*)&addr2;
				return in_1->sin_addr.s_addr == in_2->sin_addr.s_addr;
			case AF_INET6:
				struct sockaddr_in6 *in6_1, *in6_2;
				in6_1 = (struct sockaddr_in6*)&addr1;
				in6_2 = (struct sockaddr_in6*)&addr2;
				return IN6_ARE_ADDR_EQUAL(&in6_1->sin6_addr, &in6_2->sin6_addr);
			default:
				return false;
		}
	} else {
		return false;
	}
}

bool CUDPSocket::isNone(const sockaddr_storage& addr)
{
	struct sockaddr_in *in = (struct sockaddr_in *)&addr;

	return ((addr.ss_family == AF_INET) && (in->sin_addr.s_addr == htonl(INADDR_NONE)));
}

bool CUDPSocket::open(const sockaddr_storage& address)
{
	m_af = address.ss_family;

	return open();
}

bool CUDPSocket::open()
{
#if defined(_WIN32) || defined(_WIN64)
	assert(m_fd == INVALID_SOCKET);
#else
	assert(m_fd == -1);
#endif

	sockaddr_storage addr;
	unsigned int addrlen;
	struct addrinfo hints;

	::memset(&hints, 0, sizeof(hints));
	hints.ai_flags  = AI_PASSIVE;
	hints.ai_family = m_af;

	// To determine protocol family, call lookup() on the local address first.
	int err = lookup(m_localAddress, m_localPort, addr, addrlen, hints);
	if (err != 0) {
		LogError("The local address is invalid - %s", m_localAddress.c_str());
		return false;
	}

	m_af = addr.ss_family;

	m_fd = ::socket(m_af, SOCK_DGRAM, 0);
	if (m_fd < 0) {
#if defined(_WIN32) || defined(_WIN64)
		LogError("Cannot create the UDP socket, err: %lu", ::GetLastError());
#else
		LogError("Cannot create the UDP socket, err: %d", errno);
#endif
		return false;
	}

	if (m_localPort > 0U) {
		int reuse = 1;
		if (::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) == -1) {
#if defined(_WIN32) || defined(_WIN64)
			LogError("Cannot set the UDP socket option, err: %lu", ::GetLastError());
#else
			LogError("Cannot set the UDP socket option, err: %d", errno);
#endif
			close();
			return false;
		}

		if (::bind(m_fd, (sockaddr*)&addr, addrlen) == -1) {
#if defined(_WIN32) || defined(_WIN64)
			LogError("Cannot bind the UDP address, err: %lu", ::GetLastError());
#else
			LogError("Cannot bind the UDP address, err: %d", errno);
#endif
			close();
			return false;
		}

		LogInfo("Opening UDP port on %hu", m_localPort);
	}

	return true;
}

int CUDPSocket::read(unsigned char* buffer, unsigned int length, sockaddr_storage& address, unsigned int &addressLength)
{
	assert(buffer != NULL);
	assert(length > 0U);

#if defined(_WIN32) || defined(_WIN64)
	if (m_fd == INVALID_SOCKET)
		return 0;
#else
	if (m_fd == -1)
		return 0;
#endif

	// Check that the readfrom() won't block
	struct pollfd pfd;
	pfd.fd      = m_fd;
	pfd.events  = POLLIN;
	pfd.revents = 0;

	// Return immediately
#if defined(_WIN32) || defined(_WIN64)
	int ret = WSAPoll(&pfd, 1, 0);
#else
	int ret = ::poll(&pfd, 1, 0);
#endif
	if (ret < 0) {
#if defined(_WIN32) || defined(_WIN64)
		LogError("Error returned from UDP poll, err: %lu", ::GetLastError());
#else
		LogError("Error returned from UDP poll, err: %d", errno);
#endif
		return -1;
	}

	if ((pfd.revents & POLLIN) == 0)
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

		if (len == -1 && errno == ENOTSOCK) {
			LogMessage("Re-opening UDP port on %hu", m_localPort);
			close();
			open();
		}
#endif
		return -1;
	}

	addressLength = size;

	return len;
}

bool CUDPSocket::write(const unsigned char* buffer, unsigned int length, const sockaddr_storage& address, unsigned int addressLength)
{
	assert(buffer != NULL);
	assert(length > 0U);
#if defined(_WIN32) || defined(_WIN64)
	assert(m_fd != INVALID_SOCKET);
#else
	assert(m_fd >= 0);
#endif

	bool result = false;

#if defined(_WIN32) || defined(_WIN64)
	int ret = ::sendto(m_fd, (char *)buffer, length, 0, (sockaddr *)&address, addressLength);
#else
	ssize_t ret = ::sendto(m_fd, (char *)buffer, length, 0, (sockaddr *)&address, addressLength);
#endif

	if (ret < 0) {
#if defined(_WIN32) || defined(_WIN64)
		LogError("Error returned from sendto, err: %lu", ::GetLastError());
#else
		LogError("Error returned from sendto, err: %d", errno);
#endif
	} else {
#if defined(_WIN32) || defined(_WIN64)
		if (ret == int(length))
			result = true;
#else
		if (ret == ssize_t(length))
			result = true;
#endif
	}

	return result;
}

void CUDPSocket::close()
{
#if defined(_WIN32) || defined(_WIN64)
	if (m_fd != INVALID_SOCKET) {
		::closesocket(m_fd);
		m_fd = INVALID_SOCKET;
	}
#else
	if (m_fd >= 0) {
		::close(m_fd);
		m_fd = -1;
	}
#endif
}

