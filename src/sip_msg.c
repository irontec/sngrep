/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2015 Ivan Alonso (Kaian)
 ** Copyright (C) 2015 Irontec SL. All rights reserved.
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
#include "sip_msg.h"

sip_msg_t *
msg_create(const char *payload)
{
    sip_msg_t *msg;
    if (!(msg = malloc(sizeof(sip_msg_t))))
        return NULL;
    memset(msg, 0, sizeof(sip_msg_t));

    // Create a vector to store attributes
    msg->attrs = vector_create(4, 4);
    vector_set_destroyer(msg->attrs, sip_attr_destroyer);
    // Create a vector to store packets
    msg->packets = vector_create(1, 1);
    vector_set_destroyer(msg->packets, capture_packet_destroyer);
    return msg;
}

void
msg_destroy(sip_msg_t *msg)
{
    // Free message attribute list
    vector_destroy(msg->attrs);
    // Free message SDP media
    vector_destroy(msg->medias);
    // Free message packets
    vector_destroy(msg->packets);
    // Free payload if parsed
    if (msg->payload)
        free(msg->payload);
    // Free all memory
    free(msg);
}

void
msg_destroyer(void *msg)
{
    msg_destroy((sip_msg_t *)msg);
}

struct sip_call *
msg_get_call(const sip_msg_t *msg)
{
    return msg->call;
}

int
msg_media_count(sip_msg_t *msg)
{
    return vector_count(msg->medias);
}

int
msg_has_sdp(void *item)
{
    // TODO
    sip_msg_t *msg = (sip_msg_t *)item;
    return vector_count(msg->medias) ? 1 : 0;
}

int
msg_is_request(sip_msg_t *msg)
{
    return msg->reqresp < SIP_METHOD_SENTINEL;
}

void
msg_add_packet(sip_msg_t *msg, capture_packet_t *packet)
{
    vector_append(msg->packets, packet);
}

void
msg_add_media(sip_msg_t *msg, sdp_media_t *media)
{
    if (!msg->medias) {
        // Create a vector to store sdp
        msg->medias = vector_create(2, 2);
        vector_set_destroyer(msg->medias, media_destroyer);
    }
    vector_append(msg->medias, media);
}

const char *
msg_get_payload(sip_msg_t *msg)
{
    // Return Message payload pointer
    if (msg->payload)
        return (const char *)msg->payload;

    // Calculate message payload pointer
    // TODO Multi packet support
    capture_packet_t *packet = vector_first(msg->packets);
    // Get payload from packet data
    msg->payload = malloc(packet->payload_len + 1);
    memset(msg->payload, 0, packet->payload_len + 1);
    if (packet->payload) {
        memcpy(msg->payload, packet->payload, packet->payload_len);
    } else {
        memcpy(msg->payload, packet->data + (packet->size - packet->payload_len), packet->payload_len);
    }
    return (const char *) msg->payload;
}

struct timeval
msg_get_time(sip_msg_t *msg)
{
    struct timeval t = { };
    capture_packet_t *packet;

    if (msg && (packet = vector_first(msg->packets)))
        return packet->header->ts;
    return t;
}

void
msg_set_attribute(sip_msg_t *msg, enum sip_attr_id id, const char *fmt, ...)
{
    char value[512];

    // Get the actual value for the attribute
    va_list ap;
    va_start(ap, fmt);
    vsprintf(value, fmt, ap);
    va_end(ap);

    sip_attr_set(msg->attrs, id, value);
}

const char *
msg_get_attribute(sip_msg_t *msg, enum sip_attr_id id)
{
    return sip_attr_get_value(msg->attrs, id);
}
