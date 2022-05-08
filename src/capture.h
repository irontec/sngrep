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
#include "address.h"

#ifndef __FAVOR_BSD
#define __FAVOR_BSD
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE 1
#endif

#if defined(BSD) || defined (__OpenBSD__) || defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#endif

#ifdef USE_IPV6
#include <netinet/ip6.h>
#endif

#ifdef SLL_HDR_LEN
#include <pcap/sll.h>
#endif

#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <stdbool.h>
#include "packet.h"
#include "vector.h"

//! Max allowed packet assembled size
#define MAX_CAPTURE_LEN 20480
//! Max allowed packet length
#define MAXIMUM_SNAPLEN 262144

//! Define VLAN 802.1Q Ethernet type
#ifndef ETHERTYPE_8021Q
#define ETHERTYPE_8021Q 0x8100
#endif

//! NFLOG Support (for libpcap <1.6.0)
#define DLT_NFLOG       239
#define NFULA_PAYLOAD   9

typedef struct nflog_tlv {
    u_int16_t   tlv_length;
    u_int16_t   tlv_type;
} nflog_tlv_t;

//! Define Websocket Transport codes
#define WH_FIN      0x80
#define WH_RSV      0x70
#define WH_OPCODE   0x0F
#define WH_MASK     0x80
#define WH_LEN      0x7F
#define WS_OPCODE_TEXT 0x1

enum capture_storage {
    CAPTURE_STORAGE_NONE = 0,
    CAPTURE_STORAGE_MEMORY,
    CAPTURE_STORAGE_DISK
};

//! Shorter declaration of capture_config structure
typedef struct capture_config capture_config_t;
//; Shorter declaration of capture_info structure
typedef struct capture_info capture_info_t;

/**
 * @brief Capture common configuration
 *
 * Store capture configuration and global data
 */
struct capture_config {
    //! Calls capture limit. 0 for disabling
    size_t limit;
    //! Set size of pcap buffer
    size_t pcap_buffer_size;
    //! Also capture RTP packets
    bool rtp_capture;
    //! Rotate capturad dialogs when limit have reached
    bool rotate;
    //! Capture sources are paused (all packets are skipped)
    int paused;
    //! Where should we store captured packets
    enum capture_storage storage;
    //! Key file for TLS decrypt
    const char *keyfile;
    //! TLS Server address
    address_t tlsserver;
    //! capture filter expression text
    const char *filter;
    //! The compiled filter expression
    struct bpf_program fp;
    //! libpcap dump file handler
    pcap_dumper_t *pd;
    //! libpcap dump file name
    const char *dumpfilename;
    //! inode of the dump file we have open
    ino_t dump_inode;
    //! Capture sources
    vector_t *sources;
    //! Capture Lock. Avoid parsing and handling data at the same time
    pthread_mutex_t lock;
};

/**
 * @brief store all information related with packet capture
 *
 * Store capture required data from one packet source
 */
struct capture_info
{
    //! Flag to determine if capture is running
    bool running;
    //! Flag to determine if this capture is libpcap
    bool ispcap;
    //! libpcap link type
    int link;
    //! libpcap link header size
    int8_t link_hl;
    //! libpcap capture handler
    pcap_t *handle;
    //! Netmask of our sniffing device
    bpf_u_int32 mask;
    //! The IP of our sniffing device
    bpf_u_int32 net;
    //! Input file in Offline capture
    const char *infile;
    //! Capture device in Online mode
    const char *device;
    //! Packets pending IP reassembly
    vector_t *ip_reasm;
    //! Packets pending TCP reassembly
    vector_t *tcp_reasm;
    //! Capture thread function
    void *(*capture_fn)(void *data);
    //! Capture thread for online capturing
    pthread_t capture_t;
};

/**
 * @brief Initialize capture data
 *
 * @param limit Numbers of calls >0
 * @param rtp_catpure Enable rtp capture
 * @param rotate Enable capture rotation
 * @param set size of pcap buffer
 */
void
capture_init(size_t limit, bool rtp_capture, bool rotate, size_t pcap_buffer_size);

/**
 * @brief Deinitialize capture data
 */
void
capture_deinit();

/**
 * @brief Online capture function
 *
 * @param device Device to start capture from
 *
 * @return 0 on spawn success, 1 otherwise
 */
int
capture_online(const char *dev);

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
capture_offline(const char *infile);

/**
 * @brief Read the next package and parse SIP messages
 *
 * This function is shared between online and offline capture
 * methods using pcap. This will get the payload from a package and
 * add it to the SIP storage layer.
 *
 */
void
parse_packet(u_char *capinfo, const struct pcap_pkthdr *header, const u_char *packet);

