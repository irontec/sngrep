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
 * @file sip_msg.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage SIP call data
 *
 * This file contains the functions and structure to manage SIP message data
 *
 */
#include "config.h"
#include <glib.h>
#include "glib-utils.h"
#include "sip_msg.h"
#include "packet/dissectors/packet_sip.h"
#include "packet/dissectors/packet_sdp.h"
#include "storage.h"

SipMsg *
msg_create()
{
    SipMsg *msg = g_malloc0(sizeof(SipMsg));
    msg->medias = g_sequence_new(NULL);
    return msg;
}

void
msg_destroy(gpointer item)
{
    SipMsg *msg = item;

    // Free message SDP media
    if (!msg->retrans)
        g_sequence_free(msg->medias);

    // Free message packets
    packet_free(msg->packet);
    // Free all memory
    g_free(msg);
}

SipCall *
msg_get_call(const SipMsg *msg) {
    return msg->call;
}

guint
msg_media_count(SipMsg *msg)
{
    return g_sequence_get_length(msg->medias);
}

gboolean
msg_has_sdp(void *item)
{
    return msg_media_count(item) > 0;
}

gboolean
msg_is_request(SipMsg *msg)
{
    return packet_sip_method(msg->packet) < 100;
}

const gchar *
msg_get_payload(SipMsg *msg)
{
    return packet_sip_payload(msg->packet);
}

GTimeVal
msg_get_time(const SipMsg *msg) {
    GTimeVal t = { };
    PacketFrame *frame;

    if (msg && (frame = g_list_nth_data(msg->packet->frames, 0)))
        return frame->ts;
    return t;
}

const gchar *
msg_get_attribute(SipMsg *msg, gint id, char *value)
{
    char *ar;

    switch (id) {
        case SIP_ATTR_SRC:
            sprintf(value, "%s:%u", msg_src_address(msg).ip, msg_src_address(msg).port);
            break;
        case SIP_ATTR_DST:
            sprintf(value, "%s:%u", msg_dst_address(msg).ip, msg_dst_address(msg).port);
            break;
        case SIP_ATTR_METHOD:
            sprintf(value, "%.*s", SIP_ATTR_MAXLEN, packet_sip_method_str(msg->packet));
            break;
        case SIP_ATTR_SIPFROM:
            sprintf(value, "%.*s", SIP_ATTR_MAXLEN, packet_sip_header(msg->packet, SIP_HEADER_FROM));
            break;
        case SIP_ATTR_SIPTO:
            sprintf(value, "%.*s", SIP_ATTR_MAXLEN, packet_sip_header(msg->packet, SIP_HEADER_TO));
            break;
        case SIP_ATTR_SIPFROMUSER:
            sprintf(value, "%.*s", SIP_ATTR_MAXLEN, packet_sip_header(msg->packet, SIP_HEADER_FROM));
            if ((ar = strchr(value, '@')))
                *ar = '\0';
            break;
        case SIP_ATTR_SIPTOUSER:
            sprintf(value, "%.*s", SIP_ATTR_MAXLEN, packet_sip_header(msg->packet, SIP_HEADER_TO));
            if ((ar = strchr(value, '@')))
                *ar = '\0';
            break;
        case SIP_ATTR_DATE:
            timeval_to_date(msg_get_time(msg), value);
            break;
        case SIP_ATTR_TIME:
            timeval_to_time(msg_get_time(msg), value);
            break;
        default:
            fprintf(stderr, "Unhandled attribute %s (%d)\n", sip_attr_get_name(id), id);
            break;
    }

    return strlen(value) ? value : NULL;

}

const gchar *
msg_get_preferred_codec_alias(SipMsg *msg)
{
    PacketSdpMedia *media = g_sequence_first(msg->medias);
    g_return_val_if_fail(media != NULL, NULL);

    PacketSdpFormat *format = g_list_nth_data(media->formats, 0);
    g_return_val_if_fail(format != NULL, NULL);

    return format->alias;
}

const gchar *
msg_get_header(SipMsg *msg, char *out)
{
    char from_addr[80], to_addr[80], time[80], date[80];

    // Source and Destination address
    msg_get_attribute(msg, SIP_ATTR_DATE, date);
    msg_get_attribute(msg, SIP_ATTR_TIME, time);
    msg_get_attribute(msg, SIP_ATTR_SRC, from_addr);
    msg_get_attribute(msg, SIP_ATTR_DST, to_addr);

    // Get msg header
    sprintf(out, "%s %s %s -> %s", date, time, from_addr, to_addr);
    return out;
}

const SipMsg *
msg_is_retrans(SipMsg *msg)
{
    SipMsg *prev = NULL;
    GSequenceIter *it;

    // Get previous message in call with same origin and destination
    it = g_sequence_get_end_iter(msg->call->msgs);

    // Skip already added message
    it = g_sequence_iter_prev(it);

    while(!g_sequence_iter_is_begin(it)) {
        it = g_sequence_iter_prev(it);
        prev = g_sequence_get(it);
        // Same addresses
        if (addressport_equals(msg_src_address(prev), msg_src_address(msg)) &&
            addressport_equals(msg_dst_address(prev), msg_dst_address(msg))) {
            // Same payload
            if (!strcasecmp(msg_get_payload(msg), msg_get_payload(prev))) {
                // Store the flag that determines if message is retrans
                msg->retrans = prev;
                break;
            }
        }
    }

}