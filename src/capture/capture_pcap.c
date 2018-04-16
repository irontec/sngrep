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
#include <glib.h>
#include <netdb.h>
#include <string.h>
#include <stdbool.h>
#include "glib-utils.h"
#include "capture.h"
#include "capture_hep.h"
#include "capture_pcap.h"
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

GQuark
capture_pcap_error_quark()
{
    return  g_quark_from_static_string("capture-pcap");
}

CaptureInput *
capture_input_pcap_online(const gchar *dev, GError **error)
{
    char errbuf[PCAP_ERRBUF_SIZE];

    // Create a new structure to handle this capture source
    CapturePcap *pcap = g_malloc(sizeof(CapturePcap));

    // Try to find capture device information
    if (pcap_lookupnet(dev, &pcap->net, &pcap->mask, errbuf) == -1) {
        g_set_error (error,
                     CAPTURE_PCAP_ERROR,
                     CAPTURE_PCAP_ERROR_DEVICE_LOOKUP,
                     "Can't get netmask for device %s\n",
                     dev);
        return NULL;
    }

    // Open capture device
    pcap->handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
    if (pcap->handle == NULL) {
        g_set_error (error,
                     CAPTURE_PCAP_ERROR,
                     CAPTURE_PCAP_ERROR_DEVICE_OPEN,
                     "Couldn't open device %s: %s\n",
                     dev, errbuf);
        return NULL;
    }

    // Get datalink to parse packets correctly
    pcap->link = pcap_datalink(pcap->handle);

    // Check linktypes sngrep knowns before start parsing packets
    if ((pcap->link_hl = datalink_size(pcap->link)) == -1) {
        g_set_error (error,
                     CAPTURE_PCAP_ERROR,
                     CAPTURE_PCAP_ERROR_UNKNOWN_LINK,
                     "Unknown link type %d",
                     pcap->link);
        return NULL;
    }

    // Create Vectors for IP and TCP reassembly
    pcap->tcp_reasm = g_sequence_new(NULL);
    pcap->ip_reasm  = g_sequence_new(NULL);

    // Create a new structure to handle this capture source
    CaptureInput *input = g_malloc0(sizeof(CaptureInput));
    input->source = dev;
    input->priv   = pcap;
    input->tech   = CAPTURE_TECH_PCAP;
    input->mode   = CAPTURE_MODE_ONLINE;
    input->start  = capture_input_pcap_start;
    input->stop   = capture_input_pcap_stop;
    input->filter = capture_input_pcap_filter;

    return input;
}

CaptureInput *
capture_input_pcap_offline(const gchar *infile, GError **error)
{
    char errbuf[PCAP_ERRBUF_SIZE];

    // Check if file is standard input
    if (strlen(infile) == 1 && *infile == '-') {
        infile = "/dev/stdin";
        freopen("/dev/tty", "r", stdin);
    }

    // Create a new structure to handle this capture source
    CapturePcap *pcap = g_malloc(sizeof(CapturePcap));

    // Open PCAP file
    if ((pcap->handle = pcap_open_offline(infile, errbuf)) == NULL) {
        gchar *filename = g_path_get_basename(infile);
        g_set_error (error,
                     CAPTURE_PCAP_ERROR,
                     CAPTURE_PCAP_ERROR_FILE_OPEN,
                     "Couldn't open pcap file %s: %s\n",
                     filename, errbuf);
        g_free(filename);
        return NULL;
    }

    // Get datalink to parse packets correctly
    pcap->link = pcap_datalink(pcap->handle);

    // Check linktypes sngrep knowns before start parsing packets
    if ((pcap->link_hl = datalink_size(pcap->link)) == -1) {
        g_set_error (error,
                     CAPTURE_PCAP_ERROR,
                     CAPTURE_PCAP_ERROR_UNKNOWN_LINK,
                     "Unknown link type %d",
                     pcap->link);
        return NULL;
    }

    // Create Vectors for IP and TCP reassembly
    pcap->tcp_reasm = g_sequence_new(NULL);
    pcap->ip_reasm  = g_sequence_new(NULL);

    // Create a new structure to handle this capture source
    CaptureInput *input = g_malloc0(sizeof(CaptureInput));
    input->source = infile;
    input->priv   = pcap;
    input->tech   = CAPTURE_TECH_PCAP;
    input->mode   = CAPTURE_MODE_OFFLINE;
    input->start  = capture_input_pcap_start;
    input->stop   = capture_input_pcap_stop;
    input->filter = capture_input_pcap_filter;

    return input;
}

