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
 * @file capture_hep.c
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
#include "glib-extra.h"
#include "capture/packet/packet_hep.h"
#include "capture/packet/packet_ip.h"
#include "capture/packet/packet_udp.h"
#include "capture/packet/packet_sip.h"
#include "packet.h"
#include "capture_hep.h"

GQuark
capture_hep_error_quark()
{
    return g_quark_from_static_string("capture-hep");
}

static gboolean
capture_hep_parse_url(const gchar *url_str, CaptureHepUrl *url, GError **error)
{
    // Parse url in format dissectors:host:port
    gchar **tokens = g_strsplit(url_str, ":", 3);

    // Check we have at least three tokens
    if (g_strv_length(tokens) != 3) {
        g_set_error(error,
                    CAPTURE_HEP_ERROR,
                    CAPTURE_HEP_ERROR_URL_PARSE,
                    "Unable to dissect URL %s: Invalid arguments number",
                    url_str);
        return FALSE;
    }

    // Set capture url data
    url->proto = g_strdup(tokens[0]);
    url->host = g_strdup(tokens[1]);
    url->port = g_strdup(tokens[2]);

    if (g_strcmp0(url->proto, "udp") != 0) {
        g_set_error(error,
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
    struct addrinfo *ai, hints[1] = {{ 0 }};

    // Create a new structure to handle this capture source
    hep = g_malloc(sizeof(CaptureHep));

    hep->version = setting_get_intvalue(SETTING_HEP_LISTEN_VER);
    hep->password = setting_get_value(SETTING_HEP_LISTEN_PASS);
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
        g_set_error(error,
                    CAPTURE_HEP_ERROR,
                    CAPTURE_HEP_ERROR_VERSION,
                    "HEP: Unsupported protocol version %d",
                    hep->version);
        return NULL;
    }

    hints->ai_flags = AI_NUMERICSERV;
    hints->ai_family = AF_UNSPEC;
    hints->ai_socktype = SOCK_DGRAM;
    hints->ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(hep->url.host, hep->url.port, hints, &ai)) {
        g_set_error(error,
                    CAPTURE_HEP_ERROR,
                    CAPTURE_HEP_ERROR_URL_PARSE,
                    "HEP: failed getaddrinfo() for %s:%s",
                    hep->url.host, hep->url.port);
        return NULL;
    }

    // Create a socket for a new TCP IPv4 connection
    hep->socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (hep->socket < 0) {
        g_set_error(error,
                    CAPTURE_HEP_ERROR,
                    CAPTURE_HEP_ERROR_SOCKET,
                    "HEP: Error creating server socket: %s",
                    g_strerror(errno));
        return NULL;
    }

    // Bind that socket to the requested address and port
    if (bind(hep->socket, ai->ai_addr, ai->ai_addrlen) == -1) {
        g_set_error(error,
                    CAPTURE_HEP_ERROR,
                    CAPTURE_HEP_ERROR_BIND,
                    "HEP: Error binding address: %s",
                    g_strerror(errno));
        return NULL;
    }

    // Create a new structure to handle this capture source
    input = g_malloc0(sizeof(CaptureInput));
    input->source = g_strdup_printf("L:%s", hep->url.port);
    input->priv = hep;
    input->tech = CAPTURE_TECH_HEP;
    input->mode = CAPTURE_MODE_ONLINE;
    input->start = capture_input_hep_start;
    input->stop = capture_input_hep_stop;

    // Ceate packet parser tree
    PacketParser *parser = packet_parser_new(input);
    packet_parser_add_proto(parser, parser->dissector_tree, PACKET_HEP);
    input->parser = parser;

    return input;
}

static void
capture_input_hep_receive(CaptureInput *input)
{
    char buffer[MAX_HEP_BUFSIZE];
    struct sockaddr hep_client;
    socklen_t hep_client_len;
    CaptureHep *hep = input->priv;
    PacketParser *parser = input->parser;

    /* Receive HEP generic header */
    gssize received = recvfrom(hep->socket, buffer, MAX_HEP_BUFSIZE, 0, &hep_client, &hep_client_len);
    if ( received == -1)
        return;

    // Convert packet data
    GByteArray *data = g_byte_array_new();
    g_byte_array_append(data, (const guint8 *) buffer, (guint) received);

    // Create a new packet for this data
    Packet *packet = packet_new();
    PacketFrame *frame = g_malloc0(sizeof(PacketFrame));
    frame->data = g_byte_array_new();
    g_byte_array_append(frame->data, data->data, data->len);
    packet->frames = g_list_append(packet->frames, frame);

    // Initialize parser dissector to first one
    parser->current = parser->dissector_tree;

    // Request initial dissector parsing
    data = packet_parser_next_dissector(parser, packet, data);

    // Free not parsed packet data
    if (data != NULL) {
        g_byte_array_free(data, TRUE);
        g_free(frame);

        g_list_free(packet->frames);
        g_free(packet);
    }

}

