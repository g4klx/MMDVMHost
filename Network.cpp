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

#include "Network.h"
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

void CNetworkInfo::getNetworkInterface(unsigned char* info)
{
	LogInfo("Interfaces Info");
	::strcpy((char*)info, "(address unknown)");


#if !defined(_WIN32) && !defined(_WIN64)

#define IFLISTSIZ 25

	struct ifaddrs *ifaddr, *ifa;
	int family, s, n, ifnr;
	char host[NI_MAXHOST];
	char interfacelist[IFLISTSIZ][50+INET6_ADDRSTRLEN];
	char *dflt, *p;
	FILE *f;
	char line[100U];

	dflt=NULL;
	f = fopen("/proc/net/route" , "r");
	while(fgets(line , 100U , f)) {
		dflt = strtok(line , " \t");
		p = strtok(NULL , " \t");
		if(dflt!=NULL && p!=NULL) {
			if(strcmp(p , "00000000") == 0) { break; }
		}
	}
	fclose(f);


	for(n=0;n<IFLISTSIZ;n++) {
		interfacelist[n][0]=0;
	}
	ifnr=0;
	if (getifaddrs(&ifaddr) == -1) {
		LogError("getifaddrs failure");
	} else {
		for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
			if (ifa->ifa_addr == NULL) continue;
			family = ifa->ifa_addr->sa_family;
			if (family == AF_INET || family == AF_INET6) {
				s = getnameinfo(ifa->ifa_addr,(family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
				if (s != 0) {
					LogError("getnameinfo() failed: %s\n", gai_strerror(s));
					continue;
				}
				if (family == AF_INET) {
					sprintf(interfacelist[ifnr], "%s: %s", ifa->ifa_name,host);
					LogInfo("    IPv4: %s", interfacelist[ifnr] );
				} else {
					sprintf(interfacelist[ifnr], "%s: %s", ifa->ifa_name,host);
					LogInfo("    IPv6: %s", interfacelist[ifnr] );
				}
				ifnr++;
			}
		}
		freeifaddrs(ifaddr);

		LogInfo("    Default interface is : %s" , dflt);

		for(n=0;n<(ifnr);n++) {
			p=strchr(interfacelist[n],'%');
			if (p!=NULL) *p=0;
			if(strstr(interfacelist[n], dflt) != 0) {
				::strcpy((char*)info,interfacelist[n]);
				break;
			}
		}
		LogInfo("    IP to show: %s", info );
	}
#else
	PMIB_IPFORWARDTABLE pIpForwardTable;
	DWORD dwSize = 0;
	DWORD dwRetVal = 0;
	int i, ifnr;

	pIpForwardTable = (MIB_IPFORWARDTABLE *)malloc(sizeof(MIB_IPFORWARDTABLE));
	if (pIpForwardTable == NULL) {
		LogError("Error allocating memory");
		return;
	}
	if (GetIpForwardTable(pIpForwardTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
		free(pIpForwardTable);
		pIpForwardTable = (MIB_IPFORWARDTABLE *)malloc(dwSize);
		if (pIpForwardTable == NULL) {
			LogError("Error allocating memory");
			return;
		}
	}
	if ((dwRetVal = GetIpForwardTable(pIpForwardTable, &dwSize, 0)) == NO_ERROR) {
		for (i = 0; i < (int)pIpForwardTable->dwNumEntries; i++) {
			if (pIpForwardTable->table[i].dwForwardDest == 0) break;
		}
		ifnr = pIpForwardTable->table[i].dwForwardIfIndex;
		free(pIpForwardTable);

		PIP_ADAPTER_INFO pAdapterInfo;
		pAdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));
		ULONG buflen = sizeof(IP_ADAPTER_INFO);

		if (GetAdaptersInfo(pAdapterInfo, &buflen) == ERROR_BUFFER_OVERFLOW) {
			free(pAdapterInfo);
			pAdapterInfo = (IP_ADAPTER_INFO *)malloc(buflen);
			if (pAdapterInfo == NULL) {
				LogError("Error allocating memory");
				return;
			}
		}
		if (GetAdaptersInfo(pAdapterInfo, &buflen) == NO_ERROR) {
			PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
			while (pAdapter) {
				LogInfo("    IP  : %s", pAdapter->IpAddressList.IpAddress.String);
				if (pAdapter->Index == ifnr) {
					::strcpy((char*)info, pAdapter->IpAddressList.IpAddress.String);
				}
				pAdapter = pAdapter->Next;
			}
			LogInfo("    IP to show: %s", info);

		}
		else {
			LogError("Call to GetAdaptersInfo failed.");
		}
		return;
	}
	else {
		LogError("GetIpForwardTable failed.");
		free(pIpForwardTable);
		return;
	}
#endif
}
