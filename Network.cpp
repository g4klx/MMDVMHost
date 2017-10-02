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
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


void CNetworkInfo::getNetworkInterface(unsigned char* info)
{
#define IFLISTSIZ 25

        LogInfo("Interfaces Info");
        struct ifaddrs *ifaddr, *ifa;
        int family, s, n, ifnr;
        char host[NI_MAXHOST];
        char text[50+INET6_ADDRSTRLEN];
        char interfacelist[IFLISTSIZ][50+INET6_ADDRSTRLEN];
        char *p;

        strcpy(text,"(interface lookup failed)");
        for(n=0;n<IFLISTSIZ;n++) {
            interfacelist[n][0]=0;
        }
        ifnr=0;

        if (getifaddrs(&ifaddr) == -1) {
            strcpy((char*)info,"getifaddrs failure");
        } else {
            for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
               if (ifa->ifa_addr == NULL)
                   continue;

                family = ifa->ifa_addr->sa_family;

                if (family == AF_INET || family == AF_INET6) {
                    s = getnameinfo(ifa->ifa_addr,
                        (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                              sizeof(struct sockaddr_in6),
                        host, NI_MAXHOST,
                        NULL, 0, NI_NUMERICHOST);
                    if (s != 0) {
                        LogInfo("getnameinfo() failed: %s\n", gai_strerror(s));
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
            for(n=0;n<(ifnr);n++) {
                p=strchr(interfacelist[n],'%');
                if (p!=NULL) *p=0;
                if (strstr(interfacelist[n],"lo")==NULL) {
                    strcpy((char*)info,interfacelist[n]);
                    break;
                }
            }
            LogInfo("    IP to show: %s", info );
        }
}

