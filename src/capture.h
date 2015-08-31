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

#include "config.h"
#include <pthread.h>
#include <pcap.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifndef __FAVOR_BSD
#define __FAVOR_BSD
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE 1
#endif

#if defined (__OpenBSD__)
#define timeval bpf_timeval
#endif

#if defined(BSD) || defined (__OpenBSD__) || defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#endif

#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include "vector.h"

#ifdef INET6_ADDRSTRLEN
#define ADDRESSLEN INET6_ADDRSTRLEN + 1
#else
#define ADDRESSLEN 47
#endif

//! Capture modes
enum capture_status {
    CAPTURE_ONLINE = 0,
    CAPTURE_ONLINE_PAUSED,
    CAPTURE_OFFLINE,
    CAPTURE_OFFLINE_LOADING,
};

//! Shorter declaration of capture_info structure
typedef struct capture_info capture_info_t;
//! Shorter declaration of dns_cache structure
typedef struct dns_cache dns_cache_t;
//! Shorter declaration of capture_packet structure
typedef struct capture_packet capture_packet_t;

//! Stored packet types
enum capture_packet_type {
    CAPTURE_PACKET_SIP_UDP = 0,
    CAPTURE_PACKET_SIP_TCP,
    CAPTURE_PACKET_SIP_TLS,
    CAPTURE_PACKET_SIP_WS,
    CAPTURE_PACKET_SIP_WSS,
    CAPTURE_PACKET_RTP,
};

/**
 * @brief Storage for DNS resolved ips
 *
 * Structure to store resolved addresses when capture.lookup
 * configuration option is enabled.
 */
struct dns_cache {
    int count;
    char addr[ADDRESSLEN][256];
    char hostname[16][256];
};

/**
 * @brief store all information related with packet capture
 *
 * All required information to capture and process packets
 * will be stored in this
 */
struct capture_info {
    //! Capture status
    int status;
    //! Calls capture limit. 0 for disabling
    int limit;
    //! Also capture RTP packets
    int rtp_capture;
    //! Input file in Offline capture
    const char *infile;
    //! Key file for TLS decrypt
    const char *keyfile;
    //! The compiled filter expression
    struct bpf_program fp;
    //! Netmask of our sniffing device
    bpf_u_int32 mask;
    //! The IP of our sniffing device
    bpf_u_int32 net;
    //! libpcap capture handler
    pcap_t *handle;
    //! libpcap dump file handler
    pcap_dumper_t *pd;
    //! libpcap link type
    int link;
    //! libpcap link header size
    int8_t link_hl;
    //! Cache for DNS lookups
    dns_cache_t dnscache;
    //! Local devices pointer
    pcap_if_t *devices;
    //! Capture thread for online capturing
    pthread_t capture_t;
};

struct capture_packet {
    // Packet type as defined in capture_packet_type
    int type;
    //! PCAP Packet Header data
    struct pcap_pkthdr *header;
    //! PCAP Packet content
    u_char *data;
    //! PCAP Packet payload when it can not be get from data
    u_char *payload;
    //! PPCAP Packet content len
    uint32_t size;
    //! Payload length
    uint32_t payload_len;
};

/**
 * @brief Online capture function
 *
 * @param device Device to start capture from
 * @param outfile Dumpfile for captured packets
 *
 * @return 0 on spawn success, 1 otherwise
 */
int
capture_online(const char *dev, const char *outfile);

/**
 * @brief Read from pcap file and fill sngrep sctuctures
 *
 * This function will use libpcap files and previous structures to
 * parse the pcap file.
 *
 * @param infile File to read packets from
 *
 * @return 0 if load has been successfull, 1 otherwise
 */
int
capture_offline(const char *infile, const char *outfile);

/**
 * @brief Read the next package and parse SIP messages
 *
 * This function is shared between online and offline capture
 * methods using pcap. This will get the payload from a package and
 * add it to the SIP storage layer.
 *
 * @param handle LIBPCAP capture handler
 */
void
parse_packet(u_char *mode, const struct pcap_pkthdr *header, const u_char *packet);