void *
capture_input_hep_start(CaptureInput *input)
{
    CaptureHep *hep = input->priv;

    // Begin accepting connections
    while (hep->socket > 0) {
        // Reset dissector for next packet
        input->parser->current = input->parser->dissector_tree;
        capture_input_hep_receive(input);

    }

    // Leave the thread gracefully
    g_thread_exit(NULL);

    return NULL;
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

    return NULL;
}

CaptureOutput *
capture_output_hep(const gchar *url, GError **error)
{
    struct addrinfo *ai, hints[1] = {{ 0 }};

    // Create a new structure to handle this capture sink
    CaptureHep *hep = g_malloc(sizeof(CaptureHep));

    // Fill configuration structure
    hep->version = setting_get_intvalue(SETTING_HEP_SEND_VER);
    hep->password = setting_get_value(SETTING_HEP_SEND_PASS);
    hep->id = setting_get_intvalue(SETTING_HEP_SEND_ID);
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
        g_set_error(error,
                    CAPTURE_HEP_ERROR,
                    CAPTURE_HEP_ERROR_VERSION,
                    "HEP client: Unsupported protocol version %d",
                    hep->version);
        return NULL;
    }

    hints->ai_flags = AI_NUMERICSERV;
    hints->ai_family = AF_UNSPEC;
    hints->ai_socktype = SOCK_DGRAM;
    hints->ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(hep->url.host, hep->url.port, hints, &ai)) {
        g_set_error(error,
                    CAPTURE_HEP_ERROR,
                    CAPTURE_HEP_ERROR_URL_PARSE,
                    "HEP client: failed getaddrinfo() for %s:%s",
                    hep->url.host, hep->url.port);
        return NULL;
    }

    hep->socket = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (hep->socket < 0) {
        g_set_error(error,
                    CAPTURE_HEP_ERROR,
                    CAPTURE_HEP_ERROR_SOCKET,
                    "Error creating client socket: %s",
                    g_strerror(errno));
        return NULL;
    }

    if (connect(hep->socket, ai->ai_addr, (socklen_t) (ai->ai_addrlen)) == -1) {
        if (errno != EINPROGRESS) {
            g_set_error(error,
                        CAPTURE_HEP_ERROR,
                        CAPTURE_HEP_ERROR_CONNECT,
                        "Error connecting: %s",
                        g_strerror(errno));
            return NULL;
        }
    }

    CaptureOutput *output = g_malloc0(sizeof(CaptureOutput));
    output->tech = CAPTURE_TECH_PCAP;
    output->sink = g_strdup_printf("H:%s", hep->url.port);
    output->priv = hep;
    output->write = capture_output_hep_write;
    output->close = capture_output_hep_close;

    return output;
}

