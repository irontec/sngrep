/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2019 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2019 Irontec SL. All rights reserved.
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
#include <string.h>
#include <pcap.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "glib-extra/glib.h"
#include "address.h"

gboolean
addressport_equals(const Address addr1, const Address addr2)
{
    if (addr1.ip == NULL && addr2.ip == NULL) {
        return TRUE;
    }

    return addr1.port == addr2.port && g_strcmp0(addr1.ip, addr2.ip) == 0;
}

gboolean
address_equals(const Address addr1, const Address addr2)
{
    if (addr1.ip == NULL && addr2.ip == NULL) {
        return TRUE;
    }

    return g_strcmp0(addr1.ip, addr2.ip) == 0;
}

gboolean
address_is_local(const Address addr)
{
    //! Local devices pointer
    static pcap_if_t *devices = 0;
    pcap_if_t *dev;
    pcap_addr_t *da;
    gchar errbuf[PCAP_ERRBUF_SIZE];
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
        for (da = dev->addresses; da; da = da->next) {
            // Ignore empty addresses
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
                default:
                    continue;
            }

            // Check if this address matches
            if (g_strcmp0(addr.ip, ip) == 0) {
                return TRUE;
            }

        }
    }
    return FALSE;
}

Address
address_from_str(const gchar *address)
{
    Address addr = ADDRESS_ZERO;
    if(address == NULL) {
        return addr;
    }

    g_auto(GStrv) ip_port_data = g_strsplit(address, ":", 2);
    switch (g_strv_length(ip_port_data)) {
        case 1:
            addr = address_new(ip_port_data[0], 0);
            break;
        case 2:
            addr = address_new(ip_port_data[0], g_atoi(ip_port_data[1]));
            break;
        default:
            return addr;
    }

    return addr;
}

const gchar *
address_get_ip(Address address)
{
    return address.ip;
}

guint16
address_get_port(Address address)
{
    return address.port;
}

guint
address_get_ip_len(Address address)
{
    if (address.ip == NULL)
        return 0;
    return strlen(address.ip);
}

Address
address_strip_port(Address address)
{
    address.port = 0;
    return address;
}

void
address_free(Address address)
{
    g_free(address.ip);
}

Address
address_new(const gchar *ip, guint16 port)
{
    Address address = ADDRESS_ZERO;
    address.ip = g_strdup(ip);
    address.port = port;
    return address;
}
