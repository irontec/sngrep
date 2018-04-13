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
#ifndef __SNGREP_CAPTURE_PCAP_H
#define __SNGREP_CAPTURE_PCAP_H

#include <glib.h>
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

/* Old versions of libpcap in OpenBSD use <net/bpf.h>
 * which actually defines timestamps as bpf_timeval instead
 * of simple timeval. This no longer happens in newest libpcap
 * versions, where header packets have timestamps in timeval
 * structs */
#if defined (__OpenBSD__) && defined(_NET_BPF_H_)
#define timeval bpf_timeval
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
#include "capture.h"

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

//! Max allowed packet length (for libpcap)
#define MAXIMUM_SNAPLEN 262144
//! Error reporting
#define CAPTURE_PCAP_ERROR (capture_pcap_error_quark())

//! Error codes
enum capture_pcap_errors
{
    CAPTURE_PCAP_ERROR_DEVICE_LOOKUP = 0,
    CAPTURE_PCAP_ERROR_DEVICE_OPEN,
    CAPTURE_PCAP_ERROR_FILE_OPEN,
    CAPTURE_PCAP_ERROR_UNKNOWN_LINK,
    CAPTURE_PCAP_ERROR_FILTER_COMPILE,
    CAPTURE_PCAP_ERROR_FILTER_APPLY,
};

//! Shorter declaration of capture_info structure
typedef struct _CapturePcap CapturePcap;

/**
 * @brief store all information related with packet capture
 *
 * Store capture required data from one packet source
 */
struct _CapturePcap
{
    //! libpcap capture handler
    pcap_t *handle;
    //! Netmask of our sniffing device
    bpf_u_int32 mask;
    //! The IP of our sniffing device
    bpf_u_int32 net;
    //! libpcap link type
    gint link;
    //! libpcap link header size
    int8_t link_hl;
    //! Packets pending IP reassembly
    GSequence *ip_reasm;
    //! Packets pending TCP reassembly
    GSequence *tcp_reasm;
};


/**
 * @brief Get Capture domain struct for GError
 */
GQuark
capture_pcap_error_quark();

/**
 * @brief Online capture function
 *
 * @param device Device to start capture from
 * @param error GError with failure description (optional)
 *
 * @return capture input struct pointer or NULL on failure
 */
CaptureInput *
capture_input_pcap_online(const gchar *dev, GError **error);

/**
 * @brief Read from pcap file and fill sngrep sctuctures
 *
 * @param infile File to read packets from
 * @param error GError with failure description (optional)
 *
 * @return input struct pointer or NULL on failure
 */
CaptureInput *
capture_input_pcap_offline(const gchar *infile, GError **error);

/**
 * @brief PCAP Capture Thread
 *
 * This function is used as worker thread for capturing filtered packets and
 * pass them to the UI layer.
 */
void
capture_input_pcap_start(CaptureInput *input);

/**
 * @brief Close pcap handler
 */
void
capture_input_pcap_stop(CaptureInput *input);

/**
 * @brief Set a bpf filter in open capture
 *
 * @param filter String containing the BPF filter text
 * @return TRUE if valid, FALSE otherwise
 */
gboolean
capture_input_pcap_filter(CaptureInput *input, const gchar *filter, GError **error);

/**
 *
 */
void
capture_input_pcap_set_keyfile(CaptureInput *input, const gchar *keyfile);

/**
 * @brief Open a new dumper file for capture handler
 */
CaptureOutput *
capture_output_pcap(const char *dumpfile, GError **error);

/**
 * @brief Store a packet in dump file
 *
 * File must be previously opened with dump_open
 */
void
capture_output_pcap_dump(CaptureOutput *output, packet_t *packet);

/**
 * @brief Close a dump file
 */
void
capture_output_pcap_close(CaptureOutput *output);

/**
 * @brief Read the next package
 *
 * This function is shared between online and offline capture
 * methods using pcap. This will get the payload from a package and
 * add it to the packet manager.
 */
void
capture_pcap_parse_packet(u_char *input, const struct pcap_pkthdr *header, const guchar *content);

/**
 * @brief Return the last error in the pcap handler
 * @param pcap handler
 * @return a string representing the last error
 */
char *
capture_pcap_error(pcap_t *handle);

/**
 * @brief Sorter by time for captured packets
 */
gint
capture_packet_time_sorter(gconstpointer a, gconstpointer b, gpointer user_data);

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
capture_packet_reasm_ip(CapturePcap *capinfo, const struct pcap_pkthdr *header,
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
capture_packet_reasm_tcp(CapturePcap *capinfo, packet_t *packet, struct tcphdr *tcp,
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
 * @brief Return the last capture error
 */
char *
capture_last_error();

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

/**
 * @brief Get Input file from Offline mode
 *
 * @return Input file in Offline mode
 * @return NULL in Online mode
 */
const gchar*
capture_input_pcap_file(CaptureManager *manager);

/**
 * @brief Get Device interface from Online mode
 *
 * @return Device name used to capture packets
 * @return NULL in Offline or Mixed mode
 */
const gchar *
capture_input_pcap_device(CaptureManager *manager);

#endif
