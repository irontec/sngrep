/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2016 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2016 Irontec SL. All rights reserved.
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
#include <string.h>
#include <stdbool.h>
#include "capture.h"
#ifdef USE_EEP
#include "capture_eep.h"
#endif
#ifdef WITH_GNUTLS
#include "capture_gnutls.h"
#endif
#ifdef WITH_OPENSSL
#include "capture_openssl.h"
#endif
#include "sip.h"
#include "rtp.h"
#include "setting.h"
#include "util.h"

// Capture information
capture_config_t capture_cfg =
{ 0 };

void
capture_init(size_t limit, bool rtp_capture, bool rotate)
{
    capture_cfg.limit = limit;
    capture_cfg.rtp_capture = rtp_capture;
    capture_cfg.rotate = rotate;
    capture_cfg.sources = vector_create(1, 1);
    capture_cfg.tcp_reasm = vector_create(0, 10);
    capture_cfg.ip_reasm = vector_create(0, 10);

    // Fixme
    if (setting_has_value(SETTING_CAPTURE_STORAGE, "none")) {
        capture_cfg.storage = CAPTURE_STORAGE_NONE;
    } else if (setting_has_value(SETTING_CAPTURE_STORAGE, "memory")) {
        capture_cfg.storage = CAPTURE_STORAGE_MEMORY;
    } else if (setting_has_value(SETTING_CAPTURE_STORAGE, "disk")) {
        capture_cfg.storage = CAPTURE_STORAGE_DISK;
    }

    // Initialize calls lock
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
#if defined(PTHREAD_MUTEX_RECURSIVE) || defined(__FreeBSD__) || defined(BSD) || defined (__OpenBSD__) || defined(__DragonFly__)
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#else
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
#endif
    pthread_mutex_init(&capture_cfg.lock, &attr);

}

void
capture_deinit()
{
    // Close pcap handler
    capture_close();

    // Deallocate vectors
    vector_set_destroyer(capture_cfg.sources, vector_generic_destroyer);
    vector_destroy(capture_cfg.sources);
    vector_set_destroyer(capture_cfg.tcp_reasm, packet_destroyer);
    vector_destroy(capture_cfg.tcp_reasm);
    vector_set_destroyer(capture_cfg.ip_reasm, packet_destroyer);
    vector_destroy(capture_cfg.ip_reasm);

    // Remove capture mutex
    pthread_mutex_destroy(&capture_cfg.lock);
}

int
capture_online(const char *dev, const char *outfile)
{
    capture_info_t *capinfo;

    //! Error string
    char errbuf[PCAP_ERRBUF_SIZE];

    // Set capture mode
    capture_cfg.status = CAPTURE_ONLINE;

    // Create a new structure to handle this capture source
    if (!(capinfo = sng_malloc(sizeof(capture_info_t)))) {
        fprintf(stderr, "Can't allocate memory for capture data!\n");
        return 1;
    }

    // Try to find capture device information
    if (pcap_lookupnet(dev, &capinfo->net, &capinfo->mask, errbuf) == -1) {
        fprintf(stderr, "Can't get netmask for device %s\n", dev);
        capinfo->net = 0;
        capinfo->mask = 0;
        return 2;
    }

    // Open capture device
    capinfo->handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
    if (capinfo->handle == NULL) {
        fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
        return 2;
    }

    // Get datalink to parse packets correctly
    capinfo->link = pcap_datalink(capinfo->handle);

    // Check linktypes sngrep knowns before start parsing packets
    if ((capinfo->link_hl = datalink_size(capinfo->link)) == -1) {
        fprintf(stderr, "Unable to handle linktype %d\n", capinfo->link);
        return 3;
    }

    // Add this capture information as packet source
    vector_append(capture_cfg.sources, capinfo);

    // If requested store packets in a dump file
    if (outfile && !capture_cfg.pd) {
        if ((capture_cfg.pd = dump_open(outfile)) == NULL) {
            fprintf(stderr, "Couldn't open output dump file %s: %s\n", outfile,
                    pcap_geterr(capinfo->handle));
            return 2;
        }
    }

    return 0;
}

