/**************************************************************************
 **
 **  sngrep - Ncurses ngrep interface for SIP
 **
 **   Copyright (C) 2013 Ivan Alonso (Kaian)
 **   Copyright (C) 2013 Irontec SL. All rights reserved.
 **
 **   This program is free software: you can redistribute it and/or modify
 **   it under the terms of the GNU General Public License as published by
 **   the Free Software Foundation, either version 3 of the License, or
 **   (at your option) any later version.
 **
 **   This program is distributed in the hope that it will be useful,
 **   but WITHOUT ANY WARRANTY; without even the implied warranty of
 **   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **   GNU General Public License for more details.
 **
 **   You should have received a copy of the GNU General Public License
 **   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 ****************************************************************************/
#ifndef __SNGREP_PCAP_H
#define __SNGREP_PCAP_H

#include <pcap.h>
#include <pcap/pcap.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <time.h>

/* Ethernet headers are always exactly 14 bytes */
#define SIZE_ETHERNET 14
/* Linux cooked packages headers are 16 bytes */
#define SLL_HDR_LEN 16
/* UDP  headers are always exactly 8 bytes */
#define SIZE_UDP 8

/* IP data structure */
struct nread_ip
{
    u_int8_t ip_vhl; /* header length, version    */
    u_int8_t ip_tos; /* type of service           */
    u_int16_t ip_len; /* total length              */
    u_int16_t ip_id; /* identification            */
    u_int16_t ip_off; /* fragment offset field     */
#define IP_RF 0x8000                 /* reserved fragment flag    */
#define IP_DF 0x4000                 /* dont fragment flag        */
#define IP_MF 0x2000                 /* more fragments flag       */
#define IP_OFFMASK 0x1fff            /* mask for fragmenting bits */
    u_int8_t ip_ttl; /* time to live              */
    u_int8_t ip_p; /* protocol                  */
    u_int16_t ip_sum; /* checksum                  */
    struct in_addr ip_src, ip_dst; /* source and dest address   */
};

#define IP_HL(ip)               (((ip)->ip_vhl) & 0x0f)
#define IP_V(ip)                (((ip)->ip_vhl) >> 4)

/* UDP data structure */
struct nread_udp
{
    u_short udp_sport; /* source port */
    u_short udp_dport; /* destination port */
    u_short udp_hlen; /* Udp header length*/
    u_short udp_chksum; /* Udp Checksum */
};

/**
 * Read from pcap file and fill sngrep sctuctures
 * This function will use libpcap files and previous structures to 
 * parse the pcap file.
 * This program is only focused in VoIP calls so we only consider
 * TCP/UDP packets with Ethernet or Linux coocked headers
 * 
 * @param file Full path to PCAP file
 * @return 0 if load has been successfull, 1 otherwise
 *
 */
int load_pcap_file(const char* file);

#endif
