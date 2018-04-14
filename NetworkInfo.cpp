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
#if !defined(_WIN32) && !defined(_WIN64)
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#else
#include <ws2tcpip.h>
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

#if !defined(_WIN32) && !defined(_WIN64)
	const unsigned int IFLISTSIZ = 25U;

	FILE* fp = ::fopen("/proc/net/route" , "r");
	if (fp == NULL) {
		LogError("Unabled to open /proc/route");
		return;
	}

	char* dflt = NULL;

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

	if (dflt == NULL) {
		LogError("Unable to find the default route");
		return;
	}

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
			} else {
				::sprintf(interfacelist[ifnr], "%s:%s", ifa->ifa_name, host);
				LogInfo("    IPv6: %s", interfacelist[ifnr]);
			}

			ifnr++;
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
#else
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
