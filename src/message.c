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
#include "message.h"
#include "packet/dissectors/packet_sip.h"
#include "packet/dissectors/packet_sdp.h"
#include "storage.h"

Message *
msg_create()
{
    Message *msg = g_malloc0(sizeof(Message));
    msg->medias = NULL;
    return msg;
}

void
msg_destroy(gpointer item)
{
    Message *msg = item;

    // Free message SDP media
    if (!msg->retrans)
        g_list_free(msg->medias);

    // Free message packets
    packet_free(msg->packet);
    // Free all memory
    g_free(msg);
}

Call *
msg_get_call(const Message *msg) {
    return msg->call;
}

guint
msg_media_count(Message *msg)
{
    return g_list_length(msg->medias);
}

PacketSdpMedia *
msg_media_for_addr(Message *msg, Address dst)
{
    for (GList *l = msg->medias; l != NULL; l = l->next) {
        PacketSdpMedia *media = l->data;
        if (addressport_equals(media->address, dst)) {
            return media;
        }
    }

    return NULL;
}

gboolean
msg_has_sdp(void *item)
{
    return msg_media_count(item) > 0;
}

gboolean
msg_is_request(Message *msg)
{
    return packet_sip_method(msg->packet) < 100;
}

gboolean
msg_is_initial_transaction(Message *msg)
{
    return packet_sip_to_tag(msg->packet) == NULL;
}

const gchar *
msg_get_payload(Message *msg)
{
    return packet_sip_payload(msg->packet);
}

GTimeVal
msg_get_time(const Message *msg) {
    GTimeVal t = { 0 };
    PacketFrame *frame;

    if (msg && (frame = g_list_nth_data(msg->packet->frames, 0)))
        return frame->ts;
    return t;
}

const gchar *
msg_get_attribute(Message *msg, gint id, char *value)
{
    char *ar;

    switch (id) {
        case ATTR_SRC:
            sprintf(value, "%s:%u", msg_src_address(msg).ip, msg_src_address(msg).port);
            break;
        case ATTR_DST:
            sprintf(value, "%s:%u", msg_dst_address(msg).ip, msg_dst_address(msg).port);
            break;
        case ATTR_METHOD:
            sprintf(value, "%.*s", ATTR_MAXLEN, packet_sip_method_str(msg->packet));
            break;
        case ATTR_SIPFROM:
            sprintf(value, "%.*s", ATTR_MAXLEN, packet_sip_header(msg->packet, SIP_HEADER_FROM));
            break;
        case ATTR_SIPTO:
            sprintf(value, "%.*s", ATTR_MAXLEN, packet_sip_header(msg->packet, SIP_HEADER_TO));
            break;
        case ATTR_SIPFROMUSER:
            sprintf(value, "%.*s", ATTR_MAXLEN, packet_sip_header(msg->packet, SIP_HEADER_FROM));
            if ((ar = strchr(value, '@')))
                *ar = '\0';
            break;
        case ATTR_SIPTOUSER:
            sprintf(value, "%.*s", ATTR_MAXLEN, packet_sip_header(msg->packet, SIP_HEADER_TO));
            if ((ar = strchr(value, '@')))
                *ar = '\0';
            break;
        case ATTR_DATE:
            timeval_to_date(msg_get_time(msg), value);
            break;
        case ATTR_TIME:
            timeval_to_time(msg_get_time(msg), value);
            break;
        default:
            fprintf(stderr, "Unhandled attribute %s (%d)\n", attr_name(id), id);
            break;
    }

    return strlen(value) ? value : NULL;

}

const gchar *
msg_get_preferred_codec_alias(Message *msg)
{
    PacketSdpMedia *media = g_list_nth_data(msg->medias, 0);
    g_return_val_if_fail(media != NULL, NULL);

    PacketSdpFormat *format = g_list_nth_data(media->formats, 0);
    g_return_val_if_fail(format != NULL, NULL);

    return format->alias;
}

const gchar *
msg_get_header(Message *msg, char *out)
{
    char from_addr[80], to_addr[80], time[80], date[80];

    // Source and Destination address
    msg_get_attribute(msg, ATTR_DATE, date);
    msg_get_attribute(msg, ATTR_TIME, time);
    msg_get_attribute(msg, ATTR_SRC, from_addr);
    msg_get_attribute(msg, ATTR_DST, to_addr);

    // Get msg header
    sprintf(out, "%s %s %s -> %s", date, time, from_addr, to_addr);
    return out;
}

const Message *
msg_is_retrans(Message *msg)
{
    for (gint i = g_ptr_array_len(msg->call->msgs) - 1; i >= 0; i--) {
        Message *prev = g_ptr_array_index(msg->call->msgs, i);

        // Skip yourself
        if (prev == msg) continue;

        // Check source and destination addresses are equal
        if (addressport_equals(msg_src_address(prev), msg_src_address(msg)) &&
            addressport_equals(msg_dst_address(prev), msg_dst_address(msg))) {
            // Check they have the same payload
            if (!strcasecmp(msg_get_payload(msg), msg_get_payload(prev))) {
                return prev;
            }
        }
    }

    return NULL;
}
