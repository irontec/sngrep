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
 * @file capture.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in pcap.h
 *
 * sngrep can parse a pcap file to display call flows.
 * This file include the functions that uses libpcap to do so.
 *
 */

#include "config.h"
#include <netdb.h>
#include "capture.h"
#include "capture_ws.h"
#ifdef WITH_OPENSSL
#include "capture_tls.h"
#endif
#include "sip.h"
#include "rtp.h"
#include "setting.h"
#include "ui_manager.h"
#ifdef WITH_IPV6
#include <netinet/ip6.h>
#endif

// Capture information
capture_info_t capinfo =
    { 0 };

int
capture_online(const char *dev, const char *outfile)
{
    //! Error string
    char errbuf[PCAP_ERRBUF_SIZE];

    // Set capture mode
    capinfo.status = CAPTURE_ONLINE;

    // Try to find capture device information
    if (pcap_lookupnet(dev, &capinfo.net, &capinfo.mask, errbuf) == -1) {
        fprintf(stderr, "Can't get netmask for device %s\n", dev);
        capinfo.net = 0;
        capinfo.mask = 0;
    }

    // Open capture device
    capinfo.handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
    if (capinfo.handle == NULL) {
        fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
        return 2;
    }

    // If requested store packets in a dump file
    if (outfile) {
        if ((capinfo.pd = dump_open(outfile)) == NULL) {
            fprintf(stderr, "Couldn't open output dump file %s: %s\n", outfile,
                    pcap_geterr(capinfo.handle));
            return 2;
        }
    }

    // Get datalink to parse packets correctly
    capinfo.link = pcap_datalink(capinfo.handle);

    // Check linktypes sngrep knowns before start parsing packets
    if (datalink_size(capinfo.link) == -1) {
        fprintf(stderr, "Unable to handle linktype %d\n", capinfo.link);
        return 3;
    }

    // Get Local devices addresses
    pcap_findalldevs(&capinfo.devices, errbuf);

    return 0;
}

int
capture_offline(const char *infile, const char *outfile)
{
    // Error text (in case of file open error)
    char errbuf[PCAP_ERRBUF_SIZE];

    // Set capture mode
    capinfo.status = CAPTURE_OFFLINE_LOADING;
    // Set capture input file
    capinfo.infile = infile;

    // Open PCAP file
    if ((capinfo.handle = pcap_open_offline(infile, errbuf)) == NULL) {
        fprintf(stderr, "Couldn't open pcap file %s: %s\n", infile, errbuf);
        return 1;
    }

    // If requested store packets in a dump file
    if (outfile) {
        if ((capinfo.pd = dump_open(outfile)) == NULL) {
            fprintf(stderr, "Couldn't open output dump file %s: %s\n", outfile,
                    pcap_geterr(capinfo.handle));
            return 2;
        }
    }

    // Get datalink to parse packets correctly
    capinfo.link = pcap_datalink(capinfo.handle);

    // Check linktypes sngrep knowns before start parsing packets
    if (datalink_size(capinfo.link) == -1) {
        fprintf(stderr, "Unable to handle linktype %d\n", capinfo.link);
        return 3;
    }

    return 0;
}

