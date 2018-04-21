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
 * @brief Functions to manage hep protocol
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
#include <glib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include "timeval.h"
#include "setting.h"
#include "glib-utils.h"
#include "packet/dissectors/packet_ip.h"
#include "packet/dissectors/packet_udp.h"
#include "packet/dissectors/packet_sip.h"
#include "packet/packet.h"
#include "capture_hep.h"

GQuark
capture_hep_error_quark()
{
    return  g_quark_from_static_string("capture-hep");
}

static gboolean
capture_hep_parse_url(const gchar *url_str, CaptureHepUrl *url, GError **error)
{
    // Parse url in format dissectors:host:port
    gchar **tokens = g_strsplit(url_str, ":", 3);

    // Check we have at least three tokens
    if (g_strv_length(tokens) != 3) {
        g_set_error (error,
                     CAPTURE_HEP_ERROR,
                     CAPTURE_HEP_ERROR_URL_PARSE,
                     "Unable to dissect URL %s: Invalid arguments number",
                     url_str);
        return FALSE;
    }

    // Set capture url data
    url->proto  = g_strdup(tokens[0]);
    url->host   = g_strdup(tokens[1]);
    url->port   = g_strdup(tokens[2]);

    if (g_strcmp0(url->proto, "udp") != 0) {
        g_set_error (error,
                     CAPTURE_HEP_ERROR,
                     CAPTURE_HEP_ERROR_URL_PARSE,
                     "Unable to dissect URL %s: Unsupported protocol %s",
                     url_str, url->proto);
        return FALSE;
    }

    g_strfreev(tokens);
    return TRUE;
}

CaptureInput *
capture_input_hep(const gchar *url, GError **error)
{
    CaptureInput *input;
    CaptureHep *hep;
    struct addrinfo *ai, hints[1] = { { 0 } };

    // Create a new structure to handle this capture source
    hep = g_malloc(sizeof(CaptureHep));

    hep->version   = setting_get_intvalue(SETTING_HEP_LISTEN_VER);
    hep->password  = setting_get_value(SETTING_HEP_LISTEN_PASS);
    if (url != NULL) {
        if (!capture_hep_parse_url(url, &hep->url, error)) {
            return NULL;
        }
    } else {
        hep->url.proto = "udp";
        hep->url.host = setting_get_value(SETTING_HEP_LISTEN_ADDR);
        hep->url.port = setting_get_value(SETTING_HEP_LISTEN_PORT);
    }

    // Check protocol version is support
    if (hep->version != 2 && hep->version != 3) {
        g_set_error (error,
                     CAPTURE_HEP_ERROR,
                     CAPTURE_HEP_ERROR_VERSION,
                     "HEP: Unsupported protocol version %d",
                     hep->version);
        return NULL;
    }

    hints->ai_flags     = AI_NUMERICSERV;
    hints->ai_family    = AF_UNSPEC;
    hints->ai_socktype  = SOCK_DGRAM;
    hints->ai_protocol  = IPPROTO_UDP;

    if (getaddrinfo(hep->url.host, hep->url.port, hints, &ai)) {
        g_set_error (error,
                     CAPTURE_HEP_ERROR,
                     CAPTURE_HEP_ERROR_URL_PARSE,
                     "HEP: failed getaddrinfo() for %s:%s",
                     hep->url.host, hep->url.port);
        return NULL;
    }

    // Create a socket for a new TCP IPv4 connection
    hep->socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (hep->socket < 0) {
        g_set_error (error,
                     CAPTURE_HEP_ERROR,
                     CAPTURE_HEP_ERROR_SOCKET,
                     "HEP: Error creating server socket: %s",
                     g_strerror(errno));
        return NULL;
    }

    // Bind that socket to the requested address and port
    if (bind(hep->socket, ai->ai_addr, ai->ai_addrlen) == -1) {
        g_set_error (error,
                     CAPTURE_HEP_ERROR,
                     CAPTURE_HEP_ERROR_BIND,
                     "HEP: Error binding address: %s",
                     g_strerror(errno));
        return NULL;
    }

    // Create a new structure to handle this capture source
    input = g_malloc0(sizeof(CaptureInput));
    input->source = g_strdup_printf("L:%s", hep->url.port);
    input->priv   = hep;
    input->tech   = CAPTURE_TECH_HEP;
    input->mode   = CAPTURE_MODE_ONLINE;
    input->start  = capture_input_hep_start;
    input->stop   = capture_input_hep_stop;

    // Ceate packet parser tree
    PacketParser *parser = packet_parser_new(input);
    packet_parser_add_proto(parser, parser->dissector_tree, PACKET_SIP);
    input->parser = parser;

    return input;
}

