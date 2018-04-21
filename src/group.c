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
#include "config.h"
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include "glib-utils.h"
#include "group.h"

SipCallGroup *
call_group_new()
{
    SipCallGroup *group = g_malloc0(sizeof(SipCallGroup));
    group->calls = g_sequence_new(NULL);
    return group;
}

void
call_group_free(SipCallGroup *group)
{
    // Remove all calls of the group
    SipCall *call;
    while ((call = g_sequence_first(group->calls))) {
        call_group_del(group, call);
    }
    g_sequence_free(group->calls);
    sng_free(group);
}

gboolean
call_group_changed(SipCallGroup *group)
{
    gboolean  changed = FALSE;

    // Check if any of the group has changed
    // We check all the calls even after we found a changed one to reset all
    // the changed pointers
    SipCall *call = NULL;
    while ((call = call_group_get_next(group, call))) {
        if (call_has_changed(call)) {
            call->changed = false;
            changed = TRUE;

            // If this group is based on a Call-Id, check there are no new call related
            if (group->callid && !strcmp(group->callid, call->callid)) {
                call_group_add_calls(group, call->xcalls);
            }
        }
    }

    // Return if any of the calls have changed
    return changed;
}

SipCallGroup *
call_group_clone(SipCallGroup *original)
{
    SipCallGroup *clone;

    if (!original)
        return NULL;

    if (!(clone = sng_malloc(sizeof(SipCallGroup)))) {
        return NULL;
    }

    clone->calls = g_sequence_copy(original->calls, NULL, NULL);
    return clone;
}

void
call_group_add(SipCallGroup *group, SipCall *call)
{
    if (!call) return;

    if (!call_group_exists(group, call)) {
        call->locked = true;
        g_sequence_append(group->calls, call);
    }
}

void
call_group_add_calls(SipCallGroup *group, GSequence *calls)
{
    GSequenceIter *it = g_sequence_get_begin_iter(calls);

    // Get the call with the next chronological message
    for (;!g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        SipCall *call = g_sequence_get(it);
        call->locked = true;
        if (!call_group_exists(group, call)) {
            g_sequence_append(group->calls, call);
        }
    }
}

void
call_group_del(SipCallGroup *group, SipCall *call)
{
    if (!call) return;
    call->locked = false;
    g_sequence_remove_data(group->calls, call);
}

gboolean
call_group_exists(SipCallGroup *group, SipCall *call)
{
    return (g_sequence_index(group->calls, call) >= 0) ? TRUE : FALSE;
}

gint
call_group_color(SipCallGroup *group, SipCall *call)
{
    return (g_sequence_index(group->calls, call) % 7) + 1;
}

SipCall *
call_group_get_next(SipCallGroup *group, SipCall *call)
{
    SipMsg *next, *first;
    SipCall *c;

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
    for (guint i = 0; i < g_sequence_get_length(group->calls); i++) {
        if ((c = g_sequence_nth(group->calls, i)) == call)
            continue;

        // Get first message
        first = g_sequence_first(c->msgs);

        // Is first message of this call older?
        if (msg_is_older(first, g_sequence_first(call->msgs))
            && (!next || !msg_is_older(first, next))) {
            next = first;
            break;
        }
    }

    return (next) ? next->call : NULL;
}

gint
call_group_count(SipCallGroup *group)
{
    return g_sequence_get_length(group->calls);
}

gint
call_group_msg_count(SipCallGroup *group)
{
    SipCall *call;
    SipMsg *msg;
    GSequenceIter *msgs;
    gint msgcnt = 0;

    for (guint i = 0; i < g_sequence_get_length(group->calls); i++) {
        call = g_sequence_nth(group->calls, i);
        msgs = g_sequence_get_begin_iter(call->msgs);
        for (;!g_sequence_iter_is_end(msgs); msgs = g_sequence_iter_next(msgs)) {
            msg = g_sequence_get(msgs);
            if (group->sdp_only && !msg_has_sdp(msg)) {
                continue;
            }
            msgcnt++;
        }
    }
    return msgcnt;
}

gint
call_group_msg_number(SipCallGroup *group, SipMsg *msg)
{
    gint number = 0;
    SipMsg *cur = NULL;

    while ((cur = call_group_get_next_msg(group, cur))) {
        if (group->sdp_only && !msg_has_sdp(msg))
            continue;

        if (cur == msg)
            return number;
        number++;
    }

    return 0;
}

SipMsg *
call_group_get_next_msg(SipCallGroup *group, SipMsg *msg)
{
    SipMsg *next;
    GSequence *messages = g_sequence_new(NULL);

    GSequenceIter *it = g_sequence_get_begin_iter(group->calls);
    for (;!g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        SipCall *call = g_sequence_get(it);
        g_sequence_append_sequence(messages, call->msgs);
    }
    g_sequence_sort(messages, call_group_msg_sorter, NULL);

    if (msg == NULL) {
        next = g_sequence_first(messages);
    } else {
        next = g_sequence_nth(messages, g_sequence_index(messages, msg) + 1);
    }
    g_sequence_free(messages);

    next = sip_parse_msg(next);
    if (next && group->sdp_only && !msg_has_sdp(next)) {
        return call_group_get_next_msg(group, next);
    }

    return next;
}

SipMsg *
call_group_get_prev_msg(SipCallGroup *group, SipMsg *msg)
{
    SipMsg *prev;
    GSequence *messages = g_sequence_new(NULL);
    GSequenceIter *calls = g_sequence_get_begin_iter(group->calls);
    SipCall *call;
    for (;!g_sequence_iter_is_end(calls); calls = g_sequence_iter_next(calls)) {
        call = g_sequence_get(calls);
        GSequenceIter *msgs = g_sequence_get_begin_iter(call->msgs);
        for (;!g_sequence_iter_is_end(msgs); msgs = g_sequence_iter_next(msgs)) {
            g_sequence_insert_sorted(messages, g_sequence_get(msgs), call_group_msg_sorter, NULL);
        }
    }

    if (msg == NULL) {
        prev = g_sequence_last(messages);
    } else {
        prev = g_sequence_nth(messages, g_sequence_index(messages, msg) - 1);
    }
    g_sequence_free(messages);

    prev = sip_parse_msg(prev);
    if (prev && group->sdp_only && !msg_has_sdp(prev)) {
        return call_group_get_prev_msg(group, prev);
    }

    return prev;
}

rtp_stream_t *
call_group_get_next_stream(SipCallGroup *group, rtp_stream_t *stream)
{
    rtp_stream_t *next = NULL;
    rtp_stream_t *cand;
    SipCall *call;
    GSequenceIter *streams;

    for (guint i = 0; i < g_sequence_get_length(group->calls); i++) {
        call = g_sequence_nth(group->calls, i);
        streams = g_sequence_get_begin_iter(call->streams);
        for (;!g_sequence_iter_is_end(streams); streams = g_sequence_iter_next(streams)) {
            cand = g_sequence_get(streams);
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

gint
call_group_msg_sorter(gconstpointer a, gconstpointer b, G_GNUC_UNUSED gpointer user_data)
{
    return timeval_is_older(
        msg_get_time(a),
        msg_get_time(b)
    );
}
