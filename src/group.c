/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2016 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2016 Irontec SL. All rights reserved.
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
 * @file group.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source code of functions defined in group.h
 *
 */
#include <string.h>
#include <stdlib.h>
#include "group.h"

sip_call_group_t *
call_group_create()
{
    sip_call_group_t *group;
    if (!(group = sng_malloc(sizeof(sip_call_group_t)))) {
        return NULL;
    }
    group->calls = vector_create(5, 2);
    return group;
}

void
call_group_destroy(sip_call_group_t *group)
{
    vector_destroy(group->calls);
    sng_free(group);
}

sip_call_group_t *
call_group_clone(sip_call_group_t *original)
{
    sip_call_group_t *clone;

    if (!original)
        return NULL;

    if (!(clone = sng_malloc(sizeof(sip_call_group_t)))) {
        return NULL;
    }

    clone->calls = vector_clone(original->calls);
    return clone;
}

void
call_group_add(sip_call_group_t *group, sip_call_t *call)
{
    if (!call_group_exists(group, call)) {
        vector_append(group->calls, call);
    }
}

void
call_group_del(sip_call_group_t *group, sip_call_t *call)
{
    vector_remove(group->calls, call);
}

int
call_group_exists(sip_call_group_t *group, sip_call_t *call)
{
    return (vector_index(group->calls, call) >= 0) ? 1 : 0;
}

int
call_group_color(sip_call_group_t *group, sip_call_t *call)
{
    return (vector_index(group->calls, call) % 7) + 1;
}

sip_call_t *
call_group_get_next(sip_call_group_t *group, sip_call_t *call)
{
    sip_msg_t *next, *first;
    sip_call_t *c;
    int i;

    if (!group)
        return NULL;

    // Get call of the first message in group
    if (!call) {
        if ((next = call_group_get_next_msg(group, NULL))) {
            return next->call;
        }
        return NULL;
    }

    // Initialize candidate
    next = NULL;

    // Get the call with the next chronological message
    for (i = 0; i < vector_count(group->calls); i++) {
        if ((c = vector_item(group->calls, i)) == call)
            continue;

        // Get first message
        first = vector_first(c->msgs);

        // Is first message of this call older?
        if (msg_is_older(first, vector_first(call->msgs))
            && (!next || !msg_is_older(first, next))) {
            next = first;
            break;
        }
    }

    return (next) ? next->call : NULL;
}

int
call_group_count(sip_call_group_t *group)
{
    return vector_count(group->calls);
}

int
call_group_msg_count(sip_call_group_t *group)
{
    sip_call_t *call;
    vector_iter_t msgs;
    int msgcnt = 0, i;

    for (i = 0; i < vector_count(group->calls); i++) {
        call = vector_item(group->calls, i);
        msgs = vector_iterator(call->msgs);
        if (group->sdp_only) {
            vector_iterator_set_filter(&msgs, msg_has_sdp);
        }
        msgcnt += vector_iterator_count(&msgs);
    }
    return msgcnt;
}

int
call_group_msg_number(sip_call_group_t *group, sip_msg_t *msg)
{
    int number = 0;
    sip_msg_t *cur = NULL;
    while ((cur = call_group_get_next_msg(group, cur))) {
        if (group->sdp_only && !msg_has_sdp(msg))
            continue;

        if (cur == msg)
            return number;
        number++;
    }
    return 0;
}

sip_msg_t *
call_group_get_next_msg(sip_call_group_t *group, sip_msg_t *msg)
{
    sip_msg_t *next = NULL;
    sip_msg_t *cand;
    vector_iter_t msgs;
    sip_call_t *call;
    int i;

    for (i = 0; i < vector_count(group->calls); i++) {

        call = vector_item(group->calls, i);
        msgs = vector_iterator(call->msgs);

        if (msg && call == msg_get_call(msg))
            vector_iterator_set_current(&msgs, msg->index);

        if (group->sdp_only)
            vector_iterator_set_filter(&msgs, msg_has_sdp);

        cand = NULL;
        while ((cand = vector_iterator_next(&msgs))) {
            // candidate must be between msg and next
            if (msg_is_older(cand, msg) && (!next || !msg_is_older(cand, next))) {
                next = cand;
                break;
            }
        }
    }

    return sip_parse_msg(next);
}

sip_msg_t *
call_group_get_prev_msg(sip_call_group_t *group, sip_msg_t *msg)
{
    sip_msg_t *next = NULL;
    sip_msg_t *prev = NULL;

    // FIXME Horrible performance for huge dialogs
    while ((next = call_group_get_next_msg(group, next))) {
        if (next == msg)
            break;
        prev = next;
    }

    return prev;
}

rtp_stream_t *
call_group_get_next_stream(sip_call_group_t *group, rtp_stream_t *stream)
{
    rtp_stream_t *next = NULL;
    rtp_stream_t *cand;
    sip_call_t *call;
    vector_iter_t streams;
    int i;

    for (i = 0; i < vector_count(group->calls); i++) {
        call = vector_item(group->calls, i);
        streams = vector_iterator(call->streams);
        while ( (cand = vector_iterator_next(&streams))) {
            if (!stream_get_count(cand))
                continue;
            if (cand->type != PACKET_RTP)
                continue;

            // candidate must be between msg and next
            if (stream_is_older(cand, stream) && (!next || stream_is_older(next, cand))) {
                next = cand;
            }
        }
    }

    return next;
}

