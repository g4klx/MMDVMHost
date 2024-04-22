/*
 *   Copyright (C) 2017 by Lieven De Samblanx ON7LDS
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

#include "NetworkInfo.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <ctime>
#include <clocale>

#include <sys/types.h>
#if defined(__linux__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__APPLE__)
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#if defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__APPLE__)
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/route.h>
#endif
#elif defined(_WIN32) || defined(_WIN64)
#include <ws2tcpip.h>
#include <WinSock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#ifndef NO_ERROR
#define NO_ERROR 0
#endif
#ifndef ERROR_BUFFER_OVERFLOW
#define ERROR_BUFFER_OVERFLOW 111
#endif
#ifndef ERROR_INSUFFICIENT_BUFFER
#define ERROR_INSUFFICIENT_BUFFER 122
#endif
#endif

CNetworkInfo::CNetworkInfo()
{
}

CNetworkInfo::~CNetworkInfo()
{
}

void CNetworkInfo::getNetworkInterface(unsigned char* info)
{
	LogInfo("Interfaces Info");

	::strcpy((char*)info, "(address unknown)");

#if defined(__linux__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__APPLE__)
	char* dflt = NULL;

#if defined(__linux__)
	FILE* fp = ::fopen("/proc/net/route" , "r");	// IPv4 routing
	if (fp == NULL) {
		LogError("Unabled to open /proc/route");
		return;
	}

	char line[100U];
	while (::fgets(line, 100U, fp)) {
		char* p1 = strtok(line , " \t");
		char* p2 = strtok(NULL , " \t");

		if (p1 != NULL && p2 != NULL) {
			if (::strcmp(p2, "00000000") == 0) {
				dflt = p1;
				break;
			}
		}
	}

	::fclose(fp);

#elif defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__APPLE__)
	int mib[] = {
		CTL_NET,
		PF_ROUTE,
		0,		// protocol
		AF_INET,	// IPv4 routing
		NET_RT_DUMP,
		0,		// show all routes
#if defined(__OpenBSD__) || defined(__FreeBSD__)
		0,		// table id
#endif
	};
	const int cnt = sizeof(mib) / sizeof(int);
	size_t size;
	char ifname[IF_NAMESIZE] = {};

	if (::sysctl(mib, cnt, NULL, &size, NULL, 0) == -1 || size <= 0) {
		LogError("Unable to estimate routing table size");
		return;
	}

	char *buf = new char[size];
	if (::sysctl(mib, cnt, buf, &size, NULL, 0) == -1) {
		LogError("Unable to get routing table");
		delete[] buf;
		return;
	}

	struct rt_msghdr *rtm;
	for (char *p = buf; p < buf + size; p += rtm->rtm_msglen) {
		rtm = (struct rt_msghdr *)p;
		if (rtm->rtm_version != RTM_VERSION)
			continue;
#if defined(__OpenBSD__)
		struct sockaddr_in *sa = (struct sockaddr_in *)(p + rtm->rtm_hdrlen);
#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__APPLE__)
		struct sockaddr_in *sa = (struct sockaddr_in *)(rtm + 1);
#endif
		if (sa->sin_addr.s_addr == INADDR_ANY) {
			::if_indextoname(rtm->rtm_index, ifname);
			break;
		}
	}

	delete[] buf;
	if (::strlen(ifname))
		dflt = ifname;
#endif
	if (dflt == NULL) {
		LogError("Unable to find the default route");
		return;
	}

	const unsigned int IFLISTSIZ = 25U;
	char interfacelist[IFLISTSIZ][50+INET6_ADDRSTRLEN];
	for (unsigned int n = 0U; n < IFLISTSIZ; n++)
		interfacelist[n][0] = 0;

	struct ifaddrs* ifaddr;
	if (::getifaddrs(&ifaddr) == -1) {
		LogError("getifaddrs failure");
		return;
	}

	unsigned int ifnr = 0U;
	for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;

		int family = ifa->ifa_addr->sa_family;
		if (family == AF_INET || family == AF_INET6) {
			char host[NI_MAXHOST];
			int s = ::getnameinfo(ifa->ifa_addr, family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
			if (s != 0) {
				LogError("getnameinfo() failed: %s\n", gai_strerror(s));
				continue;
			}

			if (family == AF_INET) {
				::sprintf(interfacelist[ifnr], "%s:%s", ifa->ifa_name, host);
				LogInfo("    IPv4: %s", interfacelist[ifnr]);
				ifnr++;
			} else {
				::sprintf(interfacelist[ifnr], "%s:%s", ifa->ifa_name, host);
				LogInfo("    IPv6: %s", interfacelist[ifnr]);
				// due to default routing is for IPv4, other
				// protocols are not candidate to display.
			}
		}
	}

	::freeifaddrs(ifaddr);

	LogInfo("    Default interface is : %s" , dflt);

	for (unsigned int n = 0U; n < ifnr; n++) {
		char* p = ::strchr(interfacelist[n], '%');
		if (p != NULL)
			*p = 0;

		if (::strstr(interfacelist[n], dflt) != 0) {
			::strcpy((char*)info, interfacelist[n]);
			break;
		}
	}

	LogInfo("    IP to show: %s", info);
#elif defined(_WIN32) || defined(_WIN64)
	PMIB_IPFORWARDTABLE pIpForwardTable = (MIB_IPFORWARDTABLE *)::malloc(sizeof(MIB_IPFORWARDTABLE));
	if (pIpForwardTable == NULL) {
		LogError("Error allocating memory");
		return;
	}

	DWORD dwSize = 0U;
	if (::GetIpForwardTable(pIpForwardTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
		::free(pIpForwardTable);
		pIpForwardTable = (MIB_IPFORWARDTABLE *)::malloc(dwSize);
		if (pIpForwardTable == NULL) {
			LogError("Error allocating memory");
			return;
		}
	}

	DWORD ret = ::GetIpForwardTable(pIpForwardTable, &dwSize, 0);
	if (ret != NO_ERROR) {
		::free(pIpForwardTable);
		LogError("GetIpForwardTable failed.");
		return;
	}

	DWORD found = 999U;
	for (DWORD i = 0U; i < pIpForwardTable->dwNumEntries; i++) {
		if (pIpForwardTable->table[i].dwForwardDest == 0U) {
			found = i;
			break;
		}
	}

	if (found == 999U) {
		::free(pIpForwardTable);
		LogError("Unable to find the default destination in the routing table.");
		return;
	}

	DWORD ifnr = pIpForwardTable->table[found].dwForwardIfIndex;
	::free(pIpForwardTable);

	PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO *)::malloc(sizeof(IP_ADAPTER_INFO));
	if (pAdapterInfo == NULL) {
		LogError("Error allocating memory");
		return;
	}
    
	ULONG buflen = sizeof(IP_ADAPTER_INFO);
	if (::GetAdaptersInfo(pAdapterInfo, &buflen) == ERROR_BUFFER_OVERFLOW) {
		::free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *)::malloc(buflen);
		if (pAdapterInfo == NULL) {
			LogError("Error allocating memory");
			return;
		}
	}

	if (::GetAdaptersInfo(pAdapterInfo, &buflen) != NO_ERROR) {
		::free(pAdapterInfo);
		LogError("Call to GetAdaptersInfo failed.");
		return;
	}

	PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
	while (pAdapter != NULL) {
		LogInfo("    IP  : %s", pAdapter->IpAddressList.IpAddress.String);
		if (pAdapter->Index == ifnr)
			::strcpy((char*)info, pAdapter->IpAddressList.IpAddress.String);

		pAdapter = pAdapter->Next;
	}

	::free(pAdapterInfo);

	LogInfo("    IP to show: %s", info);
#endif
}