int
capture_offline(const char *infile, const char *outfile)
{
    capture_info_t *capinfo;

    // Error text (in case of file open error)
    char errbuf[PCAP_ERRBUF_SIZE];

    // Set capture mode
    capture_cfg.status = CAPTURE_OFFLINE_LOADING;

    // Create a new structure to handle this capture source
    if (!(capinfo = sng_malloc(sizeof(capture_info_t)))) {
        fprintf(stderr, "Can't allocate memory for capture data!\n");
        return 1;
    }

    // Set capture input file
    capinfo->infile = infile;

    // Open PCAP file
    if ((capinfo->handle = pcap_open_offline(infile, errbuf)) == NULL) {
        fprintf(stderr, "Couldn't open pcap file %s: %s\n", infile, errbuf);
        return 1;
    }

    // Get datalink to parse packets correctly
    capinfo->link = pcap_datalink(capinfo->handle);

    // Check linktypes sngrep knowns before start parsing packets
    if ((capinfo->link_hl = datalink_size(capinfo->link)) == -1) {
        fprintf(stderr, "Unable to handle linktype %d\n", capinfo->link);
        return 3;
    }

    // Add this capture information as packet source
    vector_append(capture_cfg.sources, capinfo);

    // If requested store packets in a dump file
    if (outfile && !capture_cfg.pd) {
        if ((capture_cfg.pd = dump_open(outfile)) == NULL) {
            fprintf(stderr, "Couldn't open output dump file %s: %s\n", outfile,
                    pcap_geterr(capinfo->handle));
            return 2;
        }
    }

    return 0;
}

void
parse_packet(u_char *info, const struct pcap_pkthdr *header, const u_char *packet)
{
    // Capture info
    capture_info_t *capinfo = (capture_info_t *) info;
    // UDP header data
    struct udphdr *udp;
    // UDP header size
    uint16_t udp_off;
    // TCP header data
    struct tcphdr *tcp;
    // TCP header size
    uint16_t tcp_off;
    // Packet data
    u_char data[MAX_CAPTURE_LEN];
    // Packet payload data
    u_char *payload = NULL;
    // Whole packet size
    uint32_t size_capture = header->caplen;
    // Packet payload size
    uint32_t size_payload =  size_capture - capinfo->link_hl;
    // Captured packet info
    packet_t *pkt;

    // Ignore packets while capture is paused
    if (capture_paused())
        return;

    // Check if we have reached capture limit
    if (capture_cfg.limit && sip_calls_count() >= capture_cfg.limit) {
        // If capture rotation is disabled, just skip this packet
        if (!capture_cfg.rotate) {
            return;
        }
    }

    // Check maximum capture length
    if (header->caplen > MAX_CAPTURE_LEN)
        return;

    // Copy packet payload
    memcpy(data, packet, header->caplen);

    // Check if we have a complete IP packet
    if (!(pkt = capture_packet_reasm_ip(capinfo, header, data, &size_payload, &size_capture)))
        return;

    // Only interested in UDP packets
    if (pkt->proto == IPPROTO_UDP) {
        // Get UDP header
        udp = (struct udphdr *)((u_char *)(data) + (size_capture - size_payload));
        udp_off = sizeof(struct udphdr);

        // Set packet ports
        pkt->src.port = htons(udp->uh_sport);
        pkt->dst.port = htons(udp->uh_dport);

        // Remove UDP Header from payload
        size_payload -= udp_off;

        if ((int32_t)size_payload < 0)
            size_payload = 0;

        // Remove TCP Header from payload
        payload = (u_char *) (udp) + udp_off;

        // Complete packet with Transport information
        packet_set_type(pkt, PACKET_SIP_UDP);
        packet_set_payload(pkt, payload, size_payload);

    } else if (pkt->proto == IPPROTO_TCP) {
        // Get TCP header
        tcp = (struct tcphdr *)((u_char *)(data) + (size_capture - size_payload));
        tcp_off = (tcp->th_off * 4);

        // Set packet ports
        pkt->src.port = htons(tcp->th_sport);
        pkt->dst.port = htons(tcp->th_dport);

        // Get actual payload size
        size_payload -= tcp_off;

        if ((int32_t)size_payload < 0)
            size_payload = 0;

        // Get payload start
        payload = (u_char *)(tcp) + tcp_off;

        // Complete packet with Transport information
        packet_set_type(pkt, PACKET_SIP_TCP);
        packet_set_payload(pkt, payload, size_payload);

        // Create a structure for this captured packet
        if (!(pkt = capture_packet_reasm_tcp(pkt, tcp, payload, size_payload)))
            return;

#if defined(WITH_GNUTLS) || defined(WITH_OPENSSL)
        // Check if packet is TLS
        if (capture_cfg.keyfile) {
            tls_process_segment(pkt, tcp);
        }
#endif

        // Check if packet is WS or WSS
        capture_ws_check_packet(pkt);
    } else {
        // Not handled protocol
        packet_destroy(pkt);
        return;
    }

    // Avoid parsing from multiples sources.
    // Avoid parsing while screen in being redrawn
    capture_lock();
    // Check if we can handle this packet
    if (capture_packet_parse(pkt) == 0) {
#ifdef USE_EEP
        // Send this packet through eep
        capture_eep_send(pkt);
#endif
        // Store this packets in output file
        dump_packet(capture_cfg.pd, pkt);
        // If storage is disabled, delete frames payload
        if (capture_cfg.storage == 0) {
            packet_free_frames(pkt);
        }
        // Allow Interface refresh and user input actions
        capture_unlock();
        return;
    }

    // Not an interesting packet ...
    packet_destroy(pkt);
    // Allow Interface refresh and user input actions
    capture_unlock();
}