void
capture_input_pcap_start(CaptureInput *input)
{
    // Get private data
    CapturePcap *pcap = (CapturePcap *) input->priv;

    // Parse available packets
    pcap_loop(pcap->handle, -1, capture_pcap_parse_packet, (u_char *) input);

    // Close input file in offline mode
    if (input->mode == CAPTURE_MODE_OFFLINE) {
        // Finished reading packets
        pcap_close(pcap->handle);
        input->running = FALSE;
    }
}

void
capture_input_pcap_stop(CaptureInput *input)
{
    CapturePcap *pcap = (CapturePcap *)input->priv;

    // Stop capturing packets
    if (pcap->handle && input->running) {
        pcap_breakloop(pcap->handle);
        input->running = FALSE;
    }
}

gboolean
capture_input_pcap_filter(CaptureInput *input, const gchar *filter, GError **error)
{
    // The compiled filter expression
    struct bpf_program bpf;

    // Capture PCAP private data
    CapturePcap *pcap = (CapturePcap *)input->priv;

    //! Check if filter compiles
    if (pcap_compile(pcap->handle, &bpf, filter, 0, pcap->mask) == -1) {
        g_set_error (error,
                     CAPTURE_PCAP_ERROR,
                     CAPTURE_PCAP_ERROR_FILTER_COMPILE,
                     "Couldn't compile filter '%s': %s\n",
                     filter, pcap_geterr(pcap->handle));
        return FALSE;
    }

    // Set capture filter
    if (pcap_setfilter(pcap->handle, &bpf) == -1) {
        g_set_error (error,
                     CAPTURE_PCAP_ERROR,
                     CAPTURE_PCAP_ERROR_FILTER_APPLY,
                     "Couldn't set filter '%s': %s\n",
                     filter, pcap_geterr(pcap->handle));
        return FALSE;
    }

    return TRUE;
}

static void
capture_output_pcap_write(CaptureOutput *output, Packet *packet)
{
    CapturePcap *pcap = output->priv;

    g_return_if_fail(pcap != NULL);
    g_return_if_fail(pcap->dumper != NULL);

    for (GList *l = packet->frames; l != NULL; l = l->next) {
        PacketFrame *frame = l->data;
        pcap_dump((u_char*) pcap->dumper, frame->header, frame->data);
    }
}

static void
capture_output_pcap_close(CaptureOutput *output)
{
    CapturePcap *pcap = output->priv;

    g_return_if_fail(pcap != NULL);
    g_return_if_fail(pcap->dumper != NULL);

    dump_close(pcap->dumper);
}