void
parse_packet(u_char *mode, const struct pcap_pkthdr *header, const u_char *packet)
{
    // Datalink Header size
    int size_link;
    // IP version
    uint32_t ip_ver;
    // IP header data
    struct ip *ip4;
#ifdef WITH_IPV6
    // IPv6 header data
    struct ip6_hdr *ip6;
#endif
    // IP protocol
    uint8_t ip_proto;
    // IP segment length
    uint32_t ip_len;
    // IP header size
    uint32_t size_ip;
    // Fragment offset
    uint16_t ip_off = 0;
    // Fragmentation flag
    uint8_t ip_frag = 0;
    // Fragmentation offset
    uint16_t ip_frag_off = 0;
    //! Source Address
    char ip_src[ADDRESSLEN];
    //! Destination Address
    char ip_dst[ADDRESSLEN];
    // UDP header data
    struct udphdr *udp;
    // UDP header size
    uint16_t udp_size;
    // TCP header data
    struct tcphdr *tcp;
    // TCP header size
    uint16_t tcp_size;
    // Packet payload data
    u_char *msg_payload = NULL;
    // Packet payload size
    uint32_t size_payload;
    // Parsed message data
    sip_msg_t *msg;
    // Total packet size
    uint32_t size_packet;
    // SIP message transport
    int transport;
    // Source and Destination Ports
    u_short sport, dport;
    // Media structure for RTP packets
    rtp_stream_t *stream;
    // Captured packet info
    capture_packet_t *pkt;

    // Ignore packets while capture is paused
    if (capture_is_paused())
        return;

    // Check if we have reached capture limit
    if (capinfo.limit && sip_calls_count() >= capinfo.limit)
        return;

    // Get link header size from datalink type
    size_link = datalink_size(capinfo.link);

    // Get IP header
    ip4 = (struct ip*) (packet + size_link);

#ifdef WITH_IPV6
    // Get IPv6 header
    ip6 = (struct ip6_hdr*) (packet + size_link);
#endif

    // Get IP version
    ip_ver = ip4->ip_v;

    switch (ip_ver) {
        case 4:
            size_ip = ip4->ip_hl * 4;
            ip_proto = ip4->ip_p;
            ip_len = ntohs(ip4->ip_len);
            inet_ntop(AF_INET, &ip4->ip_src, ip_src, sizeof(ip_src));
            inet_ntop(AF_INET, &ip4->ip_dst, ip_dst, sizeof(ip_dst));

            ip_off = ntohs(ip4->ip_off);
            ip_frag = ip_off & (IP_MF | IP_OFFMASK);
            ip_frag_off = (ip_frag) ? (ip_off & IP_OFFMASK) * 8 : 0;
            break;
#ifdef WITH_IPV6
        case 6:
            size_ip = sizeof(struct ip6_hdr);
            ip_proto = ip6->ip6_nxt;
            ip_len = ntohs(ip6->ip6_plen);
            inet_ntop(AF_INET6, &ip6->ip6_src, ip_src, INET6_ADDRSTRLEN);
            inet_ntop(AF_INET6, &ip6->ip6_dst, ip_dst, INET6_ADDRSTRLEN);

            if (ip_proto == IPPROTO_FRAGMENT) {
                struct ip6_frag *ip6f = (struct ip6_frag *) (ip6 + ip_len);
                ip_frag_off = ntohs(ip6f->ip6f_offlg & IP6F_OFF_MASK);
            }
            break;
#endif
        default:
            return;
    }

    // Only interested in UDP packets
    if (ip_proto == IPPROTO_UDP) {
        // Set transport UDP
        transport = CAPTURE_PACKET_SIP_UDP;

        // Get UDP header
        udp = (struct udphdr*) (packet + size_link + size_ip);
        udp_size = (ip_frag_off) ? 0 : sizeof(struct udphdr);

        // Set packet ports
        sport = htons(udp->uh_sport);
        dport = htons(udp->uh_dport);

        size_payload = htons(udp->uh_ulen) - udp_size;
        if (size_payload != 0) {
            // Get packet payload
            msg_payload = malloc(size_payload + 1);
            memset(msg_payload, 0, size_payload + 1);
            memcpy(msg_payload, (u_char *) (packet + size_link + size_ip + udp_size), size_payload);
        }

        // Total packet size
        size_packet = size_link + size_ip + udp_size + size_payload;

    } else if (ip_proto == IPPROTO_TCP) {
        // Set transport TCP
        transport = CAPTURE_PACKET_SIP_TCP;

        tcp = (struct tcphdr*) (packet + size_link + size_ip);
        tcp_size = (ip_frag_off) ? 0 : (tcp->th_off * 4);

        // Set packet ports
        sport = htons(tcp->th_sport);
        dport = htons(tcp->th_dport);

        // We're only interested in packets with payload
        size_payload = ip_len - (size_ip + tcp_size);
        if (size_payload != 0) {
            // Get packet payload
            msg_payload = malloc(size_payload + 1);
            memset(msg_payload, 0, size_payload + 1);
            memcpy(msg_payload, (u_char *) (packet + size_link + size_ip + tcp_size), size_payload);
        }

        // Total packet size
        size_packet = size_link + size_ip + tcp_size + size_payload;
#ifdef WITH_OPENSSL
        if (!msg_payload || !strstr((const char*) msg_payload, "SIP/2.0")) {
            if (capture_get_keyfile()) {
                // Allocate memory for the payload
                msg_payload = malloc(size_payload + 1);
                memset(msg_payload, 0, size_payload + 1);

                // Try to decrypt the packet
                tls_process_segment(ip4, &msg_payload, &size_payload);

                // Check if we have decoded payload
                if (size_payload == 0) {
                    free(msg_payload);
                    return;
                }

                // Set Transport TLS
                transport = CAPTURE_PACKET_SIP_TLS;
            }
        }
#endif
        // Check if packet is Websocket
        if (msg_payload && capture_ws_check_packet(msg_payload, &size_payload)) {
            transport = CAPTURE_PACKET_SIP_WS;
        }
    } else {
        // Not handled protocol
        return;
    }

    // We're only interested in packets with payload
    if (size_payload == 0)
        return;

    // Create a structure for this captured packet
    pkt = capture_packet_create(header, packet, size_packet, size_payload);
    capture_packet_set_type(pkt, transport);

    // Parse this header and payload
    if ((msg = sip_load_message(pkt, ip_src, sport, ip_dst, dport, msg_payload))) {
        // Store this packets in output file
        dump_packet(capinfo.pd, header, packet);
        // Deallocate packet duplicated payload
        free(msg_payload);
        return;
    }

    // Check if this packet belongs to a RTP stream
    // TODO Store this packet in the stream
    if ((stream = rtp_check_stream(header, ip_src, sport, ip_dst, dport, msg_payload, size_payload))) {
        // We have an RTP packet!
        capture_packet_set_type(pkt, CAPTURE_PACKET_RTP);
        // Store this pacekt if capture rtp is enabled
        if (capinfo.rtp_capture) {
            call_add_rtp_packet(stream_get_call(stream), pkt);
        } else {
            capture_packet_destroy(pkt);
        }
        // Store this packets in output file
        dump_packet(capinfo.pd, header, packet);
        // Deallocate packet duplicated payload
        free(msg_payload);
        return;
    }

    // Deallocate packet duplicated payload
    free(msg_payload);

    // Not an interesting packet ...
    capture_packet_destroy(pkt);
}