packet_t *
capture_packet_reasm_ip(capture_info_t *capinfo, const struct pcap_pkthdr *header, u_char *packet, uint32_t *size, uint32_t *caplen)
{
    // IP header data
    struct ip *ip4;
#ifdef USE_IPV6
    // IPv6 header data
    struct ip6_hdr *ip6;
#endif
    // IP version
    uint32_t ip_ver;
    // IP protocol
    uint8_t ip_proto;
    // IP header size
    uint32_t ip_hl = 0;
    // Fragment offset
    uint16_t ip_off = 0;
    // IP content len
    uint16_t ip_len = 0;
    // Fragmentation flag
    uint16_t ip_frag = 0;
    // Fragmentation identifier
    uint32_t ip_id = 0;
    // Fragmentation offset
    uint16_t ip_frag_off = 0;
    //! Source Address
    address_t src = { };
    //! Destination Address
    address_t dst = { };
    //! Common interator for vectors
    vector_iter_t it;
    //! Packet containers
    packet_t *pkt;
    //! Storage for IP frame
    frame_t *frame;
    uint32_t len_data = 0;
    //! Link + Extra header size
    int8_t link_hl = capinfo->link_hl;

    // Skip VLAN header if present
    if (capinfo->link == DLT_EN10MB) {
        struct ether_header *eth = (struct ether_header *) packet;
        if (ntohs(eth->ether_type) == ETHERTYPE_8021Q) {
            link_hl += 4;
        }
    }

    // Get IP header
    ip4 = (struct ip *) (packet + link_hl);

#ifdef USE_IPV6
    // Get IPv6 header
    ip6 = (struct ip6_hdr *) (packet + link_hl);
#endif

    // Get IP version
    ip_ver = ip4->ip_v;

    switch (ip_ver) {
        case 4:
            ip_hl = ip4->ip_hl * 4;
            ip_proto = ip4->ip_p;
            ip_off = ntohs(ip4->ip_off);
            ip_len = ntohs(ip4->ip_len);

            ip_frag = ip_off & (IP_MF | IP_OFFMASK);
            ip_frag_off = (ip_frag) ? (ip_off & IP_OFFMASK) * 8 : 0;
            ip_id = ntohs(ip4->ip_id);

            inet_ntop(AF_INET, &ip4->ip_src, src.ip, sizeof(src.ip));
            inet_ntop(AF_INET, &ip4->ip_dst, dst.ip, sizeof(dst.ip));
            break;
#ifdef USE_IPV6
        case 6:
            ip_hl = sizeof(struct ip6_hdr);
            ip_proto = ip6->ip6_nxt;
            ip_len = ntohs(ip6->ip6_ctlun.ip6_un1.ip6_un1_plen) + ip_hl;

            if (ip_proto == IPPROTO_FRAGMENT) {
                struct ip6_frag *ip6f = (struct ip6_frag *) (ip6 + ip_hl);
                ip_frag_off = ntohs(ip6f->ip6f_offlg & IP6F_OFF_MASK);
                ip_id = ntohl(ip6f->ip6f_ident);
            }

            inet_ntop(AF_INET6, &ip6->ip6_src, src.ip, sizeof(src.ip));
            inet_ntop(AF_INET6, &ip6->ip6_dst, dst.ip, sizeof(dst.ip));
            break;
#endif
        default:
            return NULL;
    }

    // Fixup VSS trailer in ethernet packets
    *caplen = link_hl + ip_len;

    // Remove IP Header length from payload
    *size = *caplen - link_hl - ip_hl;

    // If no fragmentation
    if (ip_frag == 0) {
        // Just create a new packet with given network data
        pkt = packet_create(ip_ver, ip_proto, src, dst, ip_id);
        packet_add_frame(pkt, header, packet);
        return pkt;
    }

    // Look for another packet with same id in IP reassembly vector
    it = vector_iterator(capture_cfg.ip_reasm);
    while ((pkt = vector_iterator_next(&it))) {
        if (addressport_equals(pkt->src, src)
                && addressport_equals(pkt->dst, dst)
                && pkt->ip_id == ip_id) {
            break;
        }
    }

    // If we already have this packet stored, append this frames to existing one
    if (pkt) {
        packet_add_frame(pkt, header, packet);
    } else {
        // Add To the possible reassembly list
        pkt = packet_create(ip_ver, ip_proto, src, dst, ip_id);
        packet_add_frame(pkt, header, packet);
        vector_append(capture_cfg.ip_reasm, pkt);
        return NULL;
    }

    // If no more fragments
    if ((ip_off & IP_MF) == 0) {
        // TODO Dont check the flag, check the holes
        // Calculate assembled IP payload data
        it = vector_iterator(pkt->frames);
        while ((frame = vector_iterator_next(&it))) {
            struct ip *frame_ip = (struct ip *) (frame->data + link_hl);
            len_data += frame->header->caplen - link_hl - frame_ip->ip_hl * 4;
        }

        // Check packet content length
        if (len_data > MAX_CAPTURE_LEN)
            return NULL;

        // Initialize memory for the assembly packet
        memset(packet, 0, link_hl + ip_hl + len_data);

        it = vector_iterator(pkt->frames);
        while ((frame = vector_iterator_next(&it))) {
            // Get IP header
            struct ip *frame_ip = (struct ip *) (frame->data + link_hl);
            memcpy(packet + link_hl + ip_hl + (ntohs(frame_ip->ip_off) & IP_OFFMASK) * 8,
                   frame->data + link_hl + frame_ip->ip_hl * 4,
                   frame->header->caplen - link_hl - frame_ip->ip_hl * 4);
        }

        *caplen = link_hl + ip_hl + len_data;
        *size = len_data;

        // Return the assembled IP packet
        vector_remove(capture_cfg.ip_reasm, pkt);
        return pkt;
    }

    return NULL;
}