CaptureOutput *
capture_output_pcap(const gchar *filename, GError **error)
{

    // PCAP Output is only availble if capture has a single input
    // and thas input is from PCAP thech
    CaptureManager *manager = capture_manager();
    g_return_val_if_fail(manager != NULL, NULL);

    if (g_slist_length(manager->inputs) != 1) {
        g_set_error (error,
                     CAPTURE_PCAP_ERROR,
                     CAPTURE_PCAP_ERROR_SAVE_MULTIPLE,
                     "Save is only supported with a single capture input.");
        return NULL;
    }

    CaptureInput *input = g_slist_nth_data(manager->inputs, 0);
    g_return_val_if_fail(input != NULL, NULL);

    if (input->tech != CAPTURE_TECH_PCAP) {
        g_set_error (error,
                     CAPTURE_PCAP_ERROR,
                     CAPTURE_PCAP_ERROR_SAVE_NOT_PCAP,
                     "Save is only supported from PCAP capture inputs.");
        return NULL;
    }

    CapturePcap *input_pcap = input->priv;

    // Create a new structure to handle this capture source
    CapturePcap *pcap = malloc(sizeof(CapturePcap));

    pcap->dumper = pcap_dump_open(input_pcap->handle, filename);

    // Create a new structure to handle this capture dumper
    CaptureOutput *output = malloc(sizeof(CaptureOutput));
    output->priv   = pcap;
    output->write  = capture_output_pcap_dump;
    output->close  = capture_output_pcap_close;
    return output;
}

void
capture_output_pcap_dump(CaptureOutput *output, packet_t *packet)
{
}

void
capture_output_pcap_close(CaptureOutput *output)
{
}

