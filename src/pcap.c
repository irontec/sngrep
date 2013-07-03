/**************************************************************************
 **
 ** sngrep - SIP callflow viewer using ngrep
 **
 ** Copyright (C) 2013 Ivan Alonso (Kaian)
 ** Copyright (C) 2013 Irontec SL. All rights reserved.
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
#ifdef WITH_LIBPCAP
/**
 * @file pcap.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in pcap.h
 *
 * sngrep can parse a pcap file to display call flows.
 * This file include the functions that uses libpcap to do so.
 *
 * @todo We could request libpcap to filter the file before being processed
 * and only read sip packages. We also allow UDP packages here, and SIP can
 * use other transports, uh.
 *
 */
#include "pcap.h"
#include "sip.h"

//! FIXME Link type
int linktype;

#ifndef WITH_NGREP

int
online_capture(void *pargv)
{
    char **argv = (char**) pargv;
    int argc = 1;
    char filter_exp[256];
    //! Session handle
    pcap_t *handle;
    //! Device to sniff on
    char dev[] = "any";
    //! Error string
    char errbuf[PCAP_ERRBUF_SIZE];
    //! The compiled filter expression
    struct bpf_program fp;
    //! The filter expression
    //char *filter_exp = (char *) pargs;
    //! Netmask of our sniffing device
    bpf_u_int32 mask;
    //! The IP of our sniffing device
    bpf_u_int32 net;

    //! Build the filter options
    while (argv[argc]) {
        sprintf(filter_exp, "%s %s", filter_exp, argv[argc++]);
    }

    if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
        fprintf(stderr, "Can't get netmask for device %s\n", dev);
        net = 0;
        mask = 0;
    }
    handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
        return(2);
    }
    if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
        fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
        return(2);
    }
    if (pcap_setfilter(handle, &fp) == -1) {
        fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
        return(2);
    }

    // Get datalink to parse packages correctly
    linktype = pcap_datalink(handle);

    // Parse available packages
    pcap_loop(handle, -1, parse_packet, "Online");
    // Close PCAP file
    pcap_close(handle);
}
#endif

int
load_from_file(const char* file)
{
    // PCAP file handler
    pcap_t *handle;
    // Error text (in case of file open error)
    char errbuf[PCAP_ERRBUF_SIZE];
    // The header that pcap gives us
    struct pcap_pkthdr header;
    // The actual packet
    const u_char *packet;

    // Open PCAP file
    if ((handle = pcap_open_offline(file, errbuf)) == NULL) {
        fprintf(stderr, "Couldn't open pcap file %s: %s\n", file, errbuf);
        return 1;
    }

    // Get datalink to parse packages correctly
    linktype = pcap_datalink(handle);

    // Loop through packages
    while ((packet = pcap_next(handle, &header))) {
        // Parse package
        parse_packet("Offline", &header, packet);
    }
    // Close PCAP file
    pcap_close(handle);
    return 0;
}

void
parse_packet(u_char *mode, const struct pcap_pkthdr *header, const u_char *packet)
{
    // Datalink Header size
    int size_link;
    // Ethernet header data
    struct ether_header *eptr;
    // Ethernet header type
    u_short ether_type;
    // IP header data
    struct nread_ip *ip;
    // IP header size
    int size_ip;
    // UDP header data
    struct nread_udp *udp;
    // XXX Fake header (Like the one from ngrep)
    char msg_header[256];
    // Packet payload data
    u_char *msg_payload;
    // Packet payload size
    int size_payload;
    // Parsed message data
    sip_msg_t *msg;

    // Get link header size from datalink type
    if (linktype == DLT_EN10MB) {
        eptr = (struct ether_header *) packet;
        if ((ether_type = ntohs(eptr->ether_type)) != ETHERTYPE_IP) return;
        size_link = SIZE_ETHERNET;
    } else if (linktype == DLT_LINUX_SLL) {
        size_link = SLL_HDR_LEN;
    } else if (linktype == DLT_NULL) {
        size_link = DLT_RAW;
    } else {
        // Something we are not prepared to parse :(
        fprintf(stderr, "Error handing linktype %d\n", linktype);
        return;
    }

    // Get IP header
    ip = (struct nread_ip*) (packet + size_link);
    size_ip = IP_HL(ip) * 4;

    // Only interested in UDP packets
    if (ip->ip_p != IPPROTO_UDP) return;

    // Get UDP header
    udp = (struct nread_udp*) (packet + size_link + size_ip);

    // Get package payload
    msg_payload = (u_char *) (packet + size_link + size_ip + SIZE_UDP);
    size_payload = htons(udp->udp_hlen) - SIZE_UDP;
    msg_payload[size_payload] = '\0';

    // XXX Process timestamp
    struct timeval ut_tv = header->ts;
    time_t t = (time_t) ut_tv.tv_sec;

    // XXX Get current time
    char timestr[200];
    struct tm *time = localtime(&t);
    strftime(timestr, sizeof(timestr), "%Y/%m/%d %T", time);

    // XXX Build a header string
    sprintf(msg_header, "U %s.%06ld", timestr, ut_tv.tv_usec);
    sprintf(msg_header, "%s %s:%u", msg_header, inet_ntoa(ip->ip_src), htons(udp->udp_sport));
    sprintf(msg_header, "%s -> %s:%u", msg_header, inet_ntoa(ip->ip_dst), htons(udp->udp_dport));

    // Parse this header and payload
    if (!strcasecmp(mode, "Online") && (msg = sip_load_message(msg_header, (const char*) msg_payload))) {
        ui_new_msg_refresh(msg);
    }
}
#endif
