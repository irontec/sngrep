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

// Capture information
capture_info_t capinfo = { 0 };

int
capture_online(const char *dev, const char *bpf, const char *outfile, int limit)
{
    //! Error string
    char errbuf[PCAP_ERRBUF_SIZE];
    //! The compiled filter expression
    struct bpf_program fp;
    //! Netmask of our sniffing device
    bpf_u_int32 mask;
    //! The IP of our sniffing device
    bpf_u_int32 net;

    // Set capture mode
    capinfo.mode = CAPTURE_ONLINE;
    // Set capture limit
    capinfo.limit = limit;

    // Try to find capture device information
    if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
        fprintf(stderr, "Can't get netmask for device %s\n", dev);
        net = 0;
        mask = 0;
    }

    // Open capture device
    capinfo.handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
    if (capinfo.handle == NULL) {
        fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
        return 2;
    }

    // Validate and set filter expresion
    if (bpf) {
        if (pcap_compile(capinfo.handle, &fp, bpf, 0, net) == -1) {
            fprintf(stderr, "Couldn't parse filter %s: %s\n", bpf, pcap_geterr(capinfo.handle));
            return 2;
        }
        if (pcap_setfilter(capinfo.handle, &fp) == -1) {
            fprintf(stderr, "Couldn't install filter %s: %s\n", bpf, pcap_geterr(capinfo.handle));
            return 2;
        }
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
capture_offline(const char *infile, const char *bpf, int limit)
{
    // Error text (in case of file open error)
    char errbuf[PCAP_ERRBUF_SIZE];
    // The header that pcap gives us
    struct pcap_pkthdr header;
    // The actual packet
    const u_char *packet;
    //! The compiled filter expression
    struct bpf_program fp;

    // Set capture mode
    capinfo.mode = CAPTURE_OFFLINE;
    // Set capture input file
    capinfo.infile = infile;
    // Set capture limit
    capinfo.limit = limit;

    // Open PCAP file
    if ((capinfo.handle = pcap_open_offline(infile, errbuf)) == NULL) {
        fprintf(stderr, "Couldn't open pcap file %s: %s\n", infile, errbuf);
        return 1;
    }

    // Validate and set filter expresion
    if (bpf) {
        if (pcap_compile(capinfo.handle, &fp, bpf, 0, 0) == -1) {
            fprintf(stderr, "Couldn't parse filter %s: %s\n", bpf, pcap_geterr(capinfo.handle));
            return 2;
        }
        if (pcap_setfilter(capinfo.handle, &fp) == -1) {
            fprintf(stderr, "Couldn't install filter %s: %s\n", bpf, pcap_geterr(capinfo.handle));
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

    // Loop through packets
    while ((packet = pcap_next(capinfo.handle, &header))) {
        // Parse packets
        parse_packet((u_char*) "Offline", &header, packet);
    }

    return 0;
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
    pcap_close(capinfo.handle);
}

int
capture_is_online()
{
    return (capinfo.mode == CAPTURE_ONLINE);
}

void
capture_set_paused(int pause)
{
    if (capture_is_online()) {
        capinfo.paused = pause;
    }
}

int
capture_is_paused()
{
    return capinfo.paused;
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

void
parse_packet(u_char *mode, const struct pcap_pkthdr *header, const u_char *packet)
{
    // Datalink Header size
    int size_link;
    // IP header data
    struct nread_ip *ip;
    // IP header size
    int size_ip;
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
    ip = (struct nread_ip*) (packet + size_link);
    size_ip = IP_HL(ip) * 4;

    // Only interested in UDP packets
    if (ip->ip_p == IPPROTO_UDP) {
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

    } else if (ip->ip_p == IPPROTO_TCP) {
        // Set transport TCP
        transport = 1;

        tcp = (struct nread_tcp*) (packet + size_link + size_ip);
        // Set packet ports
        sport = tcp->th_sport;
        dport = tcp->th_dport;

        // We're only interested in packets with payload
        size_payload = ntohs(ip->ip_len) - (size_ip + SIZE_TCP);
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
            if (get_option_value("capture.keyfile")) {
                // Allocate memory for the payload
                msg_payload = malloc(size_payload + 1);
                memset(msg_payload, 0, size_payload + 1);

                // Try to decrypt the packet
                tls_process_segment(ip, &msg_payload, &size_payload);

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
    msg = sip_load_message(header->ts, ip->ip_src, sport, ip->ip_dst, dport, msg_payload);
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

    // Refresh current UI in online mode
    if (capture_is_online())
        ui_new_msg_refresh(msg);
}

void
capture_close()
{
    void* ret = NULL;

    //Close PCAP file
    if (capinfo.handle) {
        if(capture_is_online()) {
            pcap_breakloop(capinfo.handle);
            pthread_join(capinfo.capture_t, &ret);
        } else {
            pcap_close(capinfo.handle);
        }
    }

    // Close dump file
    if (capinfo.pd) {
        dump_close(capinfo.pd);
    }
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
        case DLT_LINUX_SLL:
            return 16;
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
lookup_hostname(struct in_addr *addr)
{
    int i;
    int hostlen;
    struct hostent *host;
    char *hostname;
    char *address;

    // Initialize values
    address = (char *) inet_ntoa(*addr);

    // Check if we have already tryied resolve this address
    for (i = 0; i < capinfo.dnscache.count; i++) {
        if (!strcmp(capinfo.dnscache.addr[i], address)) {
            return capinfo.dnscache.hostname[i];
        }
    }

    // Lookup this addres
    host = gethostbyaddr(addr, 4, AF_INET);
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
