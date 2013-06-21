
/**************************************************************************
 **
 **  sngrep - Ncurses ngrep interface for SIP
 **
 **   Copyright (C) 2013 Ivan Alonso (Kaian)
 **   Copyright (C) 2013 Irontec SL. All rights reserved.
 **
 **   This program is free software: you can redistribute it and/or modify
 **   it under the terms of the GNU General Public License as published by
 **   the Free Software Foundation, either version 3 of the License, or
 **   (at your option) any later version.
 **
 **   This program is distributed in the hope that it will be useful,
 **   but WITHOUT ANY WARRANTY; without even the implied warranty of
 **   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **   GNU General Public License for more details.
 **
 **   You should have received a copy of the GNU General Public License
 **   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 ****************************************************************************/
#include <sys/time.h>
#include "pcap.h"
#include "sip.h"

/**
 * Read from pcap file and fill sngrep sctuctures
 * This function will use libpcap files and previous structures to 
 * parse the pcap file.
 * This program is only focused in VoIP calls so we only consider
 * TCP/UDP packets with Ethernet or Linux coocked headers
 * 
 * @param file Full path to PCAP file
 * @return 0 if load has been successfull, 1 otherwise
 *
 */
int load_pcap_file(const char* file) {

    // Temporary packet buffers 
    struct pcap_pkthdr header;        // The header that pcap gives us 
    const u_char *packet;             // The actual packet 
    pcap_t *handle;                   // PCAP file handler
    char errbuf[PCAP_ERRBUF_SIZE];    // Error text (in case of file open error)
    int linktype;                     // Packages Datalink  
    int size_link;                    // Datalink Header size
    struct ether_header *eptr;        // Ethernet header data
    u_short ether_type;               // Ethernet header type
    struct nread_ip *ip;              // IP header data
    int size_ip;                      // IP header size
    struct nread_udp *udp;            // UDP header data
    char msg_header[256];             // XXX Fake header (Like the one from ngrep)
    u_char *msg_payload;              // Packet payload data
    int size_payload;                 // Packet payload size

    // Open PCAP file
    if ((handle = pcap_open_offline(file, errbuf)) == NULL) {
        fprintf(stderr,"Couldn't open pcap file %s: %s\n", file, errbuf);
        return 1;
    }
    
    // Get datalink to parse packages correctly
    linktype = pcap_datalink(handle);

    // Loop through packages
    while ((packet = pcap_next(handle,&header))) {

        // Get link header size from datalink type
        if (linktype == DLT_EN10MB ) {
            eptr = (struct ether_header *) packet;
            if ((ether_type = ntohs(eptr->ether_type)) != ETHERTYPE_IP )
                continue;
            size_link = SIZE_ETHERNET;
        } else if (linktype == DLT_LINUX_SLL) {
            size_link = SLL_HDR_LEN;    
        } else {
            // Something we are not prepared to parse :(
            fprintf(stderr, "Error handing linktype %d\n", linktype);
            return 1;
        }

        // Get IP header 
        ip = (struct nread_ip*)(packet + size_link);
        size_ip = IP_HL(ip)*4;
        // Only interested in UDP packets
        if (ip->ip_p != IPPROTO_UDP )
            continue;

        // Get UDP header
        udp = (struct nread_udp*)(packet + size_link + size_ip);

        // Get package payload
        msg_payload = (u_char *)(packet + size_link + size_ip + SIZE_UDP);
        size_payload = htons(udp->udp_hlen) - SIZE_UDP;
        msg_payload[size_payload] = '\0';

        // XXX Process timestamp
        struct timeval ut_tv = header.ts;
        time_t t = (time_t)ut_tv.tv_sec;

        // XXX Get current time 
        char timestr[200];
        //time_t t = time(NULL);
        struct tm *time = localtime(&t);
        strftime(timestr, sizeof(timestr), "%Y/%m/%d %T", time);

        // XXX Build a header string
        sprintf(msg_header, "U %s.%06ld %s:%u -> %s:%u", 
                    timestr, ut_tv.tv_usec,
                    inet_ntoa(ip->ip_src), htons(udp->udp_sport),
                    inet_ntoa(ip->ip_dst), htons(udp->udp_dport));
        // Parse this header and payload
        sip_parse_message(msg_header, (const char*)msg_payload);
    }

    // Close PCAP file
    pcap_close(handle);

    return 0;
}