static void
capture_input_hep_receive_v2(CaptureInput *input)
{
    uint8_t family, proto;
    uint32_t pos;
    char buffer[MAX_HEP_BUFSIZE] ;
    //! Source Address
    Address src;
    //! Destination address
    Address dst;
    //! HEP client data
    struct sockaddr hep_client;
    socklen_t hep_client_len;
    struct _CaptureHepHdr hdr;
    struct _CaptureHepTimeHdr hep_time;
    struct _CaptureHepIpHdr hep_ipheader;
#ifdef USE_IPV6
    struct _CaptureHepIp6Hdr hep_ip6header;
#endif
    CaptureHep *hep = input->priv;

    // Initialize buffer
    memset(buffer, 0, MAX_HEP_BUFSIZE);

    /* Receive HEP generic header */
    if (recvfrom(hep->socket, buffer, MAX_HEP_BUFSIZE, 0, &hep_client, &hep_client_len) == -1)
        return;

    /* Copy initial bytes to HEPv2 header */
    memcpy(&hdr, buffer, sizeof(struct _CaptureHepHdr));

    // Check HEP version
    if (hdr.hp_v != 2)
        return;

    /* IP dissectors */
    family = hdr.hp_f;
    /* Proto ID */
    proto = hdr.hp_p;

    pos = sizeof(struct _CaptureHepHdr);

    /* IPv4 */
    if (family == AF_INET) {
        memcpy(&hep_ipheader, (void*) buffer + pos, sizeof(struct _CaptureHepIpHdr));
        inet_ntop(AF_INET, &hep_ipheader.hp_src, src.ip, sizeof(src.ip));
        inet_ntop(AF_INET, &hep_ipheader.hp_dst, dst.ip, sizeof(dst.ip));
        pos += sizeof(struct _CaptureHepIpHdr);
    }
#ifdef USE_IPV6
        /* IPv6 */
    else if(family == AF_INET6) {
        memcpy(&hep_ip6header, (void*) buffer + pos, sizeof(struct _CaptureHepIp6Hdr));
        inet_ntop(AF_INET6, &hep_ip6header.hp6_src, src.ip, sizeof(src.ip));
        inet_ntop(AF_INET6, &hep_ip6header.hp6_dst, dst.ip, sizeof(dst.ip));
        pos += sizeof(struct _CaptureHepIp6Hdr);
    }
#endif

    /* PORTS */
    src.port = ntohs(hdr.hp_sport);
    dst.port = ntohs(hdr.hp_dport);

    /* TIMESTAMP*/
    memcpy(&hep_time, (void*) buffer + pos, sizeof(struct _CaptureHepTimeHdr));
    pos += sizeof(struct _CaptureHepTimeHdr);

    // Create Packet frame data
    PacketFrame *frame = g_malloc0(sizeof(PacketFrame));
    frame->caplen = ntohs(hdr.hp_l) - pos;
    frame->len = frame->caplen;
    frame->ts.tv_sec = hep_time.tv_sec;
    frame->ts.tv_usec = hep_time.tv_usec;

    /* Protocol TYPE */
    /* Capture ID */

    // Copy packet payload
    GByteArray *data = g_byte_array_new_take((guint8 *)(buffer + pos), frame->caplen);

    // Create a new packet
    Packet *packet = packet_new();

    // Generate Packet IP data
    PacketIpData *ip = g_malloc0(sizeof(PacketIpData));
    ip->saddr = src;
    ip->daddr = dst;
    ip->protocol = proto;
    ip->version = (family == AF_INET) ? 4 : 6;
    g_ptr_array_insert(packet->proto, PACKET_IP, ip);

    // Generate Packet UDP data
    PacketUdpData *udp = g_malloc0(sizeof(PacketUdpData));
    udp->sport = src.port;
    udp->dport = dst.port;
    g_ptr_array_insert(packet->proto, PACKET_UDP, udp);

     frame->data = g_byte_array_new();
    g_byte_array_append(frame->data, data->data, data->len);
    packet->frames = g_list_append(packet->frames, frame);

    // Parse SIP payload
    packet_parser_next_dissector(input->parser, packet, data);
}