void
capture_close()
{
    //Close PCAP file
    if (capinfo.handle) {
        pcap_breakloop(capinfo.handle);
        pthread_join(capinfo.capture_t, NULL);
        pcap_close(capinfo.handle);
    }

    // Close dump file
    if (capinfo.pd) {
        dump_close(capinfo.pd);
    }
}

int
capture_launch_thread()
{
    //! capture thread attributes
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (pthread_create(&capinfo.capture_t, &attr, (void *) capture_thread, NULL)) {
        return 1;
    }
    pthread_attr_destroy(&attr);
    return 0;
}

void
capture_thread(void *none)
{
    // Parse available packets
    pcap_loop(capinfo.handle, -1, parse_packet, NULL);
    // In offline mode, set capture to fully loaded
    if (!capture_is_online())
        capinfo.status = CAPTURE_OFFLINE;
}

int
capture_is_online()
{
    return (capinfo.status == CAPTURE_ONLINE || capinfo.status == CAPTURE_ONLINE_PAUSED);
}

int
capture_set_bpf_filter(const char *filter)
{
    //! Check if filter compiles
    if (pcap_compile(capinfo.handle, &capinfo.fp, filter, 0, capinfo.mask) == -1)
        return 1;

    // Set capture filter
    if (pcap_setfilter(capinfo.handle, &capinfo.fp) == -1)
        return 1;

    return 0;
}

void
capture_set_opts(int limit, int rtp_capture)
{
    capinfo.limit = limit;
    capinfo.rtp_capture = rtp_capture;
}

void
capture_set_paused(int pause)
{
    if (capture_is_online()) {
        capinfo.status = (pause) ? CAPTURE_ONLINE_PAUSED : CAPTURE_ONLINE;
    }
}

int
capture_is_paused()
{
    return capinfo.status == CAPTURE_ONLINE_PAUSED;
}

int
capture_get_status()
{
    return capinfo.status;
}

const char *
capture_get_status_desc()
{
    switch (capinfo.status) {
        case CAPTURE_ONLINE:
            return "Online";
        case CAPTURE_ONLINE_PAUSED:
            return "Online (Paused)";
        case CAPTURE_OFFLINE:
            return "Offline";
        case CAPTURE_OFFLINE_LOADING:
            return "Offline (Loading)";
    }
    return "";
}

const char*
capture_get_infile()
{
    return capinfo.infile;
}

const char*
capture_get_keyfile()
{
    return capinfo.keyfile;
}

void
capture_set_keyfile(const char *keyfile)
{
    capinfo.keyfile = keyfile;
}

char *
capture_last_error()
{
    return pcap_geterr(capinfo.handle);
}

capture_packet_t *
capture_packet_create(const struct pcap_pkthdr *header, const u_char *packet, int size, int payload_len)
{
    capture_packet_t *pkt;
    pkt = malloc(sizeof(capture_packet_t));
    pkt->size = size;
    pkt->header = malloc(sizeof(struct pcap_pkthdr));
    memcpy(pkt->header, header, sizeof(struct pcap_pkthdr));
    pkt->data = malloc(size);
    memcpy(pkt->data, packet, size);
    pkt->payload_len = payload_len;
    pkt->payload_start = size - pkt->payload_len;
    return pkt;
}

