/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2019 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2019 Irontec SL. All rights reserved.
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
 * @file packet_hep.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in packet_hep.h
 *
 * Support for HEP transport layer
 *
 */

#include "config.h"
#include <string.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include "glib/glib-extra.h"
#include "packet_ip.h"
#include "packet_udp.h"
#include "parser/packet.h"
#include "setting.h"
#include "packet_hep.h"

/**
 * @brief Received a HEP3 packet
 *
 * This function receives HEP protocol payload and converts it
 * to a Packet information. This code has been updated based on
 * Kamailio sipcapture module.
 *
 * @return packet pointer
 */
static GByteArray *
packet_hep_parse(PacketParser *parser, Packet *packet, GByteArray *data)
{
    CaptureHepChunkIp4 src_ip4, dst_ip4;
#ifdef USE_IPV6
    CaptureHepChunkIp6 src_ip6, dst_ip6;
#endif
    CaptureHepChunk payload_chunk;
    CaptureHepChunk authkey_chunk;
    gchar srcip[ADDRESSLEN], dstip[ADDRESSLEN];
    guint16 sport, dport;
    g_autofree gchar *password = NULL;
    GByteArray *payload = NULL;

    if (data->len < sizeof(CaptureHepGeneric))
        return data;

    // Copy initial bytes to HEP Generic header
    CaptureHepGeneric hg;
    memcpy(&hg.header, data->data, sizeof(CaptureHepGeneric));

    // header HEP3 check
    if (memcmp(hg.header.id, "\x48\x45\x50\x33", 4) != 0)
        return NULL;

    // Packet frame to store timestamps
    PacketFrame *frame = g_list_nth_data(packet->frames, 0);

    // Limit the data to given length
    g_byte_array_set_size(data, g_ntohs(hg.header.length));

    // Remove already parsed ctrl header
    data = g_byte_array_remove_range(data, 0, sizeof(CaptureHepCtrl));

    while (data->len > 0) {

        CaptureHepChunk *chunk = (CaptureHepChunk *) data->data;
        guint chunk_vendor = g_ntohs(chunk->vendor_id);
        guint chunk_type = g_ntohs(chunk->type_id);
        guint chunk_len = g_ntohs(chunk->length);

        /* Bad length, drop packet */
        if (chunk_len == 0) {
            return NULL;
        }

        /* Skip not general chunks */
        if (chunk_vendor != 0) {
            data = g_byte_array_remove_range(data, 0, chunk_len);
            continue;
        }

        switch (chunk_type) {
            case CAPTURE_EEP_CHUNK_INVALID:
                return NULL;
            case CAPTURE_EEP_CHUNK_FAMILY:
                memcpy(&hg.ip_family, (gpointer) data->data, sizeof(CaptureHepChunkUint8));
                break;
            case CAPTURE_EEP_CHUNK_PROTO:
                memcpy(&hg.ip_proto, (gpointer) data->data, sizeof(CaptureHepChunkUint8));
                break;
            case CAPTURE_EEP_CHUNK_SRC_IP4:
                memcpy(&src_ip4, (gpointer) data->data, sizeof(CaptureHepChunkIp4));
                inet_ntop(AF_INET, &src_ip4.data, srcip, sizeof(srcip));
                break;
            case CAPTURE_EEP_CHUNK_DST_IP4:
                memcpy(&dst_ip4, (gpointer) data->data, sizeof(CaptureHepChunkIp4));
                inet_ntop(AF_INET, &dst_ip4.data, dstip, sizeof(srcip));
                break;
#ifdef USE_IPV6
            case CAPTURE_EEP_CHUNK_SRC_IP6:
                memcpy(&src_ip6, (gpointer) data->data, sizeof(CaptureHepChunkIp6));
                inet_ntop(AF_INET6, &src_ip6.data, srcip, sizeof(srcip));
                break;
            case CAPTURE_EEP_CHUNK_DST_IP6:
                memcpy(&dst_ip6, (gpointer) data->data, sizeof(CaptureHepChunkIp6));
                inet_ntop(AF_INET6, &dst_ip6.data, dstip, sizeof(dstip));
                break;
#endif
            case CAPTURE_EEP_CHUNK_SRC_PORT:
                memcpy(&hg.src_port, (gpointer) data->data, sizeof(CaptureHepChunkUint16));
                sport = g_ntohs(hg.src_port.data);
                break;
            case CAPTURE_EEP_CHUNK_DST_PORT:
                memcpy(&hg.dst_port, (gpointer) data->data, sizeof(CaptureHepChunkUint16));
                dport = g_ntohs(hg.dst_port.data);
                break;
            case CAPTURE_EEP_CHUNK_TS_SEC:
                memcpy(&hg.time_sec, (gpointer) data->data, sizeof(CaptureHepChunkUint32));
                break;
            case CAPTURE_EEP_CHUNK_TS_USEC:
                memcpy(&hg.time_usec, (gpointer) data->data, sizeof(CaptureHepChunkUint32));
                break;
            case CAPTURE_EEP_CHUNK_PROTO_TYPE:
                memcpy(&hg.proto_t, (gpointer) data->data, sizeof(CaptureHepChunkUint8));
                break;
            case CAPTURE_EEP_CHUNK_CAPT_ID:
                memcpy(&hg.capt_id, (gpointer) data->data, sizeof(CaptureHepChunkUint32));
                break;
            case CAPTURE_EEP_CHUNK_KEEP_TM:
                break;
            case CAPTURE_EEP_CHUNK_AUTH_KEY:
                memcpy(&authkey_chunk, (gpointer) data->data, sizeof(CaptureHepChunk));
                guint password_len = g_ntohs(authkey_chunk.length) - sizeof(CaptureHepChunk);
                data = g_byte_array_remove_range(data, 0, sizeof(CaptureHepChunk));
                password = g_strndup((gpointer) data->data, password_len);
                break;
            case CAPTURE_EEP_CHUNK_PAYLOAD:
                memcpy(&payload_chunk, (gpointer) data->data, sizeof(CaptureHepChunk));
                frame->len = frame->caplen = chunk_len - sizeof(CaptureHepChunk);
                data = g_byte_array_remove_range(data, 0, sizeof(CaptureHepChunk));
                payload = g_byte_array_sized_new(frame->len);
                g_byte_array_append(payload, data->data, frame->len);
                break;
            case CAPTURE_EEP_CHUNK_CORRELATION_ID:
                break;
            default:
                break;
        }

        // Fixup wrong chunk lengths
        if (chunk_len > data->len)
            chunk_len = data->len;

        // Parse next chunk
        data = g_byte_array_remove_range(data, 0, chunk_len);
    }

    // Validate password
    const gchar *hep_pass = setting_get_value(SETTING_HEP_LISTEN_PASS);
    if (hep_pass != NULL) {
        // No password in packet
        if (strlen(password) == 0)
            return NULL;
        // Check password matches configured
        if (strncmp(password, hep_pass, strlen(hep_pass)) != 0)
            return NULL;
    }

    // Generate Packet IP data
    PacketIpData *ip = g_malloc0(sizeof(PacketIpData));
    g_utf8_strncpy(ip->srcip, srcip, ADDRESSLEN);
    g_utf8_strncpy(ip->dstip, dstip, ADDRESSLEN);
    ip->protocol = hg.ip_proto.data;
    ip->version = (hg.ip_family.data == AF_INET) ? 4 : 6;
    packet_add_type(packet, PACKET_IP, ip);

    // Generate Packet UDP data
    PacketUdpData *udp = g_malloc0(sizeof(PacketHepData));
    udp->sport = sport;
    udp->dport = dport;
    packet_add_type(packet, PACKET_UDP, udp);

    // Generate Packet Timestamp
    frame->ts = g_ntohl(hg.time_sec.data) * G_USEC_PER_SEC + g_ntohl(hg.time_usec.data);

    // Parse SIP payload
    return packet_parser_next_dissector(parser, packet, payload);
}

PacketDissector *
packet_hep_new()
{
    PacketDissector *proto = g_malloc0(sizeof(PacketDissector));
    proto->id = PACKET_HEP;
    proto->dissect = packet_hep_parse;
    proto->subdissectors = g_slist_append(proto->subdissectors, GUINT_TO_POINTER(PACKET_SIP));
    return proto;
}
