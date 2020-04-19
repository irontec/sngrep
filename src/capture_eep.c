/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2018 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2018 Irontec SL. All rights reserved.
 ** Copyright (C) 2012 Homer Project (http://www.sipcapture.org)
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
 *
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 * @author Alexandr Dubovikov <alexandr.dubovikov@gmail.com>
 *
 * @brief Functions to manage eep protocol
 *
 * This file contains declaration of structure and functions to send and
 * receive packet information through HEP-EEP (Extensible Encapsulation Protocol)
 *
 * Additional information about HEP-EEP protocol can be found in sipcature
 * repositories at https://github.com/sipcapture/HEP
 *
 * @note Most of this code has been taken from hep-c and sipgrep (originally
 * written by Alexandr Dubovikov). Modifications of sources to work with
 * sngrep packet structures has been made by Ivan Alonso (Kaian)
 *
 */
#include "config.h"
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <pcap.h>
#include "capture_eep.h"
#include "util.h"
#include "setting.h"

capture_eep_config_t eep_cfg = { 0 };

void *
accept_eep_client(void *data);

int
capture_eep_init()
{
    struct addrinfo *ai, hints[1] = { { 0 } };

    // Setting for EEP client
    if (setting_enabled(SETTING_EEP_SEND)) {
        // Fill configuration structure
        eep_cfg.capt_version = setting_get_intvalue(SETTING_EEP_SEND_VER);
        eep_cfg.capt_host = setting_get_value(SETTING_EEP_SEND_ADDR);
        eep_cfg.capt_port = setting_get_value(SETTING_EEP_SEND_PORT);
        eep_cfg.capt_password = setting_get_value(SETTING_EEP_SEND_PASS);
        eep_cfg.capt_id = setting_get_intvalue(SETTING_EEP_SEND_ID);;

        hints->ai_flags = AI_NUMERICSERV;
        hints->ai_family = AF_UNSPEC;
        hints->ai_socktype = SOCK_DGRAM;
        hints->ai_protocol = IPPROTO_UDP;

        if (getaddrinfo(eep_cfg.capt_host, eep_cfg.capt_port, hints, &ai)) {
            fprintf(stderr, "EEP client: failed getaddrinfo() for %s:%s\n",
                    eep_cfg.capt_host, eep_cfg.capt_port);
            return 1;
        }

        eep_cfg.client_sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (eep_cfg.client_sock < 0) {
            fprintf(stderr, "Sender socket creation failed: %s\n", strerror(errno));
            return 1;
        }

        if (connect(eep_cfg.client_sock, ai->ai_addr, (socklen_t) (ai->ai_addrlen)) == -1) {
            if (errno != EINPROGRESS) {
                fprintf(stderr, "Sender socket creation failed: %s\n", strerror(errno));
                return 1;
            }
        }
    }

    if (setting_enabled(SETTING_EEP_LISTEN)) {
        // Fill configuration structure
        eep_cfg.capt_srv_version = setting_get_intvalue(SETTING_EEP_LISTEN_VER);
        eep_cfg.capt_srv_host = setting_get_value(SETTING_EEP_LISTEN_ADDR);
        eep_cfg.capt_srv_port = setting_get_value(SETTING_EEP_LISTEN_PORT);
        eep_cfg.capt_srv_password = setting_get_value(SETTING_EEP_LISTEN_PASS);

        hints->ai_flags = AI_NUMERICSERV;
        hints->ai_family = AF_UNSPEC;
        hints->ai_socktype = SOCK_DGRAM;
        hints->ai_protocol = IPPROTO_UDP;

        if (getaddrinfo(eep_cfg.capt_srv_host, eep_cfg.capt_srv_port, hints, &ai)) {
            fprintf(stderr, "EEP server: failed getaddrinfo() for %s:%s\n",
                    eep_cfg.capt_srv_host, eep_cfg.capt_srv_port);
            return 1;
        }

        // Create a socket for a new TCP IPv4 connection
        eep_cfg.server_sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (eep_cfg.client_sock < 0) {
            fprintf(stderr, "Error creating server socket: %s\n", strerror(errno));
            return 1;
        }

        // Bind that socket to the requested address and port
        if (bind(eep_cfg.server_sock, ai->ai_addr, ai->ai_addrlen) == -1) {
            fprintf(stderr, "Error binding address: %s\n", strerror(errno));
            return 1;
        }

        // Create a new thread for accepting client connections
        if (pthread_create(&eep_cfg.server_thread, NULL, accept_eep_client, NULL) != 0) {
            fprintf(stderr, "Error creating accept thread: %s\n", strerror(errno));
            return 1;
        }

    }

    // Settings for EEP server
    return 0;
}


void *
accept_eep_client(void *data)
{
    packet_t *pkt;

    // Begin accepting connections
    while (eep_cfg.server_sock > 0) {
        if ((pkt = capture_eep_receive())) {
            // Avoid parsing from multiples sources.
            // Avoid parsing while screen in being redrawn
            capture_lock();
            if (capture_packet_parse(pkt) != 0) {
                packet_destroy(pkt);
            }
            capture_unlock();
        }
    }

    // Leave the thread gracefully
    pthread_exit(NULL);
    return 0;
}

void
capture_eep_deinit()
{
    if (eep_cfg.client_sock)
        close(eep_cfg.client_sock);

    if (eep_cfg.server_sock) {
        close(eep_cfg.server_sock);
        eep_cfg.server_sock = -1;
        //pthread_join(&eep_cfg.server_thread, &ret);
    }
}

const char *
capture_eep_send_port()
{
    return eep_cfg.capt_port;
}

const char *
capture_eep_listen_port()
{
    return eep_cfg.capt_srv_port;
}

int
capture_eep_send(packet_t *pkt)
{
    // Dont send RTP packets
    if (pkt->type == PACKET_RTP)
        return 1;

    // Check we have a connection established
    if (!eep_cfg.client_sock)
        return 1;

    switch (eep_cfg.capt_version) {
        case 2:
            return capture_eep_send_v2(pkt);
        case 3:
            return capture_eep_send_v3(pkt);
    }
    return 1;
}

int
capture_eep_send_v2(packet_t *pkt)
{
    void* buffer;
    uint32_t buflen = 0, tlen = 0;
    struct hep_hdr hdr;
    struct hep_timehdr hep_time;
    struct hep_iphdr hep_ipheader;
#ifdef USE_IPV6
    struct hep_ip6hdr hep_ip6header;
#endif
    unsigned char *data = packet_payload(pkt);
    uint32_t len = packet_payloadlen(pkt);
    frame_t *frame = vector_first(pkt->frames);

    /* Version && proto */
    hdr.hp_v = 2;
    hdr.hp_f = pkt->ip_version == 4 ? AF_INET : AF_INET6;
    hdr.hp_p = pkt->proto;
    hdr.hp_sport = htons(pkt->src.port);
    hdr.hp_dport = htons(pkt->dst.port);

    /* Timestamp */
    hep_time.tv_sec = frame->header->ts.tv_sec;
    hep_time.tv_usec = frame->header->ts.tv_usec;
    hep_time.captid = eep_cfg.capt_id;

    /* Calculate initial HEP packet size */
    tlen = sizeof(struct hep_hdr) + sizeof(struct hep_timehdr);

    /* IPv4 */
    if (pkt->ip_version == 4) {
        inet_pton(AF_INET, pkt->src.ip, &hep_ipheader.hp_src);
        inet_pton(AF_INET, pkt->dst.ip, &hep_ipheader.hp_dst);
        tlen += sizeof(struct hep_iphdr);
        hdr.hp_l += sizeof(struct hep_iphdr);
    }

#ifdef USE_IPV6
    /* IPv6 */
    else if(pkt->ip_version == 6) {
        inet_pton(AF_INET6, pkt->src.ip, &hep_ip6header.hp6_src);
        inet_pton(AF_INET6, pkt->dst.ip, &hep_ip6header.hp6_dst);
        tlen += sizeof(struct hep_ip6hdr);
        hdr.hp_l += sizeof(struct hep_ip6hdr);
    }
#endif

    // Add payload size to the final size of HEP packet
    tlen += len;
    hdr.hp_l = htons(tlen);

    // Allocate memory for HEPv2 packet
    if (!(buffer = sng_malloc(tlen)))
        return 1;

    // Copy basic headers
    buflen = 0;
    memcpy((void*) buffer + buflen, &hdr, sizeof(struct hep_hdr));
    buflen += sizeof(struct hep_hdr);

    // Copy IP header
    if (pkt->ip_version == 4) {
        memcpy((void*) buffer + buflen, &hep_ipheader, sizeof(struct hep_iphdr));
        buflen += sizeof(struct hep_iphdr);
    }
#ifdef USE_IPV6
    else if(pkt->ip_version == 6) {
        memcpy((void*) buffer + buflen, &hep_ip6header, sizeof(struct hep_ip6hdr));
        buflen += sizeof(struct hep_ip6hdr);
    }
#endif

    // Copy TImestamp header
    memcpy((void*) buffer + buflen, &hep_time, sizeof(struct hep_timehdr));
    buflen += sizeof(struct hep_timehdr);

    // Now copy payload itself
    memcpy((void*) buffer + buflen, data, len);
    buflen += len;

    if (send(eep_cfg.client_sock, buffer, buflen, 0) == -1) {
        return 1;
    }

    /* FREE */
    sng_free(buffer);

    return 1;
}

int
capture_eep_send_v3(packet_t *pkt)
{
    struct hep_generic *hg = NULL;
    void* buffer;
    uint32_t buflen = 0, iplen = 0, tlen = 0;
    hep_chunk_ip4_t src_ip4, dst_ip4;
#ifdef USE_IPV6
    hep_chunk_ip6_t src_ip6, dst_ip6;
#endif
    hep_chunk_t payload_chunk;
    hep_chunk_t authkey_chunk;
    frame_t *frame = vector_first(pkt->frames);
    unsigned char *data = packet_payload(pkt);
    uint32_t len = packet_payloadlen(pkt);

    hg = sng_malloc(sizeof(struct hep_generic));

    /* header set "HEP3" */
    memcpy(hg->header.id, "\x48\x45\x50\x33", 4);

    /* IP proto */
    hg->ip_family.chunk.vendor_id = htons(0x0000);
    hg->ip_family.chunk.type_id = htons(0x0001);
    hg->ip_family.data = pkt->ip_version == 4 ? AF_INET : AF_INET6;
    hg->ip_family.chunk.length = htons(sizeof(hg->ip_family));

    /* Proto ID */
    hg->ip_proto.chunk.vendor_id = htons(0x0000);
    hg->ip_proto.chunk.type_id = htons(0x0002);
    hg->ip_proto.data = pkt->proto;
    hg->ip_proto.chunk.length = htons(sizeof(hg->ip_proto));

    /* IPv4 */
    if (pkt->ip_version == 4) {
        /* SRC IP */
        src_ip4.chunk.vendor_id = htons(0x0000);
        src_ip4.chunk.type_id = htons(0x0003);
        inet_pton(AF_INET, pkt->src.ip, &src_ip4.data);
        src_ip4.chunk.length = htons(sizeof(src_ip4));

        /* DST IP */
        dst_ip4.chunk.vendor_id = htons(0x0000);
        dst_ip4.chunk.type_id = htons(0x0004);
        inet_pton(AF_INET, pkt->dst.ip, &dst_ip4.data);
        dst_ip4.chunk.length = htons(sizeof(dst_ip4));

        iplen = sizeof(dst_ip4) + sizeof(src_ip4);
    }

#ifdef USE_IPV6
    /* IPv6 */
    else if(pkt->ip_version == 6) {
        /* SRC IPv6 */
        src_ip6.chunk.vendor_id = htons(0x0000);
        src_ip6.chunk.type_id = htons(0x0005);
        inet_pton(AF_INET6, pkt->src.ip, &src_ip6.data);
        src_ip6.chunk.length = htons(sizeof(src_ip6));

        /* DST IPv6 */
        dst_ip6.chunk.vendor_id = htons(0x0000);
        dst_ip6.chunk.type_id = htons(0x0006);
        inet_pton(AF_INET6, pkt->dst.ip, &dst_ip6.data);
        dst_ip6.chunk.length = htons(sizeof(dst_ip6));

        iplen = sizeof(dst_ip6) + sizeof(src_ip6);
    }
#endif

    /* SRC PORT */
    hg->src_port.chunk.vendor_id = htons(0x0000);
    hg->src_port.chunk.type_id = htons(0x0007);
    hg->src_port.data = htons(pkt->src.port);
    hg->src_port.chunk.length = htons(sizeof(hg->src_port));

    /* DST PORT */
    hg->dst_port.chunk.vendor_id = htons(0x0000);
    hg->dst_port.chunk.type_id = htons(0x0008);
    hg->dst_port.data = htons(pkt->dst.port);
    hg->dst_port.chunk.length = htons(sizeof(hg->dst_port));

    /* TIMESTAMP SEC */
    hg->time_sec.chunk.vendor_id = htons(0x0000);
    hg->time_sec.chunk.type_id = htons(0x0009);
    hg->time_sec.data = htonl(frame->header->ts.tv_sec);
    hg->time_sec.chunk.length = htons(sizeof(hg->time_sec));

    /* TIMESTAMP USEC */
    hg->time_usec.chunk.vendor_id = htons(0x0000);
    hg->time_usec.chunk.type_id = htons(0x000a);
    hg->time_usec.data = htonl(frame->header->ts.tv_usec);
    hg->time_usec.chunk.length = htons(sizeof(hg->time_usec));

    /* Protocol TYPE */
    hg->proto_t.chunk.vendor_id = htons(0x0000);
    hg->proto_t.chunk.type_id = htons(0x000b);
    hg->proto_t.data = 1;
    hg->proto_t.chunk.length = htons(sizeof(hg->proto_t));

    /* Capture ID */
    hg->capt_id.chunk.vendor_id = htons(0x0000);
    hg->capt_id.chunk.type_id = htons(0x000c);
    hg->capt_id.data = htons(eep_cfg.capt_id);
    hg->capt_id.chunk.length = htons(sizeof(hg->capt_id));

    /* Payload */
    payload_chunk.vendor_id = htons(0x0000);
    payload_chunk.type_id = htons(0x000f);
    payload_chunk.length = htons(sizeof(payload_chunk) + len);

    tlen = sizeof(struct hep_generic) + len + iplen + sizeof(hep_chunk_t);

    /* auth key */
    if (eep_cfg.capt_password != NULL) {

        tlen += sizeof(hep_chunk_t);
        /* Auth key */
        authkey_chunk.vendor_id = htons(0x0000);
        authkey_chunk.type_id = htons(0x000e);
        authkey_chunk.length = htons(sizeof(authkey_chunk) + strlen(eep_cfg.capt_password));
        tlen += strlen(eep_cfg.capt_password);
    }

    /* total */
    hg->header.length = htons(tlen);

    if (!(buffer = sng_malloc(tlen))) {
        sng_free(hg);
        return 1;
    }
    memcpy((void*) buffer, hg, sizeof(struct hep_generic));
    buflen = sizeof(struct hep_generic);

    /* IPv4 */
    if (pkt->ip_version == 4) {
        /* SRC IP */
        memcpy((void*) buffer + buflen, &src_ip4, sizeof(struct hep_chunk_ip4));
        buflen += sizeof(struct hep_chunk_ip4);

        memcpy((void*) buffer + buflen, &dst_ip4, sizeof(struct hep_chunk_ip4));
        buflen += sizeof(struct hep_chunk_ip4);
    }

#ifdef USE_IPV6
    /* IPv6 */
    else if(pkt->ip_version == 6) {
        /* SRC IPv6 */
        memcpy((void*) buffer+buflen, &src_ip6, sizeof(struct hep_chunk_ip6));
        buflen += sizeof(struct hep_chunk_ip6);

        memcpy((void*) buffer+buflen, &dst_ip6, sizeof(struct hep_chunk_ip6));
        buflen += sizeof(struct hep_chunk_ip6);
    }
#endif

    /* AUTH KEY CHUNK */
    if (eep_cfg.capt_password != NULL) {

        memcpy((void*) buffer + buflen, &authkey_chunk, sizeof(struct hep_chunk));
        buflen += sizeof(struct hep_chunk);

        /* Now copying payload self */
        memcpy((void*) buffer + buflen, eep_cfg.capt_password, strlen(eep_cfg.capt_password));
        buflen += strlen(eep_cfg.capt_password);
    }

    /* PAYLOAD CHUNK */
    memcpy((void*) buffer + buflen, &payload_chunk, sizeof(struct hep_chunk));
    buflen += sizeof(struct hep_chunk);

    /* Now copying payload itself */
    memcpy((void*) buffer + buflen, data, len);
    buflen += len;

    if (send(eep_cfg.client_sock, buffer, buflen, 0) == -1) {
        return 1;
    }

    /* FREE */
    sng_free(buffer);
    sng_free(hg);
    return 0;
}

packet_t *
capture_eep_receive()
{
    switch (eep_cfg.capt_srv_version) {
        case 2:
            return capture_eep_receive_v2();
        case 3:
            return capture_eep_receive_v3();
    }
    return NULL;
}

packet_t *
capture_eep_receive_v2()
{
    uint8_t family, proto;
    unsigned char *payload = 0;
    uint32_t pos;
    char buffer[MAX_CAPTURE_LEN] ;
    //! Source Address
    address_t src;
    //! Destination address
    address_t dst;
    //! Packet header
    struct pcap_pkthdr header;
    //! New created packet pointer
    packet_t *pkt;
    //! EEP client data
    struct sockaddr eep_client;
    socklen_t eep_client_len;
    struct hep_hdr hdr;
    struct hep_timehdr hep_time;
    struct hep_iphdr hep_ipheader;
#ifdef USE_IPV6
    struct hep_ip6hdr hep_ip6header;
#endif

    // Initialize buffer
    memset(buffer, 0, MAX_CAPTURE_LEN);

    /* Receive EEP generic header */
    if (recvfrom(eep_cfg.server_sock, buffer, MAX_CAPTURE_LEN, 0, &eep_client, &eep_client_len) == -1)
        return NULL;

    /* Copy initial bytes to HEPv2 header */
    memcpy(&hdr, buffer, sizeof(struct hep_hdr));

    // Check HEP version
    if (hdr.hp_v != 2)
        return NULL;

    /* IP proto */
    family = hdr.hp_f;
    /* Proto ID */
    proto = hdr.hp_p;

    pos = sizeof(struct hep_hdr);

    /* IPv4 */
    if (family == AF_INET) {
        memcpy(&hep_ipheader, (void*) buffer + pos, sizeof(struct hep_iphdr));
        inet_ntop(AF_INET, &hep_ipheader.hp_src, src.ip, sizeof(src.ip));
        inet_ntop(AF_INET, &hep_ipheader.hp_dst, dst.ip, sizeof(dst.ip));
        pos += sizeof(struct hep_iphdr);
    }
#ifdef USE_IPV6
    /* IPv6 */
    else if(family == AF_INET6) {
        memcpy(&hep_ip6header, (void*) buffer + pos, sizeof(struct hep_ip6hdr));
        inet_ntop(AF_INET6, &hep_ip6header.hp6_src, src.ip, sizeof(src.ip));
        inet_ntop(AF_INET6, &hep_ip6header.hp6_dst, dst.ip, sizeof(dst.ip));
        pos += sizeof(struct hep_ip6hdr);
    }
#endif

    /* PORTS */
    src.port = ntohs(hdr.hp_sport);
    dst.port = ntohs(hdr.hp_dport);

    /* TIMESTAMP*/
    memcpy(&hep_time, (void*) buffer + pos, sizeof(struct hep_timehdr));
    pos += sizeof(struct hep_timehdr);
    header.ts.tv_sec = hep_time.tv_sec;
    header.ts.tv_usec = hep_time.tv_usec;

    /* Protocol TYPE */
    /* Capture ID */

    // Calculate payload size (Total size - headers size)
    header.caplen = header.len = ntohs(hdr.hp_l) - pos;

    // Copy packet payload
    payload = sng_malloc(header.caplen + 1);
    memcpy(payload, (void*) buffer + pos, header.caplen);

    // Create a new packet
    pkt = packet_create((family == AF_INET) ? 4 : 6, proto, src, dst, 0);
    packet_add_frame(pkt, &header, payload);
    packet_set_transport_data(pkt, src.port, dst.port);
    packet_set_type(pkt, PACKET_SIP_UDP);
    packet_set_payload(pkt, payload, header.caplen);

    /* FREE */
    sng_free(payload);
    return pkt;

}


/**
 * @brief Received a HEP3 packet
 *
 * This function receives HEP protocol payload and converts it
 * to a Packet information. This code has been updated based on
 * Kamailio sipcapture module.
 *
 * @return packet pointer
 */
packet_t *
capture_eep_receive_v3()
{

    struct hep_generic hg;
    hep_chunk_ip4_t src_ip4, dst_ip4;
#ifdef USE_IPV6
    hep_chunk_ip6_t src_ip6, dst_ip6;
#endif
    hep_chunk_t payload_chunk;
    hep_chunk_t authkey_chunk;
    hep_chunk_t uuid_chunk;
    char password[100];
    int password_len;
    unsigned char *payload = 0;
    uint32_t total_len, pos;
    char buffer[MAX_CAPTURE_LEN] ;
    //! Source and Destination Address
    address_t src, dst;
    //! EEP client data
    struct sockaddr eep_client;
    socklen_t eep_client_len;
    //! Packet header
    struct pcap_pkthdr header;
    //! New created packet pointer
    packet_t *pkt;

    /* Receive EEP generic header */
    if (recvfrom(eep_cfg.server_sock, buffer, MAX_CAPTURE_LEN, 0, &eep_client, &eep_client_len) == -1)
        return NULL;

    // Initialize structs
    memset(&hg, 0, sizeof(hep_generic_t));
    memset(&password, 0, sizeof(password));
    memset(&src, 0, sizeof(address_t));
    memset(&dst, 0, sizeof(address_t));
    memset(&header, 0, sizeof(struct pcap_pkthdr));

    /* Copy initial bytes to EEP Generic header */
    memcpy(&hg.header, buffer, sizeof(struct hep_generic));

    /* header check */
    if (memcmp(hg.header.id, "\x48\x45\x50\x33", 4) != 0)
        return NULL;

    total_len = ntohs(hg.header.length);
    pos = sizeof(hep_ctrl_t);

    while (pos < total_len) {

        hep_chunk_t *chunk = (struct hep_chunk*) (buffer + pos);
        int chunk_vendor = ntohs(chunk->vendor_id);
        int chunk_type = ntohs(chunk->type_id);
        int chunk_len = ntohs(chunk->length);

        /* Bad length, drop packet */
        if (chunk_len == 0) {
            return NULL;
        }

        /* Skip not general chunks */
        if (chunk_vendor != 0) {
            pos += chunk_len;
            continue;
        }

        switch (chunk_type) {
            case CAPTURE_EEP_CHUNK_INVALID:
                return NULL;
            case CAPTURE_EEP_CHUNK_FAMILY:
                memcpy(&hg.ip_family, (void*) buffer + pos, sizeof(hep_chunk_uint8_t));
                break;
            case CAPTURE_EEP_CHUNK_PROTO:
                memcpy(&hg.ip_proto, (void*) buffer + pos, sizeof(hep_chunk_uint8_t));
                break;
            case CAPTURE_EEP_CHUNK_SRC_IP4:
                memcpy(&src_ip4, (void*) buffer + pos, sizeof(struct hep_chunk_ip4));
                inet_ntop(AF_INET, &src_ip4.data, src.ip, sizeof(src.ip));
                break;
            case CAPTURE_EEP_CHUNK_DST_IP4:
                memcpy(&dst_ip4, (void*) buffer + pos, sizeof(struct hep_chunk_ip4));
                inet_ntop(AF_INET, &dst_ip4.data, dst.ip, sizeof(src.ip));
                break;
#ifdef USE_IPV6
            case CAPTURE_EEP_CHUNK_SRC_IP6:
                memcpy(&src_ip6, (void*) buffer + pos, sizeof(struct hep_chunk_ip6));
                inet_ntop(AF_INET6, &src_ip6.data, src.ip, sizeof(src.ip));
                break;
            case CAPTURE_EEP_CHUNK_DST_IP6:
                memcpy(&dst_ip6, (void*) buffer + pos, sizeof(struct hep_chunk_ip6));
                inet_ntop(AF_INET6, &dst_ip6.data, dst.ip, sizeof(dst.ip));
                break;
#endif
            case CAPTURE_EEP_CHUNK_SRC_PORT:
                memcpy(&hg.src_port, (void*) buffer + pos, sizeof(hep_chunk_uint16_t));
                src.port = ntohs(hg.src_port.data);
                break;
            case CAPTURE_EEP_CHUNK_DST_PORT:
                memcpy(&hg.dst_port, (void*) buffer + pos, sizeof(hep_chunk_uint16_t));
                dst.port = ntohs(hg.dst_port.data);
                break;
            case CAPTURE_EEP_CHUNK_TS_SEC:
                memcpy(&hg.time_sec, (void*) buffer + pos, sizeof(hep_chunk_uint32_t));
                header.ts.tv_sec = ntohl(hg.time_sec.data);
                break;
            case CAPTURE_EEP_CHUNK_TS_USEC:
                memcpy(&hg.time_usec, (void*) buffer + pos, sizeof(hep_chunk_uint32_t));
                header.ts.tv_usec = ntohl(hg.time_usec.data);
                break;
            case CAPTURE_EEP_CHUNK_PROTO_TYPE:
                memcpy(&hg.proto_t, (void*) buffer + pos, sizeof(hep_chunk_uint8_t));
                break;
            case CAPTURE_EEP_CHUNK_CAPT_ID:
                memcpy(&hg.capt_id, (void*) buffer + pos, sizeof(hep_chunk_uint32_t));
                break;
            case CAPTURE_EEP_CHUNK_KEEP_TM:
                break;
            case CAPTURE_EEP_CHUNK_AUTH_KEY:
                memcpy(&authkey_chunk, (void*) buffer + pos, sizeof(authkey_chunk));
                password_len = ntohs(authkey_chunk.length) - sizeof(authkey_chunk);
                memcpy(password, (void*) buffer + pos + sizeof(hep_chunk_t), password_len);
                break;
            case CAPTURE_EEP_CHUNK_PAYLOAD:
                memcpy(&payload_chunk, (void*) buffer + pos, sizeof(payload_chunk));
                header.caplen = header.len = chunk_len - sizeof(hep_chunk_t);
                payload = sng_malloc(header.caplen);
                memcpy(payload, (void*) buffer + pos + sizeof(hep_chunk_t), header.caplen);
                break;
            case CAPTURE_EEP_CHUNK_CORRELATION_ID:
                break;
            default:
                break;
        }

        // Parse next chunk
        pos += chunk_len;
    }

    // Validate password
    if (eep_cfg.capt_srv_password != NULL) {
        // No password in packet
        if (strlen(password) == 0)
            return NULL;
        // Check password matches configured
        if (strncmp(password, eep_cfg.capt_srv_password, strlen(eep_cfg.capt_srv_password)) != 0)
            return NULL;
    }

    // Create a new packet
    pkt = packet_create((hg.ip_family.data == AF_INET)?4:6, hg.ip_proto.data, src, dst, 0);
    packet_add_frame(pkt, &header, payload);
    packet_set_type(pkt, PACKET_SIP_UDP);
    packet_set_payload(pkt, payload, header.caplen);

    /* FREE */
    sng_free(payload);
    return pkt;
}

int
capture_eep_set_server_url(const char *url)
{
    char urlstr[256];
    char address[256], port[256];

    memset(address, 0, sizeof(address));
    memset(port, 0, sizeof(port));

    strncpy(urlstr, url, sizeof(urlstr));
    if (sscanf(urlstr, "%*[^:]:%[^:]:%s", address, port) == 2) {
        setting_set_value(SETTING_EEP_LISTEN, SETTING_ON);
        setting_set_value(SETTING_EEP_LISTEN_ADDR, address);
        setting_set_value(SETTING_EEP_LISTEN_PORT, port);
        return 0;
    }
    return 1;
}

int
capture_eep_set_client_url(const char *url)
{
    char urlstr[256];
    char address[256], port[256];

    memset(address, 0, sizeof(address));
    memset(port, 0, sizeof(port));

    strncpy(urlstr, url, sizeof(urlstr));
    if (sscanf(urlstr, "%*[^:]:%[^:]:%s", address, port) == 2) {
        setting_set_value(SETTING_EEP_SEND, SETTING_ON);
        setting_set_value(SETTING_EEP_SEND_ADDR, address);
        setting_set_value(SETTING_EEP_SEND_PORT, port);
        return 0;
    }
    return 1;
}