void
capture_packet_destroy(capture_packet_t *packet)
{
    free(packet->header);
    free(packet->data);
    free(packet);
}


void
capture_packet_destroyer(void *packet)
{
    capture_packet_destroy((capture_packet_t*) packet);
}

void
capture_packet_set_type(capture_packet_t *packet, int type)
{
    packet->type = type;
}

void
capture_packet_time_sorter(vector_t *vector, void *item)
{
    capture_packet_t *prev, *cur;
    int count = vector_count(vector);
    int i;

    // Get current item
    cur = (capture_packet_t *) item;
    prev = vector_item(vector, count - 2);

    // Check if the item is already sorted
    if (prev && timeval_is_older(cur->header->ts, prev->header->ts)) {
        return;
    }

    for (i = count - 2 ; i >= 0; i--) {
        // Get previous packet
        prev = vector_item(vector, i);
        // Check if the item is already in a sorted position
        if (timeval_is_older(cur->header->ts, prev->header->ts)) {
            vector_insert(vector, item, i + 1);
            return;
        }
    }

    // Put this item at the begining of the vector
    vector_insert(vector, item, 0);
}


int
datalink_size(int datalink)
{
    // Datalink header size
    switch (datalink) {
        case DLT_EN10MB:
            return 14;
        case DLT_IEEE802:
            return 22;
        case DLT_LOOP:
        case DLT_NULL:
            return 4;
        case DLT_SLIP:
        case DLT_SLIP_BSDOS:
            return 16;
        case DLT_PPP:
        case DLT_PPP_BSDOS:
        case DLT_PPP_SERIAL:
        case DLT_PPP_ETHER:
            return 4;
        case DLT_RAW:
            return 0;
        case DLT_FDDI:
            return 21;
        case DLT_ENC:
            return 12;
#ifdef DLT_LINUX_SLL
        case DLT_LINUX_SLL:
            return 16;
#endif
#ifdef DLT_IPNET
        case DLT_IPNET:
            return 24;
#endif
        default:
            // Not handled datalink type
            return -1;
    }

}

pcap_dumper_t *
dump_open(const char *dumpfile)
{
    return pcap_dump_open(capinfo.handle, dumpfile);
}

void
dump_packet(pcap_dumper_t *pd, const struct pcap_pkthdr *header, const u_char *packet)
{
    if (!pd)
        return;
    pcap_dump((u_char*) pd, header, packet);
    pcap_dump_flush(pd);
}

void
dump_close(pcap_dumper_t *pd)
{
    if (!pd)
        return;
    pcap_dump_close(pd);
}

const char *
lookup_hostname(const char *address)
{
    int i;
    int hostlen;
    in_addr_t netaddress;
    struct hostent *host;
    const char *hostname;

    // No lookup enabled, return address as is
    if (!setting_enabled(SETTING_CAPTURE_LOOKUP))
        return address;

    // Check if we have already tryied resolve this address
    for (i = 0; i < capinfo.dnscache.count; i++) {
        if (!strcmp(capinfo.dnscache.addr[i], address)) {
            return capinfo.dnscache.hostname[i];
        }
    }

    // Convert the address to network byte order
    if ((netaddress = inet_addr(address)) == -1)
        return address;

    // Lookup this addres
    host = gethostbyaddr(&netaddress, sizeof(netaddress), AF_INET);
    if (!host) {
        hostname = address;
    } else {
        hostname = host->h_name;
    }

    // Max hostname length set to 16 chars
    hostlen = strlen(hostname);

    // Store this result in the dnscache
    strcpy(capinfo.dnscache.addr[capinfo.dnscache.count], address);
    strncpy(capinfo.dnscache.hostname[capinfo.dnscache.count], hostname, hostlen);
    capinfo.dnscache.count++;

    // Return the stored value
    return capinfo.dnscache.hostname[capinfo.dnscache.count - 1];
}

int
is_local_address_str(const char *address)
{
    return is_local_address(inet_addr(address));
}

int
is_local_address(in_addr_t address)
{
    pcap_if_t *device;
    pcap_addr_t *dev_addr;

    for (device = capinfo.devices; device; device = device->next) {
        for (dev_addr = device->addresses; dev_addr; dev_addr = dev_addr->next)
            if (dev_addr->addr && dev_addr->addr->sa_family == AF_INET
                    && ((struct sockaddr_in*) dev_addr->addr)->sin_addr.s_addr == address)
                return 1;
    }
    return 0;
}
