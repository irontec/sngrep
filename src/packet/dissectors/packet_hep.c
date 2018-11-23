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
 * @file packet_hep.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in packet_hep.h
 *
 * Support for HEP transport layer
 *
 */

#include "config.h"
#include <netinet/udp.h>
#include <arpa/inet.h>
#include "glib-utils.h"
#include "packet/dissectors/packet_ip.h"
#include "packet/dissectors/packet_udp.h"
#include "packet/packet.h"
#include "setting.h"
#include "packet_hep.h"

static GByteArray *
packet_hep_parse(PacketParser *parser, Packet *packet, GByteArray *data)
{
    CaptureHepChunkIp4 src_ip4, dst_ip4;
#ifdef USE_IPV6
    CaptureHepChunkIp6 src_ip6, dst_ip6;
#endif
    CaptureHepChunk payload_chunk;
    CaptureHepChunk authkey_chunk;
    CaptureHepChunk uuid_chunk;
    //! Source and Destination Address
    Address src, dst;

    /* Copy initial bytes to HEP Generic header */
    struct _CaptureHepGeneric hg;
    memcpy(&hg, data->data, sizeof(CaptureHepGeneric));

    /* header check */
    if (memcmp(hg.header.id, "\x48\x45\x50\x33", 4) != 0)
        return NULL;

    /* IP dissectors */
    guint8 family = hg.ip_family.data;
    /* Proto ID */
    guint8 proto = hg.ip_proto.data;

    data = g_byte_array_remove_range(data, 0, sizeof(CaptureHepGeneric));

    /* IPv4 */
    if (family == AF_INET) {
        /* SRC IP */
        memcpy(&src_ip4, data->data, sizeof(struct _CaptureHepChunkIp4));
        inet_ntop(AF_INET, &src_ip4.data, src.ip, sizeof(src.ip));
        data = g_byte_array_remove_range(data, 0, sizeof(CaptureHepChunkIp4));

        /* DST IP */
        memcpy(&dst_ip4, data->data, sizeof(struct _CaptureHepChunkIp4));
        inet_ntop(AF_INET, &dst_ip4.data, dst.ip, sizeof(src.ip));
        data = g_byte_array_remove_range(data, 0, sizeof(CaptureHepChunkIp4));
    }
#ifdef USE_IPV6
        /* IPv6 */
    else if(family == AF_INET6) {
        /* SRC IPv6 */
        memcpy(&src_ip6, data->data, sizeof(struct _CaptureHepChunkIp6));
        inet_ntop(AF_INET6, &src_ip6.data, src.ip, sizeof(src.ip));
        data = g_byte_array_remove_range(data, 0, sizeof(CaptureHepChunkIp6));

        /* DST IP */
        memcpy(&src_ip6, data->data, sizeof(struct _CaptureHepChunkIp6));
        inet_ntop(AF_INET6, &dst_ip6.data, dst.ip, sizeof(dst.ip));
        data = g_byte_array_remove_range(data, 0, sizeof(CaptureHepChunkIp6));
    }
#endif

    /* SRC PORT */
    src.port = ntohs(hg.src_port.data);
    /* DST PORT */
    dst.port = ntohs(hg.dst_port.data);

    /* TIMESTAMP*/
    PacketFrame *frame = g_list_nth_data(packet->frames, 0);
    frame->ts.tv_sec =  ntohl(hg.time_sec.data);
    frame->ts.tv_usec = ntohl(hg.time_usec.data);

    /* Protocol TYPE */
    /* Capture ID */

    /* auth key */
    const gchar *hep_pass = setting_get_value(SETTING_HEP_LISTEN_PASS);
    if (hep_pass != NULL) {
        memcpy(&authkey_chunk, data->data, sizeof(authkey_chunk));
        data = g_byte_array_remove_range(data, 0, sizeof(CaptureHepChunk));


        gchar password[100];
        guint password_len = ntohs(authkey_chunk.length) - sizeof(authkey_chunk);
        memcpy(password, data->data, password_len);
        data = g_byte_array_remove_range(data, 0, password_len);

        // Validate the password
        if (strncmp(password, hep_pass, password_len) != 0)
            return NULL;
    }

    if (setting_enabled(SETTING_HEP_LISTEN_UUID)) {
        memcpy(&uuid_chunk, data->data, sizeof(uuid_chunk));
        data = g_byte_array_remove_range(data, 0, sizeof(CaptureHepChunk));


        guint16 uuid_len = ntohs(uuid_chunk.length) - sizeof(uuid_chunk);
        data = g_byte_array_remove_range(data, 0, uuid_len);

    }

    /* Payload */
    memcpy(&payload_chunk, data->data, sizeof(payload_chunk));
    data = g_byte_array_remove_range(data, 0, sizeof(CaptureHepChunk));

    // Generate Packet IP data
    PacketIpData *ip = g_malloc0(sizeof(PacketIpData));
    ip->saddr = src;
    ip->daddr = dst;
    ip->protocol = proto;
    ip->version = (family == AF_INET) ? 4 : 6;
    g_ptr_array_set(packet->proto, PACKET_IP, ip);

    // Generate Packet UDP data
    PacketUdpData *udp = g_malloc0(sizeof(PacketHepData));
    udp->sport = src.port;
    udp->dport = dst.port;
    g_ptr_array_set(packet->proto, PACKET_UDP, udp);

    // Parse SIP payload
    return packet_parser_next_dissector(parser, packet, data);
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
