/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013,2014 Ivan Alonso (Kaian)
 ** Copyright (C) 2013,2014 Irontec SL. All rights reserved.
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
 * @file capture.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage pcap files
 *
 * sngrep can parse a pcap file to display call flows.
 * This file include the functions that uses libpcap to do so.
 *
 */
#ifndef __SNGREP_CAPTURE_H
#define __SNGREP_CAPTURE_H

#include <pcap.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <time.h>

//! UDP headers are always exactly 8 bytes
#define SIZE_UDP 8
//! TCP headers size
#define SIZE_TCP TH_OFF(tcp)*4

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

/* TCP header */
typedef u_int tcp_seq;

struct nread_tcp
{
    u_short th_sport; /* source port */
    u_short th_dport; /* destination port */
    tcp_seq th_seq; /* sequence number */
    tcp_seq th_ack; /* acknowledgement number */
    u_char th_offx2; /* data offset, rsvd */
#define TH_OFF(th)      (((th)->th_offx2 & 0xf0) >> 4)
    u_char th_flags;
#define TH_FIN  0x01
#define TH_SYN  0x02
#define TH_RST  0x04
#define TH_PUSH 0x08
#define TH_ACK  0x10
#define TH_URG  0x20
#define TH_ECE  0x40
#define TH_CWR  0x80
#define TH_FLAGS        (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
    u_short th_win; /* window */
    u_short th_sum; /* checksum */
    u_short th_urp; /* urgent pointer */
};

/**
 * @brief Storage for DNS resolved ips
 *
 * Structure to store resolved addresses when capture.lookup
 * configuration option is enabled.
 */
struct dns_cache
{
    int count;
    char addr[16][256];
    char hostname[16][256];
};

/**
 * @brief Online capture function
 *
 * This function will validate capture options but will not capture any packet:
 * - Capture device
 * - BPF Filter
 *
 * @return 0 on spawn success, 1 otherwise
 */
extern int
capture_online();

/**
 * @brief PCAP Capture Thread
 *
 * This function is used as worker thread for capturing filtered packets and
 * pass them to the UI layer.
 */
extern void
capture_thread(void *none);

/**
 * @brief Read from pcap file and fill sngrep sctuctures
 *
 * This function will use libpcap files and previous structures to
 * parse the pcap file.
 * This program is only focused in VoIP calls so we only consider
 * TCP/UDP packets with Ethernet or Linux coocked headers
 *
 * @return 0 if load has been successfull, 1 otherwise
 *
 */
extern int
capture_offline();

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

void
capture_close();

/**
 * @brief Get datalink header size
 *
 */
extern int
datalink_size(int datalink);

pcap_dumper_t *
dump_open(const char *dumpfile);

void
dump_packet(pcap_dumper_t *pd, const struct pcap_pkthdr *header, const u_char *packet);

void
dump_close(pcap_dumper_t *pd);

/**
 * @brief Try to get hostname from its address
 *
 * Try to get hostname from the given address, store it in
 * dnscache. If hostname can not be resolved, then store the
 * original address to avoid lookup again the same address.
 */
const char *
lookup_hostname(struct in_addr *addr);

#endif