packet_t *
capture_packet_reasm_tcp(packet_t *packet, struct tcphdr *tcp, u_char *payload, int size_payload) {

    vector_iter_t it = vector_iterator(capture_cfg.tcp_reasm);
    packet_t *pkt;
    u_char *new_payload;

    //! Assembled
    if ((int32_t) size_payload <= 0)
        return packet;

    while ((pkt = vector_iterator_next(&it))) {
        if (addressport_equals(pkt->src, packet->src) &&
                addressport_equals(pkt->dst, packet->dst)) {
            break;
        }
    }

    // If we already have this packet stored
    if (pkt) {
        frame_t *frame;
        // Append this frames to the original packet
        vector_iter_t frames = vector_iterator(packet->frames);
        while ((frame = vector_iterator_next(&frames)))
            packet_add_frame(pkt, frame->header, frame->data);
        // Destroy current packet as its frames belong to the stored packet
        packet_destroy(packet);
    } else {
        // First time this packet has been seen
        pkt = packet;
        // Add To the possible reassembly list
        vector_append(capture_cfg.tcp_reasm, packet);
    }

    // Store firt tcp sequence
    if (pkt->tcp_seq == 0) {
        pkt->tcp_seq = ntohl(tcp->th_seq);
    }

    // If the first frame of this packet
    if (vector_count(pkt->frames) == 1) {
        // Set initial payload
        packet_set_payload(pkt, payload, size_payload);
    } else {
        // Check payload length. Dont handle too big payload packets
        if (pkt->payload_len + size_payload > MAX_CAPTURE_LEN) {
            packet_destroy(pkt);
            vector_remove(capture_cfg.tcp_reasm, pkt);
            return NULL;
        }
        new_payload = sng_malloc(pkt->payload_len + size_payload);
        if (pkt->tcp_seq < ntohl(tcp->th_seq)) {
            // Append payload to the existing
            pkt->tcp_seq =  ntohl(tcp->th_seq);
            memcpy(new_payload, pkt->payload, pkt->payload_len);
            memcpy(new_payload + pkt->payload_len, payload, size_payload);
        } else {
            // Prepend payload to the existing
            memcpy(new_payload, payload, size_payload);
            memcpy(new_payload + size_payload, pkt->payload, pkt->payload_len);
        }
        packet_set_payload(pkt, new_payload, pkt->payload_len + size_payload);
        sng_free(new_payload);

    }

    // This packet is ready to be parsed
    int valid = sip_validate_packet(pkt);
    if (valid == VALIDATE_COMPLETE_SIP) {
        // Full SIP packet!
        vector_remove(capture_cfg.tcp_reasm, pkt);
        return pkt;
    } else if (valid == VALIDATE_NOT_SIP) {
        // Not a SIP packet, store until PSH flag
        if (tcp->th_flags & TH_PUSH) {
            vector_remove(capture_cfg.tcp_reasm, pkt);
            return pkt;
        }
    }

    // An incomplete SIP Packet
    return NULL;
}

