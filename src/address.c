/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2018 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2018 Irontec SL. All rights reserved.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 ****************************************************************************/
/**
 * @file address.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in address.h
 *
 */

#include "config.h"
#include "address.h"
#include <string.h>
#include <pcap.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

bool
addressport_equals(address_t addr1, address_t addr2)
{
    return addr1.port == addr2.port && !strcmp(addr1.ip, addr2.ip);
}

bool
address_equals(address_t addr1, address_t addr2)
{
    return !strcmp(addr1.ip, addr2.ip);
}

bool
address_is_local(address_t addr)
{
    //! Local devices pointer
    static pcap_if_t *devices = 0;
    pcap_if_t *dev;
    pcap_addr_t *da;
    char errbuf[PCAP_ERRBUF_SIZE];
    struct sockaddr_in *ipaddr;
#ifdef USE_IPV6
    struct sockaddr_in6 *ip6addr;
#endif
    char ip[ADDRESSLEN];

    // Get all network devices
    if (!devices) {
        // Get Local devices addresses
        pcap_findalldevs(&devices, errbuf);
    }

    for (dev = devices; dev; dev = dev->next) {
        for (da = dev->addresses; da ; da = da->next) {
            // Ingore empty addresses
            if (!da->addr)
                continue;

            // Initialize variables
            memset(ip, 0, sizeof(ip));

            // Get address representation
            switch (da->addr->sa_family) {
            case AF_INET:
                ipaddr = (struct sockaddr_in *) da->addr;
                inet_ntop(AF_INET, &ipaddr->sin_addr, ip, sizeof(ip));
                break;
#ifdef USE_IPV6
            case AF_INET6:
                ip6addr = (struct sockaddr_in6 *) da->addr;
                inet_ntop(AF_INET, &ip6addr->sin6_addr, ip, sizeof(ip));
                break;
#endif
            }

            // Check if this address matches
            if (!strcmp(addr.ip, ip)) {
                return true;
            }

        }
    }
    return false;
}

address_t
address_from_str(const char *ipport)
{
    address_t ret = {};
    char scanipport[256];
    char address[256];
    int port;

    if (!ipport || strlen(ipport) > ADDRESSLEN + 6)
        return ret;

    strncpy(scanipport, ipport, sizeof(scanipport));

    if (sscanf(scanipport, "%[^:]:%d", address, &port) == 2) {
        strncpy(ret.ip, address, sizeof(ret.ip));
        ret.port = port;
    }

    return ret;
}
