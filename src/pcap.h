/**************************************************************************
 **
 ** sngrep - SIP callflow viewer using ngrep
 **
 ** Copyright (C) 2013 Ivan Alonso (Kaian)
 ** Copyright (C) 2013 Irontec SL. All rights reserved.
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
 * @file pcap.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage pcap files
 *
 * sngrep can parse a pcap file to display call flows.
 * This file include the functions that uses libpcap to do so.
 *
 * @todo We could request libpcap to filter the file before being processed
 * and only read sip packages. We also allow UDP packages here, and SIP can
 * use other transports, uh.
 *
 */
#ifndef __SNGREP_PCAP_H
#define __SNGREP_PCAP_H

#include <pcap.h>
#include <pcap/pcap.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <time.h>

//! Ethernet headers are always exactly 14 bytes
#define SIZE_ETHERNET 14
//! Linux cooked packages headers are 16 bytes
#define SLL_HDR_LEN 16
//! UDP  headers are always exactly 8 bytes
#define SIZE_UDP 8

/**
 * @brief IP data structure
 */
struct nread_ip
{
    //! header length, version
    u_int8_t ip_vhl;
    //! type of service
    u_int8_t ip_tos;
    //! total length
    u_int16_t ip_len;
    //! identification
    u_int16_t ip_id;
    //! fragment offset field
    u_int16_t ip_off;
    //! reserved fragment flag
#define IP_RF 0x8000
    //! dont fragment flag
#define IP_DF 0x4000
    //! more fragments flag
#define IP_MF 0x2000
    //!  mask for fragmenting bits
#define IP_OFFMASK 0x1fff
    //! time to live
    u_int8_t ip_ttl;
    //! protocol
    u_int8_t ip_p;
    //! checksum
    u_int16_t ip_sum;
    //! source and dest addresses
    struct in_addr ip_src, ip_dst;
};

#define IP_HL(ip)               (((ip)->ip_vhl) & 0x0f)
#define IP_V(ip)                (((ip)->ip_vhl) >> 4)

/**
 * @brief UDP data structure
 */
struct nread_udp
{
    //! source port
    u_short udp_sport;
    //! destination port
    u_short udp_dport;
    //! UDP header length
    u_short udp_hlen;
    //! UDP Checksum
    u_short udp_chksum;
};

#ifndef WITH_NGREP
/**
 * @brief Capture in background using libpcap functions
 *
 * This function is used as worker thread for capturing filtered packages and
 * pass them to the UI layer. We only use this if ngrep is not available
 * for capturing, becuause it has a lot more options.
 *
 * @param pargv Filters for libpcap
 * @return 0 on spawn success, 1 otherwise
 */
extern int
online_capture(void *pargv);

#endif

/**
 * @brief Read from pcap file and fill sngrep sctuctures
 *
 * This function will use libpcap files and previous structures to 
 * parse the pcap file.
 * This program is only focused in VoIP calls so we only consider
 * TCP/UDP packets with Ethernet or Linux coocked headers
 * 
 * @param file Full path to PCAP file
 * @return 0 if load has been successfull, 1 otherwise
 *
 */
extern int
load_from_file(const char* file);

/**
 * @brief Read the next package and parse SIP messages
 *
 * This function is shared between online and offline capture
 * methods using pcap. This will get the payload from a package and
 * add it to the SIP storage layer.
 *
 * @param handle LIBPCAP capture handler
 */
extern void
parse_packet(u_char *mode, const struct pcap_pkthdr *header, const u_char *packet);

#endif
