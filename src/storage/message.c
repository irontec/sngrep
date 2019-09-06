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
 * @file message.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage SIP call data
 *
 * This file contains the functions and structure to manage SIP message data
 *
 */
#include "config.h"
#include <glib.h>
#include "glib/glib-extra.h"
#include "message.h"
#include "capture/dissectors/packet_sip.h"
#include "capture/dissectors/packet_sdp.h"
#include "storage/storage.h"

Message *
msg_new(Packet *packet)
{
    Message *msg = g_malloc0(sizeof(Message));
    // Set message packet
    msg->packet = packet_ref(packet);
    // Create message attribute hash table
    msg->attributes = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    // Set SIP packet related data
    msg->ts = packet_time(packet);
    msg->src = address_clone(packet_src_address(packet));
    msg->dst = address_clone(packet_dst_address(packet));
    msg->is_request = packet_sip_method(packet) < 100;
    msg->cseq = packet_sip_cseq(packet);

    if (msg->is_request) {
        msg->request.id = packet_sip_method(packet);
        msg->request.method = g_strdup(packet_sip_method_str(packet));
        msg->request.auth = g_strdup(packet_sip_auth_data(packet));
        msg->request.is_initial = packet_sip_initial_transaction(packet);
    } else {
        msg->response.code = packet_sip_method(packet);
        msg->response.reason = g_strdup(packet_sip_method_str(packet));
    }

    return msg;
}

void
msg_free(Message *msg)
{
    // Free message packets
    packet_unref(msg->packet);
    g_hash_table_destroy(msg->attributes);
    g_free(msg);
}

Call *
msg_get_call(const Message *msg)
{
    return msg->call;
}

guint
msg_media_count(Message *msg)
{
    PacketSdpData *sdp = packet_sdp_data(msg->packet);
    g_return_val_if_fail(sdp != NULL, 0);
    return g_list_length(sdp->medias);
}

PacketSdpMedia *
msg_media_for_addr(Message *msg, Address *dst)
{
    PacketSdpData *sdp = packet_sdp_data(msg->packet);
    g_return_val_if_fail(sdp != NULL, NULL);

    for (GList *l = sdp->medias; l != NULL; l = l->next) {
        PacketSdpMedia *media = l->data;
        if (addressport_equals(media->address, dst)) {
            return media;
        }
    }

    return NULL;
}

gboolean
msg_is_initial_transaction(Message *msg)
{
    return msg->request.is_initial;
}

gboolean
msg_has_sdp(void *item)
{
    return msg_media_count(item) > 0;
}

const Address *
msg_src_address(Message *msg)
{
    return msg->src;
}

const Address *
msg_dst_address(Message *msg)
{
    return msg->dst;
}

gboolean
msg_is_request(Message *msg)
{
    return msg->is_request;
}

guint64
msg_get_cseq(Message *msg)
{
    return msg->cseq;
}

const gchar *
msg_get_payload(Message *msg)
{
    return packet_sip_payload(msg->packet);
}

GTimeVal
msg_get_time(const Message *msg)
{
    GTimeVal t = {0};
    PacketFrame *frame;

    if (msg && (frame = g_list_nth_data(msg->packet->frames, 0)))
        return frame->ts;
    return t;
}

const gchar *
msg_get_attribute(Message *msg, gint id)
{
    Attribute *attr = attr_header(id);
    return attr_get_value(attr->name, msg);
}

const gchar *
msg_get_preferred_codec_alias(Message *msg)
{
    PacketSdpData *sdp = packet_sdp_data(msg->packet);
    g_return_val_if_fail(sdp != NULL, NULL);

    PacketSdpMedia *media = g_list_nth_data(sdp->medias, 0);
    g_return_val_if_fail(media != NULL, NULL);

    PacketSdpFormat *format = g_list_nth_data(media->formats, 0);
    g_return_val_if_fail(format != NULL, NULL);

    return format->alias;
}

const gchar *
msg_get_header(Message *msg, char *out)
{
    // Get msg header
    sprintf(out, "%s %s %s -> %s",
            msg_get_attribute(msg, ATTR_DATE),
            msg_get_attribute(msg, ATTR_TIME),
            msg_get_attribute(msg, ATTR_SRC),
            msg_get_attribute(msg, ATTR_DST)
    );
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

void
msg_set_cached_attribute(Message *msg, Attribute *attr, gchar *value)
{
    gchar *prev_value = g_hash_table_lookup(msg->attributes, attr->name);
    if (prev_value != NULL && g_strcmp0(prev_value, value) != 0) {
        g_free(prev_value);
        g_hash_table_remove(msg->attributes, attr->name);
        g_hash_table_insert(msg->attributes, g_strdup(attr->name), value);
    }
}

gchar *
msg_get_cached_attribute(Message *msg, Attribute *attr)
{
    return g_hash_table_lookup(msg->attributes, attr->name);
}
