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
#include "packet/media.h"
#include "sip.h"

sip_msg_t *
msg_create()
{
    sip_msg_t *msg;
    if (!(msg = sng_malloc(sizeof(sip_msg_t))))
        return NULL;
    msg->medias = g_sequence_new(media_destroy);
    return msg;
}

void
msg_destroy(gpointer item)
{
    sip_msg_t *msg = item;

    // Free message SDP media
    if (!msg->retrans)
        g_sequence_free(msg->medias);

    // Free message packets
    packet_destroy(msg->packet);
    // Free all memory
    sng_free(msg->resp_str);
    sng_free(msg->sip_from);
    sng_free(msg->sip_to);
    sng_free(msg);
}

struct sip_call *
msg_get_call(const sip_msg_t *msg) {
    return msg->call;
}

int
msg_media_count(sip_msg_t *msg)
{
    return g_sequence_get_length(msg->medias);
}

int
msg_has_sdp(void *item)
{
    return msg_media_count(item);
}

int
msg_is_request(sip_msg_t *msg)
{
    return msg->reqresp < 100;
}

void
msg_add_media(sip_msg_t *msg, sdp_media_t *media)
{
    g_sequence_append(msg->medias, media);
}

const char *
msg_get_payload(sip_msg_t *msg)
{
    return (const char *) packet_payload(msg->packet);
}

struct timeval
msg_get_time(const sip_msg_t *msg) {
    struct timeval t = { };
    frame_t *frame;

    if (msg && (frame = g_sequence_first(msg->packet->frames)))
        return frame->header->ts;
    return t;
}

const char *
msg_get_attribute(sip_msg_t *msg, int id, char *value)
{
    char *ar;

    switch (id) {
        case SIP_ATTR_SRC:
            sprintf(value, "%s:%u", msg->packet->src.ip, msg->packet->src.port);
            break;
        case SIP_ATTR_DST:
            sprintf(value, "%s:%u", msg->packet->dst.ip, msg->packet->dst.port);
            break;
        case SIP_ATTR_METHOD:
            sprintf(value, "%.*s", SIP_ATTR_MAXLEN, sip_get_msg_reqresp_str(msg));
            break;
        case SIP_ATTR_SIPFROM:
            sprintf(value, "%.*s", SIP_ATTR_MAXLEN, msg->sip_from);
            break;
        case SIP_ATTR_SIPTO:
            sprintf(value, "%.*s", SIP_ATTR_MAXLEN, msg->sip_to);
            break;
        case SIP_ATTR_SIPFROMUSER:
            if ((ar = strchr(msg->sip_from, '@'))) {
                strncpy(value, msg->sip_from, ar - msg->sip_from);
            }
            break;
        case SIP_ATTR_SIPTOUSER:
            if ((ar = strchr(msg->sip_to, '@'))) {
                strncpy(value, msg->sip_to, ar - msg->sip_to);
            }
            break;
        case SIP_ATTR_DATE:
            timeval_to_date(msg_get_time(msg), value);
            break;
        case SIP_ATTR_TIME:
            timeval_to_time(msg_get_time(msg), value);
            break;
        default:
            fprintf(stderr, "Unhandled attribute %s (%d)\n", sip_attr_get_name(id), id); abort();
        break;
    }

    return strlen(value) ? value : NULL;

}

int
msg_is_older(sip_msg_t *one, sip_msg_t *two)
{
    // Yes, you are older than nothing
    if (!two)
        return 1;

    // No, you are not older than yourself
    if (one == two)
        return 0;

    // Otherwise
    return timeval_is_older(msg_get_time(one), msg_get_time(two));
}