void
capture_pcap_parse_packet(u_char *info, const struct pcap_pkthdr *header, const guchar *content)
{
    // Capture Input info
    CaptureInput *input = (CaptureInput *) info;
    // Capture pcap info
    CapturePcap *pcap = input->priv;
    // Capture manager
    CaptureManager *manager = input->manager;

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
    uint32_t size_payload =  size_capture - pcap->link_hl;
    // Captured packet info
    packet_t *pkt;

    // Ignore packets while capture is paused
    if (manager->paused)
        return;

    // Check if we have reached capture limit
    if (storage_capture_options().limit && sip_calls_count() >= storage_capture_options().limit) {
        // If capture rotation is disabled, just skip this packet
        if (!storage_capture_options().rotate) {
            return;
        }
    }

    // Check maximum capture length
    if (header->caplen > MAX_CAPTURE_LEN)
        return;

    // Copy packet payload
    memcpy(data, content, header->caplen);

    // Check if we have a complete IP packet
    if (!(pkt = capture_packet_reasm_ip(pcap, header, data, &size_payload, &size_capture)))
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
        if (!(pkt = capture_packet_reasm_tcp(pcap, pkt, tcp, payload, size_payload)))
            return;

#if defined(WITH_GNUTLS) || defined(WITH_OPENSSL)
        // Check if packet is TLS
        if (manager->keyfile) {
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
    capture_lock(manager);

    // Check if we can handle this packet
    if (capture_packet_parse(pkt) == 0) {
        // Store this packet
        capture_manager_output_packet(manager, pkt);
        // Allow Interface refresh and user input actions
        capture_unlock(manager);
        return;
    }


    // Not an interesting packet ...
    packet_destroy(pkt);
    // Allow Interface refresh and user input actions
    capture_unlock(manager);
}

packet_t *
capture_packet_reasm_ip(CapturePcap *capinfo, const struct pcap_pkthdr *header, u_char *packet, uint32_t *size, uint32_t *caplen)
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
    GSequenceIter *it;
    //! Packet containers
    packet_t *pkt;
    //! Storage for IP frame
    frame_t *frame;
    uint32_t len_data = 0;
    //! Link + Extra header size
    uint16_t link_hl = capinfo->link_hl;

    // Skip VLAN header if present
    if (capinfo->link == DLT_EN10MB) {
        struct ether_header *eth = (struct ether_header *) packet;
        if (ntohs(eth->ether_type) == ETHERTYPE_8021Q) {
            link_hl += 4;
        }
    }

#ifdef SLL_HDR_LEN
    if (capinfo->link == DLT_LINUX_SLL) {
        struct sll_header *sll = (struct sll_header *) packet;
        if (ntohs(sll->sll_protocol) == ETHERTYPE_8021Q) {
            link_hl += 4;
        }
    }
#endif

    // Skip NFLOG header if present
    if (capinfo->link == DLT_NFLOG) {
        // Parse NFLOG TLV headers
        while (link_hl + 8 <= *caplen) {
            nflog_tlv_t *tlv = (nflog_tlv_t *) (packet + link_hl);

            if (!tlv) break;

            if (tlv->tlv_type == NFULA_PAYLOAD) {
                link_hl += 4;
                break;
            }

            if (tlv->tlv_length >= 4) {
                link_hl += ((tlv->tlv_length + 3) & ~3); /* next TLV aligned to 4B */
            }
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
    it = g_sequence_get_begin_iter(capinfo->ip_reasm);
    for (pkt = NULL; !g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        pkt = g_sequence_get(it);
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
        g_sequence_append(capinfo->ip_reasm, pkt);
    }

    // Add this IP content length to the total captured of the packet
    pkt->ip_cap_len += ip_len - ip_hl;

    // Calculate how much data we need to complete this packet
    // The total packet size can only be known using the last fragment of the packet
    // where 'No more fragments is enabled' and it's calculated based on the
    // last fragment offset
    if ((ip_off & IP_MF) == 0) {
        pkt->ip_exp_len = ip_frag_off + ip_len - ip_hl;
    }

    // If we have the whole packet (captured length is expected length)
    if (pkt->ip_cap_len == pkt->ip_exp_len) {
        // TODO Dont check the flag, check the holes
        // Calculate assembled IP payload data
        it = g_sequence_get_begin_iter(pkt->frames);
        for (frame = NULL; !g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
            frame = g_sequence_get(it);
            struct ip *frame_ip = (struct ip *) (frame->data + link_hl);
            len_data += ntohs(frame_ip->ip_len) - frame_ip->ip_hl * 4;
        }

        // Check packet content length
        if (len_data > MAX_CAPTURE_LEN)
            return NULL;

        // Initialize memory for the assembly packet
        memset(packet, 0, link_hl + ip_hl + len_data);

        it = g_sequence_get_begin_iter(pkt->frames);
        for (frame = NULL; !g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
            frame = g_sequence_get(it);
            // Get IP header
            struct ip *frame_ip = (struct ip *) (frame->data + link_hl);
            memcpy(packet + link_hl + ip_hl + (ntohs(frame_ip->ip_off) & IP_OFFMASK) * 8,
                   frame->data + link_hl + frame_ip->ip_hl * 4,
                   ntohs(frame_ip->ip_len) - frame_ip->ip_hl * 4);
        }

        *caplen = link_hl + ip_hl + len_data;
        *size = len_data;

        // Return the assembled IP packet
        g_sequence_remove_data(capinfo->ip_reasm, pkt);
        return pkt;
    }

    return NULL;
}

packet_t *
capture_packet_reasm_tcp(CapturePcap *capinfo, packet_t *packet, struct tcphdr *tcp, u_char *payload, int size_payload) {

    GSequenceIter *it = g_sequence_get_begin_iter(capinfo->tcp_reasm);
    packet_t *pkt;
    u_char *new_payload;
    u_char full_payload[MAX_CAPTURE_LEN + 1];

    //! Assembled
    if ((int32_t) size_payload <= 0)
        return packet;

    for (pkt = NULL; !g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        pkt = g_sequence_get(it);
        if (addressport_equals(pkt->src, packet->src) &&
                addressport_equals(pkt->dst, packet->dst)) {
            break;
        }
    }

    // If we already have this packet stored
    if (pkt) {
        frame_t *frame;
        // Append this frames to the original packet
        GSequenceIter *it = g_sequence_get_begin_iter(packet->frames);
        for (frame = NULL; !g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
            frame = g_sequence_get(it);
            packet_add_frame(pkt, frame->header, frame->data);
        }
        // Destroy current packet as its frames belong to the stored packet
        packet_destroy(packet);
    } else {
        // First time this packet has been seen
        pkt = packet;
        // Add To the possible reassembly list
        g_sequence_append(capinfo->tcp_reasm, packet);
    }

    // Store firt tcp sequence
    if (pkt->tcp_seq == 0) {
        pkt->tcp_seq = ntohl(tcp->th_seq);
    }

    // If the first frame of this packet
    if (g_sequence_get_length(pkt->frames) == 1) {
        // Set initial payload
        packet_set_payload(pkt, payload, size_payload);
    } else {
        // Check payload length. Dont handle too big payload packets
        if (pkt->payload_len + size_payload > MAX_CAPTURE_LEN) {
            packet_destroy(pkt);
            g_sequence_remove_data(capinfo->tcp_reasm, pkt);
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

    // Store full payload content
    memset(full_payload, 0, MAX_CAPTURE_LEN);
    memcpy(full_payload, pkt->payload, pkt->payload_len);

    // This packet is ready to be parsed
    int original_size = pkt->payload_len;
    int valid = sip_validate_packet(pkt);
    if (valid == VALIDATE_COMPLETE_SIP) {
        // Full SIP packet!
        g_sequence_remove_data(capinfo->tcp_reasm, pkt);
        return pkt;
    } else if (valid == VALIDATE_MULTIPLE_SIP) {
        g_sequence_remove_data(capinfo->tcp_reasm, pkt);

        // We have a full SIP Packet, but do not remove everything from the reasm queue
        packet_t *cont = packet_clone(pkt);
        int pldiff = original_size - pkt->payload_len;
        if (pldiff > 0 && pldiff < MAX_CAPTURE_LEN) {
            packet_set_payload(cont, full_payload + pkt->payload_len, pldiff);
            g_sequence_append(capinfo->tcp_reasm, cont);
        }

        // Return the full initial packet
        return pkt;
    } else if (valid == VALIDATE_NOT_SIP) {
        // Not a SIP packet, store until PSH flag
        if (tcp->th_flags & TH_PUSH) {
            g_sequence_remove_data(capinfo->tcp_reasm, pkt);
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
            if (storage_capture_options().rtp) {
                call_add_rtp_packet(stream_get_call(stream), packet);
                return 0;
            }
        }
    }
    return 1;
}

ghar *
capture_pcap_error(pcap_t *handle)
{
    return pcap_geterr(handle);
}

gint
capture_packet_time_sorter(gconstpointer a, gconstpointer b, gpointer user_data)
{
    return timeval_is_older(
            packet_time(a),
            packet_time(b)
    );
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
        case DLT_NFLOG:
            return 4;
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
/** @todo
    capture_info_t *capinfo;

    if (g_sequence_get_length(capture_cfg.sources) == 1) {
        capinfo = g_sequence_first(capture_cfg.sources);
        return pcap_dump_open(capinfo->handle, dumpfile);
    }
*/
    return NULL;
}

void
dump_packet(pcap_dumper_t *pd, const packet_t *packet)
{
    if (!pd || !packet)
        return;

    GSequenceIter *it = g_sequence_get_begin_iter(packet->frames);
    frame_t *frame;
    for (;!g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        frame = g_sequence_get(it);
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

const gchar*
capture_input_pcap_file(CaptureManager *manager)
{
    if (g_slist_length(manager->inputs) > 1)
        return "Multiple files";

    CaptureInput *input = manager->inputs->data;
    if (input->tech == CAPTURE_TECH_PCAP && input->mode == CAPTURE_MODE_OFFLINE)
        return input->source;

    return NULL;
}

const gchar *
capture_input_pcap_device(CaptureManager *manager)
{
    if (g_slist_length(manager->inputs) > 1)
        return "multi";

    CaptureInput *input = manager->inputs->data;
    if (input->tech == CAPTURE_TECH_PCAP && input->mode == CAPTURE_MODE_ONLINE)
        return input->source;

    return NULL;
}