int
capture_ws_check_packet(packet_t *packet)
{
    int ws_off = 0;
    u_char ws_fin;
    u_char ws_opcode;
    u_char ws_mask;
    uint8_t ws_len;
    u_char ws_mask_key[4];
    u_char *payload, *newpayload;
    uint32_t size_payload;
    int i;

    /**
     * WSocket header definition according to RFC 6455
     *     0                   1                   2                   3
     *     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     *    +-+-+-+-+-------+-+-------------+-------------------------------+
     *    |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
     *    |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
     *    |N|V|V|V|       |S|             |   (if payload len==126/127)   |
     *    | |1|2|3|       |K|             |                               |
     *    +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
     *    |     Extended payload length continued, if payload len == 127  |
     *    + - - - - - - - - - - - - - - - +-------------------------------+
     *    |                               |Masking-key, if MASK set to 1  |
     *    +-------------------------------+-------------------------------+
     *    | Masking-key (continued)       |          Payload Data         |
     *    +-------------------------------- - - - - - - - - - - - - - - - +
     *    :                     Payload Data continued ...                :
     *    + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
     *    |                     Payload Data continued ...                |
     *    +---------------------------------------------------------------+
     */

    // Get payload from packet(s)
    size_payload = packet_payloadlen(packet);
    payload = packet_payload(packet);

    // Check we have payload
    if (size_payload == 0)
        return 0;

    // Flags && Opcode
    ws_fin = (*payload & WH_FIN) >> 4;
    ws_opcode = *payload & WH_OPCODE;
    ws_off++;

    // Only interested in Ws text packets
    if (ws_opcode != WS_OPCODE_TEXT)
        return 0;

    // Masked flag && Payload len
    ws_mask = (*(payload + ws_off) & WH_MASK) >> 4;
    ws_len = (*(payload + ws_off) & WH_LEN);
    ws_off++;

    // Skip Payload len
    switch (ws_len) {
            // Extended
        case 126:
            ws_off += 2;
            break;
        case 127:
            ws_off += 8;
            break;
        default:
            return 0;
    }

    // Get Masking key if mask is enabled
    if (ws_mask) {
        memcpy(ws_mask_key, (payload + ws_off), 4);
        ws_off += 4;
    }

    // Skip Websocket headers
    size_payload -= ws_off;
    if ((int32_t) size_payload <= 0)
        return 0;

    newpayload = sng_malloc(size_payload);
    memcpy(newpayload, payload + ws_off, size_payload);
    // If mask is enabled, unmask the payload
    if (ws_mask) {
        for (i = 0; i < size_payload; i++)
            newpayload[i] = newpayload[i] ^ ws_mask_key[i % 4];
    }
    // Set new packet payload into the packet
    packet_set_payload(packet, newpayload, size_payload);
    // Free the new payload
    sng_free(newpayload);

    if (packet->type == PACKET_SIP_TLS) {
        packet_set_type(packet, PACKET_SIP_WSS);
    } else {
        packet_set_type(packet, PACKET_SIP_WS);
    }
    return 1;
}


