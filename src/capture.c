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
#ifdef WITH_OPENSSL
#include "capture_tls.h"
#endif
#include "sip.h"
#include "option.h"
#include "ui_manager.h"
#ifdef WITH_IPV6
#include <netinet/ip6.h>
#endif

// Capture information
capture_info_t capinfo = { 0 };

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

    return 0;
}

int
capture_offline(const char *infile)
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
    int size_ip;
    //! Source Address
    char ip_src[50];
    //! Destination Address
    char ip_dst[50];
    // UDP header data
    struct nread_udp *udp;
    // TCP header data
    struct nread_tcp *tcp;
    // Packet payload data
    u_char *msg_payload = NULL;
    // Packet payload size
    int size_payload;
    // Parsed message data
    sip_msg_t *msg;
    // Total packet size
    int size_packet;
    // SIP message transport
    int transport; /* 0 UDP, 1 TCP, 2 TLS */
    // Source and Destination Ports
    u_short sport, dport;

    // Ignore packets while capture is paused
    if (capture_is_paused())
        return;

    // Check if we have reached capture limit
    if (capinfo.limit && sip_calls_count() >= capinfo.limit)
        return;

    // Store this packets in output file
    dump_packet(capinfo.pd, header, packet);

    // Get link header size from datalink type
    size_link = datalink_size(capinfo.link);

    // Get IP header
    ip4 = (struct ip*) (packet + size_link);

#ifdef WITH_IPV6
    // Get IPv6 header
    ip6 = (struct ip6_hdr*)(packet + size_link);
#endif

    switch(ip4->ip_v) {
        case 4:
            size_ip = ip4->ip_hl * 4;
            ip_proto = ip4->ip_p;
            ip_len = ntohs(ip4->ip_len);
            strncpy(ip_src, inet_ntoa(ip4->ip_src), INET_ADDRSTRLEN);
            strncpy(ip_dst, inet_ntoa(ip4->ip_dst), INET_ADDRSTRLEN);
            break;
#ifdef WITH_IPV6
        case 6:
            size_ip = sizeof(struct ip6_hdr);
            ip_proto = ip6->ip6_nxt;
            ip_len = ntohs(ip6->ip6_plen);
            inet_ntop(AF_INET6, &ip6->ip6_src, ip_src, INET6_ADDRSTRLEN);
            inet_ntop(AF_INET6, &ip6->ip6_dst, ip_dst, INET6_ADDRSTRLEN);
            break;
#endif
    }

    // Only interested in UDP packets
    if (ip_proto == IPPROTO_UDP) {
        // Set transport UDP
        transport = 0;

        // Get UDP header
        udp = (struct nread_udp*) (packet + size_link + size_ip);
        // Set packet ports
        sport = udp->udp_sport;
        dport = udp->udp_dport;

        // We're only interested in packets with payload
        size_payload = htons(udp->udp_hlen) - SIZE_UDP;
        if (size_payload <= 0)
            return;

        // Get packet payload
        msg_payload = malloc(size_payload + 1);
        memset(msg_payload, 0, size_payload + 1);
        memcpy(msg_payload, (u_char *) (packet + size_link + size_ip + SIZE_UDP), size_payload);

        // Total packet size
        size_packet = size_link + size_ip + SIZE_UDP + size_payload;

    } else if (ip_proto == IPPROTO_TCP) {
        // Set transport TCP
        transport = 1;

        tcp = (struct nread_tcp*) (packet + size_link + size_ip);
        // Set packet ports
        sport = tcp->th_sport;
        dport = tcp->th_dport;

        // We're only interested in packets with payload
        size_payload = ip_len - (size_ip + SIZE_TCP);
        if (size_payload > 0) {
            // Get packet payload
            msg_payload = malloc(size_payload + 1);
            memset(msg_payload, 0, size_payload + 1);
            memcpy(msg_payload, (u_char *) (packet + size_link + size_ip + SIZE_TCP), size_payload);
        }

        // Total packet size
        size_packet = size_link + size_ip + SIZE_TCP + size_payload;
#ifdef WITH_OPENSSL
        if (!msg_payload || !strstr((const char*) msg_payload, "SIP/2.0")) {
            if (capture_get_keyfile()) {
                // Allocate memory for the payload
                msg_payload = malloc(size_payload + 1);
                memset(msg_payload, 0, size_payload + 1);

                // Try to decrypt the packet
                tls_process_segment(ip4, &msg_payload, &size_payload);

                // Check if we have decoded payload
                if (size_payload <= 0)
                    free(msg_payload);

                // Set Transport TLS
                transport = 2;
            }
        }
#endif
    } else {
        // Not handled protocol
        return;
    }

    // We're only interested in packets with payload
    if (size_payload <= 0)
        return;

    // Parse this header and payload
    msg = sip_load_message(ip_src, sport, ip_dst, dport, msg_payload);
    free(msg_payload);

    // This is not a sip message, Bye!
    if (!msg)
        return;

    // Store Transport attribute
    if (transport == 0) {
        msg_set_attribute(msg, SIP_ATTR_TRANSPORT, "UDP");
    } else if (transport == 1) {
        msg_set_attribute(msg, SIP_ATTR_TRANSPORT, "TCP");
    } else if (transport == 2) {
        msg_set_attribute(msg, SIP_ATTR_TRANSPORT, "TLS");
    }

    // Set message PCAP data
    msg->pcap_header = malloc(sizeof(struct pcap_pkthdr));
    memcpy(msg->pcap_header, header, sizeof(struct pcap_pkthdr));
    msg->pcap_packet = malloc(size_packet);
    memcpy(msg->pcap_packet, packet, size_packet);

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
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
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
capture_set_limit(int limit)
{
    capinfo.limit = limit;
}

void
capture_set_paused(int pause)
{
    if (capture_is_online()) {
        if (pause)
            capinfo.status = CAPTURE_ONLINE_PAUSED;
        else
            capinfo.status = CAPTURE_ONLINE;
    }
}

int
capture_is_paused()
{
    return capinfo.status == CAPTURE_ONLINE_PAUSED;
}

const char *
capture_status()
{
    switch(capinfo.status) {
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

int
datalink_size(int datalink)
{
    // Datalink header size
    switch(datalink) {
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
    struct hostent *host;
    const char *hostname;

    // Check if we have already tryied resolve this address
    for (i = 0; i < capinfo.dnscache.count; i++) {
        if (!strcmp(capinfo.dnscache.addr[i], address)) {
            return capinfo.dnscache.hostname[i];
        }
    }

    // Lookup this addres
    host = gethostbyaddr(address, 4, AF_INET);
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
