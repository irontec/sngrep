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
#include "glib-extra/glib.h"
#include "packet_ip.h"
#include "packet_udp.h"
#include "packet.h"
#include "setting.h"
#include "packet_hep.h"

G_DEFINE_TYPE(PacketDissectorHep, packet_dissector_hep, PACKET_TYPE_DISSECTOR)

/**
 * @brief Received a HEP3 packet
 *
 * This function receives HEP protocol payload and converts it
 * to a Packet information. This code has been updated based on
 * Kamailio sipcapture module.
 *
 * @return packet pointer
 */
static GBytes *
packet_dissector_hep_dissect(PacketDissector *self, Packet *packet, GBytes *data)
{
    CaptureHepChunkIp4 src_ip4, dst_ip4;
#ifdef USE_IPV6
    CaptureHepChunkIp6 src_ip6, dst_ip6;
#endif
    CaptureHepChunk payload_chunk;
    CaptureHepChunk authkey_chunk;
    gchar srcip[ADDRESSLEN], dstip[ADDRESSLEN];
    guint16 sport = 0, dport = 0;
    g_autofree gchar *password = NULL;
    GBytes *payload = NULL;

    if (g_bytes_get_size(data) < sizeof(CaptureHepGeneric))
        return data;

    // Copy initial bytes to HEP Generic header
    CaptureHepGeneric hg;
    memcpy(&hg.header, g_bytes_get_data(data, NULL), sizeof(CaptureHepGeneric));

    // header HEP3 check
    if (memcmp(hg.header.id, "\x48\x45\x50\x33", 4) != 0)
        return data;

    // Packet frame to store timestamps
    PacketFrame *frame = g_list_nth_data(packet->frames, 0);

    // Limit the data to given length
    data = g_bytes_new_from_bytes(data, 0, g_ntohs(hg.header.length));

    // Remove already parsed ctrl header
    data = g_bytes_offset(data, sizeof(CaptureHepCtrl));

    while (g_bytes_get_size(data) > 0) {

        CaptureHepChunk *chunk = (CaptureHepChunk *) g_bytes_get_data(data, NULL);
        guint chunk_vendor = g_ntohs(chunk->vendor_id);
        guint chunk_type = g_ntohs(chunk->type_id);
        guint chunk_len = g_ntohs(chunk->length);

        /* Bad length, drop packet */
        if (chunk_len == 0) {
            return NULL;
        }

        /* Skip not general chunks */
        if (chunk_vendor != 0) {
            data = g_bytes_offset(data, chunk_len);
            continue;
        }

        switch (chunk_type) {
            case CAPTURE_EEP_CHUNK_INVALID:
                return NULL;
            case CAPTURE_EEP_CHUNK_FAMILY:
                memcpy(&hg.ip_family, g_bytes_get_data(data, NULL), sizeof(CaptureHepChunkUint8));
                break;
            case CAPTURE_EEP_CHUNK_PROTO:
                memcpy(&hg.ip_proto, g_bytes_get_data(data, NULL), sizeof(CaptureHepChunkUint8));
                break;
            case CAPTURE_EEP_CHUNK_SRC_IP4:
                memcpy(&src_ip4, g_bytes_get_data(data, NULL), sizeof(CaptureHepChunkIp4));
                inet_ntop(AF_INET, &src_ip4.data, srcip, sizeof(srcip));
                break;
            case CAPTURE_EEP_CHUNK_DST_IP4:
                memcpy(&dst_ip4, g_bytes_get_data(data, NULL), sizeof(CaptureHepChunkIp4));
                inet_ntop(AF_INET, &dst_ip4.data, dstip, sizeof(srcip));
                break;
#ifdef USE_IPV6
            case CAPTURE_EEP_CHUNK_SRC_IP6:
                memcpy(&src_ip6, g_bytes_get_data(data, NULL), sizeof(CaptureHepChunkIp6));
                inet_ntop(AF_INET6, &src_ip6.data, srcip, sizeof(srcip));
                break;
            case CAPTURE_EEP_CHUNK_DST_IP6:
                memcpy(&dst_ip6, g_bytes_get_data(data, NULL), sizeof(CaptureHepChunkIp6));
                inet_ntop(AF_INET6, &dst_ip6.data, dstip, sizeof(dstip));
                break;
#endif
            case CAPTURE_EEP_CHUNK_SRC_PORT:
                memcpy(&hg.src_port, g_bytes_get_data(data, NULL), sizeof(CaptureHepChunkUint16));
                sport = g_ntohs(hg.src_port.data);
                break;
            case CAPTURE_EEP_CHUNK_DST_PORT:
                memcpy(&hg.dst_port, g_bytes_get_data(data, NULL), sizeof(CaptureHepChunkUint16));
                dport = g_ntohs(hg.dst_port.data);
                break;
            case CAPTURE_EEP_CHUNK_TS_SEC:
                memcpy(&hg.time_sec, g_bytes_get_data(data, NULL), sizeof(CaptureHepChunkUint32));
                break;
            case CAPTURE_EEP_CHUNK_TS_USEC:
                memcpy(&hg.time_usec, g_bytes_get_data(data, NULL), sizeof(CaptureHepChunkUint32));
                break;
            case CAPTURE_EEP_CHUNK_PROTO_TYPE:
                memcpy(&hg.proto_t, g_bytes_get_data(data, NULL), sizeof(CaptureHepChunkUint8));
                break;
            case CAPTURE_EEP_CHUNK_CAPT_ID:
                memcpy(&hg.capt_id, g_bytes_get_data(data, NULL), sizeof(CaptureHepChunkUint32));
                break;
            case CAPTURE_EEP_CHUNK_KEEP_TM:
                break;
            case CAPTURE_EEP_CHUNK_AUTH_KEY:
                memcpy(&authkey_chunk, g_bytes_get_data(data, NULL), sizeof(CaptureHepChunk));
                guint password_len = g_ntohs(authkey_chunk.length) - sizeof(CaptureHepChunk);
                password = g_strndup(g_bytes_get_data(data, NULL), password_len);
                break;
            case CAPTURE_EEP_CHUNK_PAYLOAD:
                memcpy(&payload_chunk, g_bytes_get_data(data, NULL), sizeof(CaptureHepChunk));
                frame->len = frame->caplen = chunk_len - sizeof(CaptureHepChunk);
                data = g_bytes_offset(data, sizeof(CaptureHepChunk));
                payload = g_bytes_new(g_bytes_get_data(data, NULL), frame->len);
                break;
            case CAPTURE_EEP_CHUNK_CORRELATION_ID:
            default:
                break;
        }

        // Fixup wrong chunk lengths
        if (chunk_len > g_bytes_get_size(data))
            chunk_len = g_bytes_get_size(data);

        // Parse next chunk
        data = g_bytes_offset(data, chunk_len);
    }

    // Validate password
    const gchar *hep_pass = setting_get_value(SETTING_CAPTURE_HEP_LISTEN_PASS);
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
    ip->proto.id = PACKET_PROTO_IP;
    ip->srcip = g_strdup(srcip);
    ip->dstip = g_strdup(dstip);
    ip->protocol = hg.ip_proto.data;
    ip->version = (hg.ip_family.data == AF_INET) ? 4 : 6;
    packet_set_protocol_data(packet, PACKET_PROTO_IP, ip);

    // Generate Packet UDP data
    PacketUdpData *udp = g_malloc0(sizeof(PacketHepData));
    udp->proto.id = PACKET_PROTO_UDP;
    udp->sport = sport;
    udp->dport = dport;
    packet_set_protocol_data(packet, PACKET_PROTO_UDP, udp);

    // Generate Packet Timestamp
    frame->ts = g_ntohl(hg.time_sec.data) * G_USEC_PER_SEC + g_ntohl(hg.time_usec.data);

    // Parse SIP payload
    return packet_dissector_next(self, packet, payload);
}

static void
packet_dissector_hep_class_init(PacketDissectorHepClass *klass)
{
    PacketDissectorClass *dissector_class = PACKET_DISSECTOR_CLASS(klass);
    dissector_class->dissect = packet_dissector_hep_dissect;
}

static void
packet_dissector_hep_init(PacketDissectorHep *self)
{
    // HEP Dissector base information
    packet_dissector_add_subdissector(PACKET_DISSECTOR(self), PACKET_PROTO_SIP);
}

PacketDissector *
packet_dissector_hep_new()
{
    return g_object_new(
        PACKET_DISSECTOR_TYPE_HEP,
        "id", PACKET_PROTO_HEP,
        "name", "HEP",
        NULL
    );
}