int
capture_packet_parse(packet_t *packet)
{
    // Media structure for RTP packets
    rtp_stream_t *stream;

    // We're only interested in packets with payload
    if (packet_payloadlen(packet)) {
        // Parse this header and payload
        if (sip_check_packet(packet)) {
            return 0;
        }

        // Check if this packet belongs to a RTP stream
        if ((stream = rtp_check_packet(packet))) {
            // We have an RTP packet!
            packet_set_type(packet, PACKET_RTP);
            // Store this pacekt if capture rtp is enabled
            if (capture_cfg.rtp_capture) {
                call_add_rtp_packet(stream_get_call(stream), packet);
                return 0;
            }
        }
    }
    return 1;
}

void
capture_close()
{
    capture_info_t *capinfo;

    // Nothing to close
    if (vector_count(capture_cfg.sources) == 0)
        return;

    // Stop all captures
    vector_iter_t it = vector_iterator(capture_cfg.sources);
    while ((capinfo = vector_iterator_next(&it))) {
        //Close PCAP file
        if (capinfo->handle) {
            pcap_breakloop(capinfo->handle);
            pthread_join(capinfo->capture_t, NULL);
            pcap_close(capinfo->handle);
        }
    }

    // Close dump file
    if (capture_cfg.pd) {
        dump_close(capture_cfg.pd);
    }
}

int
capture_launch_thread(capture_info_t *capinfo)
{
    //! capture thread attributes
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    // Start all captures threads
    vector_iter_t it = vector_iterator(capture_cfg.sources);
    while ((capinfo = vector_iterator_next(&it))) {
        if (pthread_create(&capinfo->capture_t, &attr, (void *) capture_thread, capinfo)) {
            return 1;
        }
    }

    pthread_attr_destroy(&attr);
    return 0;
}

void
capture_thread(void *info)
{
    capture_info_t *capinfo = (capture_info_t *) info;

    // Parse available packets
    pcap_loop(capinfo->handle, -1, parse_packet, (u_char *) capinfo);

    if (!capture_is_online())
        capture_cfg.status = CAPTURE_OFFLINE;
}

