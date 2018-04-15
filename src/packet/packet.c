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
 * @file packet.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in packet.h
 *
 */
#include "config.h"
#include <glib.h>
#include "capture/capture_pcap.h"
#include "packet.h"

Packet *
packet_new()
{
    // Create a new packet
    Packet *packet = g_malloc0(sizeof(Packet));
    packet->proto = g_ptr_array_sized_new(PACKET_PROTO_COUNT);
    g_ptr_array_set_size(packet->proto, PACKET_PROTO_COUNT);
    return packet;
}


#if 0

Packet *
packet_create(Address src, Address dst)
{
    // Create a new packet
    Packet *packet;
    packet = g_malloc0(sizeof(Packet));
    packet->src = src;
    packet->dst = dst;
    return packet;
}

void
packet_destroy(Packet *packet)
{
    // Check we have a valid packet pointer
    if (!packet) return;

    // Destroy frames
    GList *l;
    for (l = packet->frames; l != NULL; l = l->next) {
        PacketFrame *frame = l->data;
        g_free(frame->header);
        g_free(frame->data);
    }

    g_list_free(packet->frames);
    g_free(packet);
}

PacketFrame *
packet_frame_create(const struct pcap_pkthdr *header, const guchar *data)
{
    PacketFrame *frame = g_malloc(sizeof(PacketFrame));
    frame->header = g_memdup(header, sizeof(struct pcap_pkthdr));
    frame->data = g_memdup(data, header->caplen);
    return frame;
}

void
packet_add_frame(Packet *pkt, PacketFrame *frame)
{
    pkt->frames = g_list_append(pkt->frames, frame);
}

struct timeval
packet_time(Packet *packet)
{
    GList *first;
    struct timeval ts = { 0 };

    // Return first frame timestamp
    if (packet && (first = g_list_first(packet->frames))) {
        PacketFrame *frame = first->data;
        ts.tv_sec = frame->header->ts.tv_sec;
        ts.tv_usec = frame->header->ts.tv_usec;
    }

    // Return packe timestamp
    return ts;
}
#endif

void
packet_take_frames(Packet *dst, Packet *src)
{
//    for (GList *l = src->frames; l != NULL; l = l->next) {
//        dst->frames = g_list_append(dst->frames, l->data);
//    }
}

gboolean
packet_has_type(Packet *packet, enum packet_proto type)
{
//    return (packet->types & ( 1 << type))  != 0;
    return g_ptr_array_index(packet->proto, type) != NULL;
}

const char *
packet_transport(Packet *packet)
{
    if (packet_has_type(packet, PACKET_UDP))
        return "UDP";

    if (packet_has_type(packet, PACKET_TCP)) {
        if (packet_has_type(packet, PACKET_WS)) {
            return (packet_has_type(packet, PACKET_TLS)) ? "WSS" : "WS";
        }
        return packet_has_type(packet, PACKET_TLS) ? "TLS" : "TCP";
    }
    return "???";
}