/**
 * @brief Create a capture thread for online mode
 *
 * @return 0 on success, 1 otherwise
 */
int
capture_launch_thread();

/**
 * @brief PCAP Capture Thread
 *
 * This function is used as worker thread for capturing filtered packets and
 * pass them to the UI layer.
 */
void
capture_thread(void *none);

/**
 * @brief Check if capture is in Online mode
 *
 * @return 1 if capture is online, 0 if offline
 */
int
capture_is_online();

/**
 * @brief Set a bpf filter in open capture
 *
 * @param filter String containing the BPF filter text
 * @return 0 if valid, 1 otherwise
 */
int
capture_set_bpf_filter(const char *filter);

/**
 * @brief Set capture options
 *
 * @param limit Numbers of calls >0
 * @param rtp_catpure Enable rtp capture
 */
void
capture_set_opts(int limit, int rtp_capture);

/**
 * @brief Pause/Resume capture
 *
 * @param pause 1 to pause capture, 0 to resume
 */
void
capture_set_paused(int pause);

/**
 * @brief Check if capture is actually running
 *
 * @return 1 if capture is paused, 0 otherwise
 */
int
capture_is_paused();

/**
 * @brief Get capture status value
 */
int
capture_get_status();

/**
 * @brief Return a string representing current capture status
 */
const char *
capture_get_status_desc();

/**
 * @brief Get Input file from Offline mode
 *
 * @return Input file in Offline mode
 * @return NULL in Online mode
 */
const char*
capture_get_infile();

/**
 * @brief Get Key file from decrypting TLS packets
 *
 * @return given keyfile
 */
const char*
capture_get_keyfile();

/**
 * @brief Set Keyfile to decrypt TLS packets
 *
 * @param keyfile Full path to keyfile
 */
void
capture_set_keyfile(const char *keyfile);

/**
 * @brief Return the last capture error
 */
char *
capture_last_error();

/**
 * @brief Allocate memory to store new packet data
 */
capture_packet_t *
capture_packet_create(const struct pcap_pkthdr *header, const u_char *packet, int size);

/**
 * @brief Deallocate a packet structure memory
 */
void
capture_packet_destroy(capture_packet_t *packet);

/**
 * @brief Destroyer function for packet vectors
 */
void
capture_packet_destroyer(void *packet);

/**
 * @brief Set packet type
 */
void
capture_packet_set_type(capture_packet_t *packet, int type);

/**
 * @brief Set packet payload when it can not be get from packet
 */
void
capture_packet_set_payload(capture_packet_t *packet, u_char *payload, uint32_t payload_len);

/**
 * @brief Getter for capture payload size
 */
uint32_t
capture_packet_get_payload_len(capture_packet_t *packet);

/**
 * @brief Getter for capture payload pointer
 */
u_char *
capture_packet_get_payload(capture_packet_t *packet);

/**
 * @brief Sorter by time for captured packets
 */
void
capture_packet_time_sorter(vector_t *vector, void *item);

/**
 * @brief Close pcap handler
 */
void
capture_close();

/**
 * @brief Get datalink header size
 *
 */
int8_t
datalink_size(int datalink);

/**
 * @brief Open a new dumper file for capture handler
 */
pcap_dumper_t *
dump_open(const char *dumpfile);

/**
 * @brief Store a packet in dump file
 *
 * File must be previously opened with dump_open
 */
void
dump_packet(pcap_dumper_t *pd, const struct pcap_pkthdr *header, const u_char *packet);

/**
 * @brief Close a dump file
 */
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
lookup_hostname(const char *address);

/**
 * @brief Check if a given address belongs to a local device
 *
 * @param address IPv4 string format for address
 * @return 1 if address is local, 0 otherwise
 */
int
is_local_address_str(const char *address);

/**
 * @brief Check if a given address belongs to a local device
 *
 * @param address IPv4 format for address
 * @return 1 if address is local, 0 otherwise
 */
int
is_local_address(in_addr_t address);

#endif