static void
capture_input_hep_receive_v3(CaptureInput *input)
{
    struct CaptureHepGeneric hg;
    CaptureHepChunkIp4 src_ip4, dst_ip4;
#ifdef USE_IPV6
    CaptureHepChunkIp6 src_ip6, dst_ip6;
#endif
    CaptureHepChunk payload_chunk;
    CaptureHepChunk authkey_chunk;
    CaptureHepChunk uuid_chunk;
    uint8_t family, proto;
    char password[100];
    size_t password_len, uuid_len;
    uint32_t pos;
    char buffer[MAX_HEP_BUFSIZE] ;
    //! Source and Destination Address
    Address src, dst;
    //! HEP client data
    struct sockaddr hep_client;
    socklen_t hep_client_len;
    CaptureHep *hep = input->priv;

    /* Receive HEP generic header */
    if (recvfrom(hep->socket, buffer, MAX_HEP_BUFSIZE, 0, &hep_client, &hep_client_len) == -1)
        return;

    /* Copy initial bytes to HEP Generic header */
    memcpy(&hg, buffer, sizeof(struct CaptureHepGeneric));

    /* header check */
    if (memcmp(hg.header.id, "\x48\x45\x50\x33", 4) != 0)
        return;

    /* IP dissectors */
    family = hg.ip_family.data;
    /* Proto ID */
    proto = hg.ip_proto.data;

    pos = sizeof(struct CaptureHepGeneric);

    /* IPv4 */
    if (family == AF_INET) {
        /* SRC IP */
        memcpy(&src_ip4, (void*) buffer + pos, sizeof(struct _CaptureHepChunkIp4));
        inet_ntop(AF_INET, &src_ip4.data, src.ip, sizeof(src.ip));
        pos += sizeof(struct _CaptureHepChunkIp4);

        /* DST IP */
        memcpy(&dst_ip4, (void*) buffer + pos, sizeof(struct _CaptureHepChunkIp4));
        inet_ntop(AF_INET, &dst_ip4.data, dst.ip, sizeof(src.ip));
        pos += sizeof(struct _CaptureHepChunkIp4);
    }
#ifdef USE_IPV6
        /* IPv6 */
    else if(family == AF_INET6) {
        /* SRC IPv6 */
        memcpy(&src_ip6, (void*) buffer + pos, sizeof(struct _CaptureHepChunkIp6));
        inet_ntop(AF_INET6, &src_ip6.data, src.ip, sizeof(src.ip));
        pos += sizeof(struct _CaptureHepChunkIp6);

        /* DST IP */
        memcpy(&src_ip6, (void*) buffer + pos, sizeof(struct _CaptureHepChunkIp6));
        inet_ntop(AF_INET6, &dst_ip6.data, dst.ip, sizeof(dst.ip));
        pos += sizeof(struct _CaptureHepChunkIp6);
    }
#endif

    /* SRC PORT */
    src.port = ntohs(hg.src_port.data);
    /* DST PORT */
    dst.port = ntohs(hg.dst_port.data);
    /* TIMESTAMP*/
    PacketFrame *frame = g_malloc0(sizeof(PacketFrame));
    frame->ts.tv_sec =  ntohl(hg.time_sec.data);
    frame->ts.tv_usec = ntohl(hg.time_usec.data);

    /* Protocol TYPE */
    /* Capture ID */

    /* auth key */
    if (hep->password != NULL) {
        memcpy(&authkey_chunk, (void*) buffer + pos, sizeof(authkey_chunk));
        pos += sizeof(authkey_chunk);

        password_len = ntohs(authkey_chunk.length) - sizeof(authkey_chunk);
        memcpy(password, (void*) buffer + pos, password_len);
        pos += password_len;

        // Validate the password
        if (strncmp(password, hep->password, password_len) != 0)
            return;
    }

    if (setting_enabled(SETTING_HEP_LISTEN_UUID)) {
        memcpy(&uuid_chunk, (void*) buffer + pos, sizeof(uuid_chunk));
        pos += sizeof(uuid_chunk);

        uuid_len = ntohs(uuid_chunk.length) - sizeof(uuid_chunk);
        pos += uuid_len;
    }

    /* Payload */
    memcpy(&payload_chunk, (void*) buffer + pos, sizeof(payload_chunk));
    pos += sizeof(payload_chunk);

    // Calculate payload size
    frame->caplen = frame->len = ntohs(payload_chunk.length) - sizeof(payload_chunk);

    // Copy packet payload
    GByteArray *data = g_byte_array_new_take((guint8 *)(buffer + pos), frame->caplen);

    // Create a new packet
    Packet *packet = packet_new();

    // Generate Packet IP data
    PacketIpData *ip = g_malloc0(sizeof(PacketIpData));
    ip->saddr = src;
    ip->daddr = dst;
    ip->protocol = proto;
    ip->version = (family == AF_INET) ? 4 : 6;
    g_ptr_array_insert(packet->proto, PACKET_IP, ip);

    // Generate Packet UDP data
    PacketUdpData *udp = g_malloc0(sizeof(PacketUdpData));
    udp->sport = src.port;
    udp->dport = dst.port;
    g_ptr_array_insert(packet->proto, PACKET_UDP, udp);

    // Create Packet frame data
    frame->data = g_byte_array_new();
    g_byte_array_append(frame->data, data->data, data->len);
    packet->frames = g_list_append(packet->frames, frame);

    // Parse SIP payload
    packet_parser_next_dissector(input->parser, packet, data);

}