void
capture_output_hep_write(CaptureOutput *output, Packet *packet)
{
    void *buffer;
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
    struct _CaptureHepGeneric *hg = g_malloc0(sizeof(struct _CaptureHepGeneric));
    memcpy(hg->header.id, "\x48\x45\x50\x33", 4);

    /* IP dissectors */
    hg->ip_family.chunk.vendor_id = htons(0x0000);
    hg->ip_family.chunk.type_id = htons(0x0001);
    hg->ip_family.chunk.length = htons(sizeof(hg->ip_family));
    hg->ip_family.data = (guint8) (ip->version == 4 ? AF_INET : AF_INET6);

    /* Proto ID */
    hg->ip_proto.chunk.vendor_id = htons(0x0000);
    hg->ip_proto.chunk.type_id = htons(0x0002);
    hg->ip_proto.chunk.length = htons(sizeof(hg->ip_proto));
    hg->ip_proto.data = (guint8) ip->protocol;

    /* IPv4 */
    if (ip->version == 4) {
        /* SRC IP */
        src_ip4.chunk.vendor_id = htons(0x0000);
        src_ip4.chunk.type_id = htons(0x0003);
        src_ip4.chunk.length = htons(sizeof(src_ip4));
        inet_pton(AF_INET, ip->saddr.ip, &src_ip4.data);

        /* DST IP */
        dst_ip4.chunk.vendor_id = htons(0x0000);
        dst_ip4.chunk.type_id = htons(0x0004);
        dst_ip4.chunk.length = htons(sizeof(dst_ip4));
        inet_pton(AF_INET, ip->daddr.ip, &dst_ip4.data);

        iplen = sizeof(dst_ip4) + sizeof(src_ip4);
    }

#ifdef USE_IPV6
        /* IPv6 */
    else if (ip->version == 6) {
        /* SRC IPv6 */
        src_ip6.chunk.vendor_id = htons(0x0000);
        src_ip6.chunk.type_id = htons(0x0005);
        src_ip6.chunk.length = htons(sizeof(src_ip6));
        inet_pton(AF_INET6, ip->saddr.ip, &src_ip6.data);

        /* DST IPv6 */
        dst_ip6.chunk.vendor_id = htons(0x0000);
        dst_ip6.chunk.type_id = htons(0x0006);
        dst_ip6.chunk.length = htons(sizeof(dst_ip6));
        inet_pton(AF_INET6, ip->daddr.ip, &dst_ip6.data);

        iplen = sizeof(dst_ip6) + sizeof(src_ip6);
    }
#endif

    /* SRC PORT */
    hg->src_port.chunk.vendor_id = htons(0x0000);
    hg->src_port.chunk.type_id = htons(0x0007);
    hg->src_port.chunk.length = htons(sizeof(hg->src_port));
    hg->src_port.data = htons(udp->sport);

    /* DST PORT */
    hg->dst_port.chunk.vendor_id = htons(0x0000);
    hg->dst_port.chunk.type_id = htons(0x0008);
    hg->dst_port.chunk.length = htons(sizeof(hg->dst_port));
    hg->dst_port.data = htons(udp->dport);

    /* TIMESTAMP SEC */
    hg->time_sec.chunk.vendor_id = htons(0x0000);
    hg->time_sec.chunk.type_id = htons(0x0009);
    hg->time_sec.chunk.length = htons(sizeof(hg->time_sec));
    hg->time_sec.data = htonl(frame->ts.tv_sec);

    /* TIMESTAMP USEC */
    hg->time_usec.chunk.vendor_id = htons(0x0000);
    hg->time_usec.chunk.type_id = htons(0x000a);
    hg->time_usec.chunk.length = htons(sizeof(hg->time_usec));
    hg->time_usec.data = htonl(frame->ts.tv_usec);

    /* Protocol TYPE */
    hg->proto_t.chunk.vendor_id = htons(0x0000);
    hg->proto_t.chunk.type_id = htons(0x000b);
    hg->proto_t.chunk.length = htons(sizeof(hg->proto_t));
    hg->proto_t.data = 1;

    /* Capture ID */
    hg->capt_id.chunk.vendor_id = htons(0x0000);
    hg->capt_id.chunk.type_id = htons(0x000c);
    hg->capt_id.chunk.length = htons(sizeof(hg->capt_id));
    hg->capt_id.data = htons(hep->id);

    /* Payload */
    payload_chunk.vendor_id = htons(0x0000);
    payload_chunk.type_id = htons(0x000f);
    payload_chunk.length = htons(sizeof(payload_chunk) + strlen(sip->payload));

    tlen = sizeof(struct _CaptureHepGeneric) + strlen(sip->payload) + iplen + sizeof(CaptureHepChunk);

    /* auth key */
    if (hep->password != NULL) {

        tlen += sizeof(CaptureHepChunk);
        /* Auth key */
        authkey_chunk.vendor_id = htons(0x0000);
        authkey_chunk.type_id = htons(0x000e);
        authkey_chunk.length = htons(sizeof(authkey_chunk) + strlen(hep->password));
        tlen += strlen(hep->password);
    }

    /* total */
    hg->header.length = htons(tlen);

    buffer = g_malloc0(tlen);
    memcpy((gchar *) buffer, hg, sizeof(struct _CaptureHepGeneric));
    buflen = sizeof(struct _CaptureHepGeneric);

    /* IPv4 */
    if (ip->version == 4) {
        /* SRC IP */
        memcpy((gchar *) buffer + buflen, &src_ip4, sizeof(struct _CaptureHepChunkIp4));
        buflen += sizeof(struct _CaptureHepChunkIp4);

        memcpy((gchar *) buffer + buflen, &dst_ip4, sizeof(struct _CaptureHepChunkIp4));
        buflen += sizeof(struct _CaptureHepChunkIp4);
    }

#ifdef USE_IPV6
        /* IPv6 */
    else if (ip->version == 6) {
        /* SRC IPv6 */
        memcpy((gchar *) buffer + buflen, &src_ip4, sizeof(struct _CaptureHepChunkIp6));
        buflen += sizeof(struct _CaptureHepChunkIp6);

        memcpy((gchar *) buffer + buflen, &dst_ip6, sizeof(struct _CaptureHepChunkIp6));
        buflen += sizeof(struct _CaptureHepChunkIp6);
    }
#endif

    /* AUTH KEY CHUNK */
    if (hep->password != NULL) {

        memcpy((gchar *) buffer + buflen, &authkey_chunk, sizeof(struct _CaptureHepChunk));
        buflen += sizeof(struct _CaptureHepChunk);

        /* Now copying payload self */
        memcpy((gchar *) buffer + buflen, hep->password, strlen(hep->password));
        buflen += strlen(hep->password);
    }

    /* PAYLOAD CHUNK */
    memcpy((gchar *) buffer + buflen, &payload_chunk, sizeof(struct _CaptureHepChunk));
    buflen += sizeof(struct _CaptureHepChunk);

    /* Now copying payload itself */
    memcpy((gchar *) buffer + buflen, sip->payload, strlen(sip->payload));
    buflen += strlen(sip->payload);

    if (send(hep->socket, buffer, buflen, 0) == -1) {
        return;
    }

    /* FREE */
    g_free(buffer);
    g_free(hg);
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

    return NULL;
}
