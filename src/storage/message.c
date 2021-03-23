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
#include "glib-extra/glib.h"
#include "message.h"
#include "packet/packet_sip.h"
#include "packet/packet_mrcp.h"
#include "packet/packet_sdp.h"
#include "storage/storage.h"

Message *
msg_new(Packet *packet)
{
    Message *msg = g_malloc0(sizeof(Message));
    // Set message packet
    msg->packet = packet_ref(packet);
    // Create message attribute list
    msg->attributes = NULL;
    // Mark retransmission flag as not checked
    msg->retrans = -1;

    // Message from SIP packet
    if (packet_has_protocol(packet, PACKET_PROTO_SIP)) {
        // Fill SIP protocol information
        msg->initial = packet_sip_initial_transaction(packet);
        msg->isrequest = packet_sip_is_request(packet);
        msg->method = packet_sip_method(packet);
        msg->method_str =  packet_sip_method_str(packet);
        msg->cseq = packet_sip_cseq(packet);
        msg->payload = packet_sip_payload_str(packet);
        msg->auth = packet_sip_auth_data(msg->packet);
    }

    // Message from MRCP arrow
    if (packet_has_protocol(packet, PACKET_PROTO_MRCP)) {
        // Fill MRCP protocol information
        msg->isrequest = packet_mrcp_is_request(packet);
        msg->method = packet_mrcp_method(packet);
        msg->method_str = packet_mrcp_method_str(packet);
        msg->payload = packet_mrcp_payload_str(packet);
        msg->cseq = packet_mrcp_request_id(packet);
    }

    return msg;
}

void
msg_free(Message *msg)
{
    // Free message packets
    packet_unref(msg->packet);
    g_slist_free_full(msg->attributes, (GDestroyNotify) attribute_value_free);
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

    if (sdp == NULL)
        return 0;

    return g_list_length(sdp->medias);
}

PacketSdpMedia *
msg_media_for_addr(Message *msg, Address dst)
{
    PacketSdpData *sdp = packet_sdp_data(msg->packet);
    g_return_val_if_fail(sdp != NULL, NULL);

    for (GList *l = sdp->medias; l != NULL; l = l->next) {
        PacketSdpMedia *media = l->data;
        if (addressport_equals(media->address, dst)) {
            return media;
        }
        if (address_equals(dst, msg_src_address(msg)) && address_get_port(dst) == address_get_port(media->address)) {
            return media;
        }
    }

    return NULL;
}

gboolean
msg_is_initial_transaction(Message *msg)
{
    return msg->initial;
}

gboolean
msg_has_sdp(void *item)
{
    return msg_media_count(item) > 0;
}

Address
msg_src_address(Message *msg)
{
    return packet_src_address(msg->packet);
}

Address
msg_dst_address(Message *msg)
{
    return packet_dst_address(msg->packet);
}

gboolean
msg_is_request(Message *msg)
{
    return msg->isrequest;
}

guint
msg_get_method(Message *msg)
{
    return msg->method;
}

const gchar *
msg_get_method_str(Message *msg)
{
    return msg->method_str;
}

guint64
msg_get_cseq(Message *msg)
{
    return msg->cseq;
}

gchar *
msg_get_payload(Message *msg)
{
    return g_strdup(msg->payload);
}

guint64
msg_get_time(const Message *msg)
{
    if (msg == NULL)
        return 0;
    return packet_time(msg->packet);
}

const gchar *
msg_get_attribute(Message *msg, Attribute *attr)
{
    AttributeValue *cached_value = NULL;

    //! Check if this attribute was already requested
    for (GSList *l = msg->attributes; l != NULL; l = l->next) {
        AttributeValue *value = l->data;
        g_return_val_if_fail(value != NULL, NULL);
        if (value->attr == attr) {
            cached_value = value;
            break;
        }
    }

    if (cached_value != NULL) {
        if (!attr->mutable) {
            //! Attribute value doesn't change, return cached value
            return cached_value->value;
        } else {
            //! Attribute value changes, cached value is obsolete
            msg->attributes = g_slist_remove(msg->attributes, cached_value);
            attribute_value_free(cached_value);
        }
    }

    //! Get current attribute value
    cached_value = attribute_value_new(
        attr,
        attribute_get_value(attr, msg)
    );

    //! Store value in attribute cache for future requests
    msg->attributes = g_slist_append(msg->attributes, cached_value);

    return cached_value->value;
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
            msg_get_attribute(msg, attribute_find_by_name(ATTR_DATE)),
            msg_get_attribute(msg, attribute_find_by_name(ATTR_TIME)),
            msg_get_attribute(msg, attribute_find_by_name(ATTR_SRC)),
            msg_get_attribute(msg, attribute_find_by_name(ATTR_DST))
    );
    return out;
}

gboolean
msg_is_retransmission(Message *msg)
{
    if (msg->retrans == -1) {
        // Try to find the original message
        const Message *original = msg_get_retransmission_original(msg);
        msg->retrans = (original != NULL) ? 1 : 0;
    }

    return (msg->retrans == 0) ? FALSE : TRUE;
}

const Message *
msg_get_retransmission_original(Message *msg)
{
    // Look in the 20 previous messages for a retrans of current message
    guint length = g_ptr_array_len(msg->call->msgs);
    for (guint i = 0; i < 20; i++) {
        // Stop if we have checked all items
        if (i >= length)
            break;

        // Get the Nth item from the tail
        Message *prev = g_ptr_array_index(msg->call->msgs, length - 1 - i);

        // Skip yourself
        if (prev == msg) continue;

        // Check source and destination addresses are equal
        if (addressport_equals(msg_src_address(prev), msg_src_address(msg)) &&
            addressport_equals(msg_dst_address(prev), msg_dst_address(msg))) {
            // Check they have the same payload
            gchar *payload_msg = msg_get_payload(msg);
            gchar *payload_prev = msg_get_payload(prev);
            if (!strcasecmp(payload_msg, payload_prev)) {
                g_free(payload_msg);
                g_free(payload_prev);
                return prev;
            }
            g_free(payload_msg);
            g_free(payload_prev);
        }
    }

    return NULL;
}

gboolean
msg_is_duplicate(Message *msg)
{
    if (!msg_is_retransmission(msg))
        return FALSE;

    g_autoptr(GDateTime) orig_ts = g_date_time_new_from_unix_usec(
        msg_get_time(msg_get_retransmission_original(msg))
    );

    g_autoptr(GDateTime) retrans_ts = g_date_time_new_from_unix_usec(
        msg_get_time(msg)
    );

    // Consider duplicate if difference with its original is 10ms or less
    return g_date_time_difference(retrans_ts, orig_ts) > 10000;
}

const gchar *
msg_get_auth_hdr(const Message *msg)
{
    return msg->auth;
}
