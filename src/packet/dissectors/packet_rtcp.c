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
 * @file packet_rtcp.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in packet_rtcp.h
 */

#include "config.h"
#include <glib.h>
#include "storage.h"
#include "packet/packet.h"
#include "packet/dissector.h"
#include "packet/old_packet.h"
#include "packet/dissectors/packet_ip.h"
#include "packet/dissectors/packet_udp.h"
#include "packet_rtp.h"

static GByteArray *
packet_rtcp_parse(PacketParser *parser G_GNUC_UNUSED, Packet *packet, GByteArray *data)
{
    packet_t *oldpkt = g_malloc0(sizeof(packet_t));

    PacketIpData *ipdata = g_ptr_array_index(packet->proto, PACKET_IP);
    g_return_val_if_fail(ipdata != NULL, NULL);
    oldpkt->src = ipdata->saddr;
    oldpkt->dst = ipdata->daddr;

    PacketUdpData *udpdata = g_ptr_array_index(packet->proto, PACKET_UDP);
    g_return_val_if_fail(udpdata != NULL, NULL);
    oldpkt->newpacket = packet;
    oldpkt->src.port = udpdata->sport;
    oldpkt->dst.port = udpdata->dport;

    packet_set_payload(oldpkt, data->data, data->len);

    oldpkt->frames = g_sequence_new(NULL);
    for (GList *l = packet->frames; l != NULL; l = l->next) {
        PacketFrame *frame = l->data;
        packet_add_frame(oldpkt, frame->header, frame->data);
    }

    storage_check_rtp_packet(oldpkt);
    return NULL;
}

PacketDissector *
packet_rtcp_new()
{
    PacketDissector *proto = g_malloc0(sizeof(PacketDissector));
    proto->id = PACKET_RTCP;
    proto->dissect = packet_rtcp_parse;

    return proto;
}