void
capture_input_hep_start(CaptureInput *input)
{
    CaptureHep *hep = input->priv;

    // Begin accepting connections
    while (hep->socket > 0) {
        // Reset dissector for next packet
        input->parser->current = input->parser->dissector_tree;

        if (hep->version == 2) {
            capture_input_hep_receive_v2(input);
        } else {
            capture_input_hep_receive_v3(input);
        }
    }

    // Leave the thread gracefully
    g_thread_exit(NULL);
}

void
capture_input_hep_stop(CaptureInput *input)
{
    CaptureHep *priv = input->priv;
    if (priv->socket > 0) {
        close(priv->socket);
        priv->socket = -1;
    }
}

const char *
capture_input_hep_port(CaptureManager *manager)
{
    for (GSList *l = manager->inputs; l != NULL; l = l->next) {
        CaptureInput *input = l->data;
        if (input->tech == CAPTURE_TECH_HEP) {
            CaptureHep *hep = input->priv;
            return hep->url.port;
        }
    }
}

CaptureOutput *
capture_output_hep(const gchar *url, GError **error)
{
    struct addrinfo *ai, hints[1] = { { 0 } };

    // Create a new structure to handle this capture sink
    CaptureHep *hep = g_malloc(sizeof(CaptureHep));

    // Fill configuration structure
    hep->version    = setting_get_intvalue(SETTING_HEP_SEND_VER);
    hep->password   = setting_get_value(SETTING_HEP_SEND_PASS);
    hep->id         = setting_get_intvalue(SETTING_HEP_SEND_ID);
    if (url != NULL) {
        if (!capture_hep_parse_url(url, &hep->url, error)) {
            return NULL;
        }
    } else {
        hep->url.proto = "udp";
        hep->url.host = setting_get_value(SETTING_HEP_SEND_ADDR);
        hep->url.port = setting_get_value(SETTING_HEP_SEND_PORT);
    }

    // Check protocol version is support
    if (hep->version != 2 && hep->version != 3) {
        g_set_error (error,
                     CAPTURE_HEP_ERROR,
                     CAPTURE_HEP_ERROR_VERSION,
                     "HEP client: Unsupported protocol version %d",
                     hep->version);
        return NULL;
    }

    hints->ai_flags     = AI_NUMERICSERV;
    hints->ai_family    = AF_UNSPEC;
    hints->ai_socktype  = SOCK_DGRAM;
    hints->ai_protocol  = IPPROTO_UDP;

    if (getaddrinfo(hep->url.host, hep->url.port, hints, &ai)) {
        g_set_error (error,
                     CAPTURE_HEP_ERROR,
                     CAPTURE_HEP_ERROR_URL_PARSE,
                     "HEP client: failed getaddrinfo() for %s:%s",
                     hep->url.host, hep->url.port);
        return NULL;
    }

    hep->socket = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (hep->socket < 0) {
        g_set_error (error,
                     CAPTURE_HEP_ERROR,
                     CAPTURE_HEP_ERROR_SOCKET,
                     "Error creating client socket: %s",
                     g_strerror(errno));
        return NULL;
    }

    if (connect(hep->socket, ai->ai_addr, (socklen_t) (ai->ai_addrlen)) == -1) {
        if (errno != EINPROGRESS) {
            g_set_error (error,
                         CAPTURE_HEP_ERROR,
                         CAPTURE_HEP_ERROR_CONNECT,
                         "Error connecting: %s",
                         g_strerror(errno));
            return NULL;
        }
    }

    CaptureOutput *output = g_malloc0(sizeof(CaptureOutput));
    output->tech    = CAPTURE_TECH_PCAP;
    output->sink    = g_strdup_printf("H:%s", hep->url.port);
    output->priv    = hep;
    output->write   = capture_output_hep_write;
    output->close   = capture_output_hep_close;

    return output;
}

