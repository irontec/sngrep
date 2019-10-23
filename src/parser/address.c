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
#include "glib/glib-extra.h"
#include "address.h"

gboolean
addressport_equals(const Address *addr1, const Address *addr2)
{
    if (addr1 == NULL && addr2 == NULL) {
        return TRUE;
    }

    if (addr1 == NULL || addr2 == NULL) {
        return FALSE;
    }

    return addr1->port == addr2->port && g_strcmp0(addr1->ip, addr2->ip) == 0;
}

gboolean
address_equals(const Address *addr1, const Address *addr2)
{
    if (addr1 == NULL && addr2 == NULL) {
        return TRUE;
    }

    if (addr1 == NULL || addr2 == NULL) {
        return FALSE;
    }

    return g_strcmp0(addr1->ip, addr2->ip) == 0;
}

gboolean
address_is_local(const Address *addr)
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
            if (g_strcmp0(addr->ip, ip) == 0) {
                return TRUE;
            }

        }
    }
    return FALSE;
}

Address *
address_from_str(const gchar *address)
{
    if (address == NULL)
        return NULL;

    g_auto(GStrv) ip_port_data = g_strsplit(address, ":", 2);
    switch (g_strv_length(ip_port_data)) {
        case 1:
            return address_new(ip_port_data[0], 0);
        case 2:
            return address_new(ip_port_data[0], g_atoi(ip_port_data[1]));
        default:
            return NULL;
    }
}

Address *
address_clone(const Address *address)
{
    return address_new(address->ip, address->port);
}

void
address_free(Address *address)
{
    if (address == NULL)
        return;

    g_free(address->ip);
    g_free(address);
}

Address *
address_new(const gchar *ip, guint16 port)
{
    Address *address = g_malloc0(sizeof(Address));
    address->ip = g_strdup(ip);
    address->port = port;
    return address;
}