int
capture_is_online()
{
    return (capture_cfg.status == CAPTURE_ONLINE || capture_cfg.status == CAPTURE_ONLINE_PAUSED);
}

int
capture_set_bpf_filter(const char *filter)
{
    vector_iter_t it = vector_iterator(capture_cfg.sources);
    capture_info_t *capinfo;

    // Apply the given filter to all sources
    while ((capinfo = vector_iterator_next(&it))) {
        //! Check if filter compiles
        if (pcap_compile(capinfo->handle, &capture_cfg.fp, filter, 0, capinfo->mask) == -1)
            return 1;

        // Set capture filter
        if (pcap_setfilter(capinfo->handle, &capture_cfg.fp) == -1)
            return 1;

    }

    return 0;
}

void
capture_set_paused(int pause)
{
    if (capture_is_online()) {
        capture_cfg.status = (pause) ? CAPTURE_ONLINE_PAUSED : CAPTURE_ONLINE;
    }
}

bool
capture_paused()
{
    return capture_cfg.status == CAPTURE_ONLINE_PAUSED;
}

enum capture_status
capture_status()
{
    return capture_cfg.status;
}

const char *
capture_status_desc()
{
    switch (capture_cfg.status) {
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
capture_input_file()
{
    capture_info_t *capinfo;

    if (vector_count(capture_cfg.sources) == 1) {
        capinfo = vector_first(capture_cfg.sources);
        if (capinfo->infile) {
            return sng_basename(capinfo->infile);
        } else {
            return NULL;
        }
    } else {
        return "Multiple files";
    }

}

const char*
capture_keyfile()
{
    return capture_cfg.keyfile;
}

void
capture_set_keyfile(const char *keyfile)
{
    capture_cfg.keyfile = keyfile;
}

char *
capture_last_error()
{
    capture_info_t *capinfo;
    if (vector_count(capture_cfg.sources) == 1) {
        capinfo = vector_first(capture_cfg.sources);
        return pcap_geterr(capinfo->handle);
    }
    return NULL;

}

void
capture_lock()
{
    // Avoid parsing more packet
    pthread_mutex_lock(&capture_cfg.lock);
}

void
capture_unlock()
{
    // Allow parsing more packets
    pthread_mutex_unlock(&capture_cfg.lock);
}



void
capture_packet_time_sorter(vector_t *vector, void *item)
{
    struct timeval curts, prevts;
    int count = vector_count(vector);
    int i;

    // TODO Implement multiframe packets
    curts = packet_time(item);
    prevts = packet_time(vector_last(vector));

    // Check if the item is already sorted
    if (timeval_is_older(curts, prevts)) {
        return;
    }

    for (i = count - 2 ; i >= 0; i--) {
        // Get previous packet
        prevts = packet_time(vector_item(vector, i));
        // Check if the item is already in a sorted position
        if (timeval_is_older(curts, prevts)) {
            vector_insert(vector, item, i + 1);
            return;
        }
    }

    // Put this item at the begining of the vector
    vector_insert(vector, item, 0);
}


int8_t
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
    capture_info_t *capinfo;

    if (vector_count(capture_cfg.sources) == 1) {
        capinfo = vector_first(capture_cfg.sources);
        return pcap_dump_open(capinfo->handle, dumpfile);
    }
    return NULL;
}

void
dump_packet(pcap_dumper_t *pd, const packet_t *packet)
{
    if (!pd || !packet)
        return;

    vector_iter_t it = vector_iterator(packet->frames);
    frame_t *frame;
    while ((frame = vector_iterator_next(&it))) {
        pcap_dump((u_char*) pd, frame->header, frame->data);
    }
    pcap_dump_flush(pd);
}

void
dump_close(pcap_dumper_t *pd)
{
    if (!pd)
        return;
    pcap_dump_close(pd);
}
