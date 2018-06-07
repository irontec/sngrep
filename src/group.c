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
    // Remove all calls of the group
    sip_call_t *call;
    while ((call = vector_first(group->calls))) {
        call_group_del(group, call);
    }
    vector_destroy(group->calls);
    sng_free(group);
}

bool
call_group_has_changed(sip_call_group_t *group)
{
    bool changed = false;

    // Check if any of the group has changed
    // We check all the calls even after we found a changed one to reset all
    // the changed pointers
    sip_call_t *call = NULL;
    while ((call = call_group_get_next(group, call))) {
        if (call_has_changed(call)) {
            call->changed = false;
            changed = true;

            // If this group is based on a Call-Id, check there are no new call related
            if (group->callid && !strcmp(group->callid, call->callid)) {
                call_group_add_calls(group, call->xcalls);
            }
        }
    }

    // Return if any of the calls have changed
    return changed;
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
    if (!call) return;

    if (!call_group_exists(group, call)) {
        call->locked = true;
        vector_append(group->calls, call);
    }
}

void
call_group_add_calls(sip_call_group_t *group, vector_t *calls)
{
    sip_call_t *call;
    vector_iter_t it = vector_iterator(calls);

    // Get the call with the next chronological message
    while ((call = vector_iterator_next(&it))) {
        call->locked = true;
        if (!call_group_exists(group, call)) {
            vector_append(group->calls, call);
        }
    }
}

void
call_group_del(sip_call_group_t *group, sip_call_t *call)
{
    if (!call) return;
    call->locked = false;
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
    sip_msg_t *next;

    if (call_group_count(group) == 1) {
        sip_call_t *call = vector_first(group->calls);
        vector_iter_t it = vector_iterator(call->msgs);
        vector_iterator_set_current(&it, vector_index(call->msgs, msg));
        next = vector_iterator_next(&it);
    } else {
        vector_t *messages = vector_create(1,10);
        vector_set_sorter(messages, call_group_msg_sorter);
        vector_iter_t callsit = vector_iterator(group->calls);
        sip_call_t *call;
        while ((call = vector_iterator_next(&callsit))) {
            vector_append_vector(messages, call->msgs);
        }

        if (msg == NULL) {
            next = vector_first(messages);
        } else {
            next = vector_item(messages, vector_index(messages, msg) + 1);
        }
        vector_destroy(messages);
    }

    next = sip_parse_msg(next);
    if (next && group->sdp_only && !msg_has_sdp(next)) {
        return call_group_get_next_msg(group, next);
    }

    return next;
}

sip_msg_t *
call_group_get_prev_msg(sip_call_group_t *group, sip_msg_t *msg)
{
    sip_msg_t *prev;

    if (call_group_count(group) == 1) {
        sip_call_t *call = vector_first(group->calls);
        vector_iter_t it = vector_iterator(call->msgs);
        vector_iterator_set_current(&it, vector_index(call->msgs, msg));
        prev = vector_iterator_prev(&it);
    } else {
        vector_t *messages = vector_create(1, 10);
        vector_set_sorter(messages, call_group_msg_sorter);


        vector_iter_t callsit = vector_iterator(group->calls);
        sip_call_t *call;
        while ((call = vector_iterator_next(&callsit))) {
            vector_append_vector(messages, call->msgs);
        }

        if (msg == NULL) {
            prev = vector_last(messages);
        } else {
            prev = vector_item(messages, vector_index(messages, msg) - 1);
        }
        vector_destroy(messages);
    }

    prev = sip_parse_msg(prev);
    if (prev && group->sdp_only && !msg_has_sdp(prev)) {
        return call_group_get_prev_msg(group, prev);
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

void
call_group_msg_sorter(vector_t *vector, void *item)
{
    struct timeval curts, prevts;
    int count = vector_count(vector);
    int i;

    // Current and last packet times
    curts = msg_get_time(item);

    for (i = count - 2 ; i >= 0; i--) {
        // Get previous packet
        prevts = msg_get_time(vector_item(vector, i));
        // Check if the item is already in a sorted position
        if (timeval_is_older(curts, prevts)) {
            vector_insert(vector, item, i + 1);
            return;
        }
    }

    // Put this item at the begining of the vector
    vector_insert(vector, item, 0);
}