/**
 * @brief Reassembly capture IP fragments
 *
 * This function will try to assemble received PCAP data into a single IP packet.
 * It will return a packet structure if no fragmentation is found or a full packet
 * has been assembled.
 *
 * @note We assume packets higher than MAX_CAPTURE_LEN won't be SIP. This has been
 * done to avoid reassembling too big packets, that aren't likely to be interesting
 * for sngrep.
 *
 * TODO
 * Assembly only works when all of the IP fragments are received in the good order.
 * Properly check memory boundaries during packet reconstruction.
 * Implement a way to timeout pending IP fragments after some time.
 * TODO
 *
 * @param capinfo Packet capture session information
 * @para header Header received from libpcap callback
 * @para packet Packet contents received from libpcap callback
 * @param size Packet size (not including Layer and Network headers)
 * @param caplen Full packet size (current fragment -> whole assembled packet)
 * @return a Packet structure when packet is not fragmented or fully reassembled
 * @return NULL when packet has not been completely assembled
 */
packet_t *
capture_packet_reasm_ip(capture_info_t *capinfo, const struct pcap_pkthdr *header,
                        u_char *packet, uint32_t *size, uint32_t *caplen);

/**
 * @brief Reassembly capture TCP segments
 *
 * This function will try to assemble TCP segments of an existing packet.
 *
 * @note We assume packets higher than MAX_CAPTURE_LEN won't be SIP. This has been
 * done to avoid reassembling too big packets, that aren't likely to be interesting
 * for sngrep.
 *
 * @param packet Capture packet structure
 * @param tcp TCP header extracted from capture packet data
 * @param payload Assembled TCP packet payload content
 * @param size_payload Payload length
 * @return a Packet structure when packet is not segmented or fully reassembled
 * @return NULL when packet has not been completely assembled
 */
packet_t *
capture_packet_reasm_tcp(capture_info_t *capinfo, packet_t *packet, struct tcphdr *tcp,
                         u_char *payload, int size_payload);

/**
 * @brief Check if given payload belongs to a Websocket connection
 *
 * Parse the given payload and determine if given payload could belong
 * to a Websocket packet. This function will change the payload pointer
 * apnd size content to point to the SIP payload data.
 *
 * @return 0 if packet is websocket, 1 otherwise
 */
int
capture_ws_check_packet(packet_t *packet);

/**
 * @brief Check if the given packet structure is SIP/RTP/..
 *
 * This function will call parse functions to determine if packet has relevant data
 *
 * @return 0 in case this packets has SIP/RTP data
 * @return 1 otherwise
 */
int
capture_packet_parse(packet_t *pkt);

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
void *
capture_thread(void *none);

/**
 * @brief Check if capture is in Online mode
 *
 * @return 1 if capture is online, 0 if offline
 */
int
capture_is_online();

/**
 * @brief Check if at least one capture handle is opened
 *
 * @return 1 if any capture source is running, 0 if all ended
 */
int
capture_is_running();

/**
 * @brief Set a bpf filter in open capture
 *
 * @param filter String containing the BPF filter text
 * @return 0 if valid, 1 otherwise
 */
int
capture_set_bpf_filter(const char *filter);

/**
 * @brief Get the configured BPF filter
 *
 * @return String containing the BPF filter text or NULL
 */
const char *
capture_get_bpf_filter();

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
bool
capture_paused();

/**
 * @brief Get capture status value
 */
int
capture_status();

/**
 * @brief Return a string representing current capture status
 */
const char *
capture_status_desc();

/**
 * @brief Get Input file from Offline mode
 *
 * @return Input file in Offline mode
 * @return NULL in Online mode
 */
const char*
capture_input_file();

/**
 * @brief Get Device interface from Online mode
 *
 * @return Device name used to capture packets
 * @return NULL in Offline or Mixed mode
 */
const char *
capture_device();

/**
 * @brief Get Key file from decrypting TLS packets
 *
 * @return given keyfile
 */
const char*
capture_keyfile();

/**
 * @brief Set Keyfile to decrypt TLS packets
 *
 * @param keyfile Full path to keyfile
 */
void
capture_set_keyfile(const char *keyfile);

/**
 * @brief Get TLS Server address if configured
 * @return address scructure
 */
address_t
capture_tls_server();

/**
 * @brief Add new source to capture list
 */
void
capture_add_source(struct capture_info *capinfo);

/**
 * @brief Return packet catprue sources count
 * @return capture sources count
 */
int
capture_sources_count();

/**
 * @brief Return the last capture error
 */
char *
capture_last_error();

/**
 * @brief Avoid parsing more packets
 */
void
capture_lock();

/**
 * @brief Allow parsing more packets
 */
void
capture_unlock();

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
 * @brief Set general capture dumper
 */
void
capture_set_dumper(pcap_dumper_t *dumper, ino_t dump_inode);

/**
 * @brief Store a packet in dumper file
 */
void
capture_dump_packet(packet_t *packet);

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
dump_open(const char *dumpfile, ino_t* dump_inode);

/**
 * @brief Store a packet in dump file
 *
 * File must be previously opened with dump_open
 */
void
dump_packet(pcap_dumper_t *pd, const packet_t *packet);

/**
 * @brief Close a dump file
 */
void
dump_close(pcap_dumper_t *pd);

/**
 * @brief Check if a given address belongs to a local device
 *
 * @param address IPv4 format for address
 * @return 1 if address is local, 0 otherwise
 */
int
is_local_address(in_addr_t address);

#endif