void
capture_output_hep_write_v2(CaptureOutput *output, Packet *packet)
{
    void* buffer;
    uint32_t buflen = 0, tlen = 0;
    struct _CaptureHepHdr hdr;
    struct _CaptureHepTimeHdr hep_time;
    struct _CaptureHepIpHdr hep_ipheader;
#ifdef USE_IPV6
    struct _CaptureHepIp6Hdr hep_ip6header;
#endif

    // Get HEP output data
    CaptureHep *hep = output->priv;

    // Get first frame information (for timestamps)
    PacketFrame *frame = g_list_nth_data(packet->frames, 0);

    // Packet IP Data
    PacketIpData *ip = g_ptr_array_index(packet->proto, PACKET_IP);
    g_return_if_fail(ip != NULL);

    // Packet UDP Data
    PacketUdpData *udp = g_ptr_array_index(packet->proto, PACKET_UDP);
    g_return_if_fail(udp != NULL);

    // Packet SIP Data
    PacketSipData *sip = g_ptr_array_index(packet->proto, PACKET_SIP);
    g_return_if_fail(sip != NULL);

    /* Version && dissectors */
    hdr.hp_v = 2;
    hdr.hp_f = (guint8) (ip->version == 4 ? AF_INET : AF_INET6);
    hdr.hp_p = ip->protocol;
    hdr.hp_sport = htons(udp->sport);
    hdr.hp_dport = htons(udp->dport);

    /* Timestamp */
    hep_time.tv_sec = (guint32) frame->ts.tv_sec;
    hep_time.tv_usec = (guint32) frame->ts.tv_usec;
    hep_time.captid = hep->id;

    /* Calculate initial HEP packet size */
    tlen = sizeof(struct _CaptureHepHdr) + sizeof(struct _CaptureHepTimeHdr);

    /* IPv4 */
    if (ip->version == 4) {
        inet_pton(AF_INET, ip->saddr.ip, &hep_ipheader.hp_src);
        inet_pton(AF_INET, ip->daddr.ip, &hep_ipheader.hp_dst);
        tlen += sizeof(struct _CaptureHepIpHdr);
        hdr.hp_l += sizeof(struct _CaptureHepIpHdr);
    }

#ifdef USE_IPV6
    /* IPv6 */
    else if(ip->version == 6) {
        inet_pton(AF_INET6, ip->saddr.ip, &hep_ip6header.hp6_src);
        inet_pton(AF_INET6, ip->daddr.ip, &hep_ip6header.hp6_dst);
        tlen += sizeof(struct _CaptureHepIp6Hdr);
        hdr.hp_l += sizeof(struct _CaptureHepIp6Hdr);
    }
#endif

    // Add payload size to the final size of HEP packet
    tlen += strlen(sip->payload);
    hdr.hp_l = htons(tlen);

    // Allocate memory for HEPv2 packet
    if (!(buffer = g_malloc0(tlen)))
        return;

    // Copy basic headers
    buflen = 0;
    memcpy(buffer + buflen, &hdr, sizeof(struct _CaptureHepHdr));
    buflen += sizeof(struct _CaptureHepHdr);

    // Copy IP header
    if (ip->version == 4) {
        memcpy(buffer + buflen, &hep_ipheader, sizeof(struct _CaptureHepIpHdr));
        buflen += sizeof(struct _CaptureHepIpHdr);
    }
#ifdef USE_IPV6
    else if(ip->version == 6) {
        memcpy(buffer + buflen, &hep_ip6header, sizeof(struct _CaptureHepIp6Hdr));
        buflen += sizeof(struct _CaptureHepIp6Hdr);
    }
#endif

    // Copy TImestamp header
    memcpy(buffer + buflen, &hep_time, sizeof(struct _CaptureHepTimeHdr));
    buflen += sizeof(struct _CaptureHepTimeHdr);

    // Now copy payload itself
    memcpy(buffer + buflen, sip->payload, strlen(sip->payload));
    buflen += strlen(sip->payload);

    if (send(hep->socket, buffer, buflen, 0) == -1) {
        return;
    }

    /* FREE */
    g_free(buffer);
}

void
capture_output_hep_write_v3(CaptureOutput *output, Packet *packet)
{
    void* buffer;
    uint32_t buflen = 0, iplen = 0, tlen = 0;
    CaptureHepChunkIp4 src_ip4, dst_ip4;
#ifdef USE_IPV6
    CaptureHepChunkIp6 src_ip6, dst_ip6;
#endif
    CaptureHepChunk payload_chunk;
    CaptureHepChunk authkey_chunk;

    // Get first frame information (for timestamps)
    PacketFrame *frame = g_list_nth_data(packet->frames, 0);

    // Packet IP Data
    PacketIpData *ip = g_ptr_array_index(packet->proto, PACKET_IP);
    g_return_if_fail(ip != NULL);

    // Packet UDP Data
    PacketUdpData *udp = g_ptr_array_index(packet->proto, PACKET_UDP);
    g_return_if_fail(udp != NULL);

    // Packet SIP Data
    PacketSipData *sip = g_ptr_array_index(packet->proto, PACKET_SIP);
    g_return_if_fail(sip != NULL);

    // Get HEP output data
    CaptureHep *hep = output->priv;

    /* header set "HEP3" */
    struct CaptureHepGeneric *hg = g_malloc0(sizeof(struct CaptureHepGeneric));
    memcpy(hg->header.id, "\x48\x45\x50\x33", 4);

    /* IP dissectors */
    hg->ip_family.chunk.vendor_id   = htons(0x0000);
    hg->ip_family.chunk.type_id     = htons(0x0001);
    hg->ip_family.chunk.length      = htons(sizeof(hg->ip_family));
    hg->ip_family.data              = (guint8) (ip->version == 4 ? AF_INET : AF_INET6);

    /* Proto ID */
    hg->ip_proto.chunk.vendor_id    = htons(0x0000);
    hg->ip_proto.chunk.type_id      = htons(0x0002);
    hg->ip_proto.chunk.length       = htons(sizeof(hg->ip_proto));
    hg->ip_proto.data               = (guint8) ip->protocol;

    /* IPv4 */
    if (ip->version == 4) {
        /* SRC IP */
        src_ip4.chunk.vendor_id     = htons(0x0000);
        src_ip4.chunk.type_id       = htons(0x0003);
        src_ip4.chunk.length        = htons(sizeof(src_ip4));
        inet_pton(AF_INET, ip->saddr.ip, &src_ip4.data);

        /* DST IP */
        dst_ip4.chunk.vendor_id     = htons(0x0000);
        dst_ip4.chunk.type_id       = htons(0x0004);
        dst_ip4.chunk.length        = htons(sizeof(dst_ip4));
        inet_pton(AF_INET, ip->daddr.ip, &dst_ip4.data);

        iplen = sizeof(dst_ip4) + sizeof(src_ip4);
    }

#ifdef USE_IPV6
    /* IPv6 */
    else if(ip->version == 6) {
        /* SRC IPv6 */
        src_ip6.chunk.vendor_id     = htons(0x0000);
        src_ip6.chunk.type_id       = htons(0x0005);
        src_ip6.chunk.length        = htons(sizeof(src_ip6));
        inet_pton(AF_INET6, ip->saddr.ip, &src_ip6.data);

        /* DST IPv6 */
        dst_ip6.chunk.vendor_id     = htons(0x0000);
        dst_ip6.chunk.type_id       = htons(0x0006);
        dst_ip6.chunk.length        = htons(sizeof(dst_ip6));
        inet_pton(AF_INET6, ip->daddr.ip, &dst_ip6.data);

        iplen = sizeof(dst_ip6) + sizeof(src_ip6);
    }
#endif

    /* SRC PORT */
    hg->src_port.chunk.vendor_id    = htons(0x0000);
    hg->src_port.chunk.type_id      = htons(0x0007);
    hg->src_port.chunk.length       = htons(sizeof(hg->src_port));
    hg->src_port.data               = htons(udp->sport);

    /* DST PORT */
    hg->dst_port.chunk.vendor_id    = htons(0x0000);
    hg->dst_port.chunk.type_id      = htons(0x0008);
    hg->dst_port.chunk.length       = htons(sizeof(hg->dst_port));
    hg->dst_port.data               = htons(udp->dport);

    /* TIMESTAMP SEC */
    hg->time_sec.chunk.vendor_id    = htons(0x0000);
    hg->time_sec.chunk.type_id      = htons(0x0009);
    hg->time_sec.chunk.length       = htons(sizeof(hg->time_sec));
    hg->time_sec.data               = htonl(frame->ts.tv_sec);

    /* TIMESTAMP USEC */
    hg->time_usec.chunk.vendor_id   = htons(0x0000);
    hg->time_usec.chunk.type_id     = htons(0x000a);
    hg->time_usec.chunk.length      = htons(sizeof(hg->time_usec));
    hg->time_usec.data              = htonl(frame->ts.tv_usec);

    /* Protocol TYPE */
    hg->proto_t.chunk.vendor_id     = htons(0x0000);
    hg->proto_t.chunk.type_id       = htons(0x000b);
    hg->proto_t.chunk.length        = htons(sizeof(hg->proto_t));
    hg->proto_t.data                = 1;

    /* Capture ID */
    hg->capt_id.chunk.vendor_id     = htons(0x0000);
    hg->capt_id.chunk.type_id       = htons(0x000c);
    hg->capt_id.chunk.length        = htons(sizeof(hg->capt_id));
    hg->capt_id.data                = htons(hep->id);

    /* Payload */
    payload_chunk.vendor_id         = htons(0x0000);
    payload_chunk.type_id           = htons(0x000f);
    payload_chunk.length            = htons(sizeof(payload_chunk) + strlen(sip->payload));

    tlen = sizeof(struct CaptureHepGeneric) + strlen(sip->payload) + iplen + sizeof(CaptureHepChunk);

    /* auth key */
    if (hep->password != NULL) {

        tlen += sizeof(CaptureHepChunk);
        /* Auth key */
        authkey_chunk.vendor_id     = htons(0x0000);
        authkey_chunk.type_id       = htons(0x000e);
        authkey_chunk.length        = htons(sizeof(authkey_chunk) + strlen(hep->password));
        tlen += strlen(hep->password);
    }

    /* total */
    hg->header.length = htons(tlen);

    buffer = g_malloc0(tlen);
    memcpy(buffer, hg, sizeof(struct CaptureHepGeneric));
    buflen = sizeof(struct CaptureHepGeneric);

    /* IPv4 */
    if (ip->version == 4) {
        /* SRC IP */
        memcpy(buffer + buflen, &src_ip4, sizeof(struct _CaptureHepChunkIp4));
        buflen += sizeof(struct _CaptureHepChunkIp4);

        memcpy(buffer + buflen, &dst_ip4, sizeof(struct _CaptureHepChunkIp4));
        buflen += sizeof(struct _CaptureHepChunkIp4);
    }

#ifdef USE_IPV6
    /* IPv6 */
    else if(ip->version == 6) {
        /* SRC IPv6 */
        memcpy(buffer + buflen, &src_ip4, sizeof(struct _CaptureHepChunkIp6));
        buflen += sizeof(struct _CaptureHepChunkIp6);

        memcpy(buffer + buflen, &dst_ip6, sizeof(struct _CaptureHepChunkIp6));
        buflen += sizeof(struct _CaptureHepChunkIp6);
    }
#endif

    /* AUTH KEY CHUNK */
    if (hep->password != NULL) {

        memcpy(buffer + buflen, &authkey_chunk, sizeof(struct _CaptureHepChunk));
        buflen += sizeof(struct _CaptureHepChunk);

        /* Now copying payload self */
        memcpy(buffer + buflen, hep->password, strlen(hep->password));
        buflen += strlen(hep->password);
    }

    /* PAYLOAD CHUNK */
    memcpy(buffer + buflen, &payload_chunk, sizeof(struct _CaptureHepChunk));
    buflen += sizeof(struct _CaptureHepChunk);

    /* Now copying payload itself */
    memcpy(buffer + buflen, sip->payload, strlen(sip->payload));
    buflen += strlen(sip->payload);

    if (send(hep->socket, buffer, buflen, 0) == -1) {
        return;
    }

    /* FREE */
    g_free(buffer);
    g_free(hg);
}

void
capture_output_hep_write(CaptureOutput *output, Packet *packet)
{
    CaptureHep *hep = output->priv;
    if (hep->version == 2) {
        capture_output_hep_write_v2(output, packet);
    } else {
        capture_output_hep_write_v3(output, packet);
    }
}

void
capture_output_hep_close(CaptureOutput *output)
{
    CaptureHep *priv = output->priv;
    if (priv->socket > 0) {
        close(priv->socket);
        priv->socket = -1;
    }
}

const gchar *
capture_output_hep_port(CaptureManager *manager)
{
    for (GSList *l = manager->outputs; l != NULL; l = l->next) {
        CaptureOutput *output = l->data;
        if (output->tech == CAPTURE_TECH_HEP) {
            CaptureHep *hep = output->priv;
            return hep->url.port;
        }
    }
}
