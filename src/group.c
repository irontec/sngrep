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
    return group;
}

void
call_group_free(SipCallGroup *group)
{
    g_list_free(group->msgs);
    g_list_free(group->calls);
    g_free(group);
}

gboolean
call_group_changed(SipCallGroup *group)
{
    gboolean changed = FALSE;

    // Check if any of the group has changed
    // We check all the calls even after we found a changed one to reset all
    // the changed pointers
    for (GList *l = group->calls; l != NULL; l = l->next) {
        SipCall *call = l->data;
        if (call->changed) {
            // Reset the change flag
            call->changed = FALSE;
            // Mark the group as changed
            changed = TRUE;

            // If this group is based on a Call-Id, check there are no new call related
            if (group->callid && !g_strcmp0(group->callid, call->callid)) {
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
    g_return_val_if_fail(original != NULL, NULL);

    // Copy Calls and messages
    SipCallGroup *clone = g_malloc0(sizeof(SipCallGroup));
    clone->calls = g_list_copy(original->calls);
    clone->msgs = g_list_copy(original->msgs);
    return clone;
}

void
call_group_add(SipCallGroup *group, SipCall *call)
{
    g_return_if_fail(group != NULL);
    g_return_if_fail(call != NULL);

    if (!call_group_exists(group, call)) {
        call->locked = TRUE;
        group->calls = g_list_append(group->calls, call);
        // Add all call messages
        for (guint i = 0; i < g_ptr_array_len(call->msgs); i++) {
            group->msgs = g_list_append(group->msgs, g_ptr_array_index(call->msgs, i));
        }
    }
}

void
call_group_add_calls(SipCallGroup *group, GSequence *calls)
{
    GSequenceIter *it = g_sequence_get_begin_iter(calls);

    // Get the call with the next chronological message
    for (;!g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        call_group_add(group, g_sequence_get(it));
    }
}

void
call_group_remove(SipCallGroup *group, SipCall *call)
{
    g_return_if_fail(group != NULL);
    g_return_if_fail(call != NULL);

    // Call is no longer locked
    call->locked = FALSE;

    // Remove the call from the group
    group->calls = g_list_remove(group->calls, call);

    // Remove all messages of this call from the group
    GList *l = group->msgs;
    while (l != NULL) {
        GList *next = l->next;
        if (msg_get_call(l->data) == call) {
            group->msgs = g_list_delete_link(group->msgs, l);
        }
        l = next;
    }
}

void
call_group_remove_all(SipCallGroup *group)
{
    g_list_free(group->calls);
    g_list_free(group->msgs);
}

gboolean
call_group_exists(SipCallGroup *group, SipCall *call)
{
    return (g_list_index(group->calls, call) != -1) ? TRUE : FALSE;
}

gint
call_group_color(SipCallGroup *group, SipCall *call)
{
    return (g_list_index(group->calls, call) % 7) + 1;
}

SipCall *
call_group_get_next(SipCallGroup *group, SipCall *call)
{
    if (call == NULL && group->calls != NULL) {
        return g_list_nth_data(group->calls, 0);
    }

    GList *l = g_list_find(group->calls, call);
    if (l != NULL) {
        GList *next = g_list_next(l);
        return (next != NULL) ? next->data : NULL;
    }
    return NULL;
}

gint
call_group_count(SipCallGroup *group)
{
    return g_list_length(group->calls);
}

gint
call_group_msg_count(SipCallGroup *group)
{
    return g_list_length(group->msgs);
}

SipMsg *
call_group_get_next_msg(SipCallGroup *group, SipMsg *msg)
{
    GList *next = NULL;

    // Get Next Message based on given
    if (msg == NULL) {
        next = g_list_first(group->msgs);
    } else {
        next = g_list_next(g_list_find(group->msgs, msg));
    }

    // If we have a next message
    if (next != NULL) {
        // Group has no filter mode, any next message is valid
        if (group->sdp_only == 0) {
            return next->data;
        }

        // Get next message that has SDP
        while (next != NULL) {
            if (msg_has_sdp(next->data)) {
                return next->data;
            }
            next = next->next;
        }
    }

    return NULL;
}

SipMsg *
call_group_get_prev_msg(SipCallGroup *group, SipMsg *msg)
{
    GList *prev = NULL;

    // Get previous message based on given
    if (msg == NULL) {
        prev = g_list_last(group->msgs);
    } else {
        prev = g_list_previous(g_list_find(group->msgs, msg));
    }

    // If we have a previous message
    if (prev != NULL) {
        // Group has no filter mode, any previous message is valid
        if (group->sdp_only == 0) {
            return prev->data;
        }

        // Get next message that has SDP
        while (prev != NULL) {
            if (msg_has_sdp(prev->data)) {
                return prev->data;
            }
            prev = prev->prev;
        }
    }

    return NULL;
}

RtpStream *
call_group_get_next_stream(SipCallGroup *group, RtpStream *stream)
{
    RtpStream *next = NULL;
    RtpStream *cand;
    SipCall *call;

    for (guint i = 0; i < g_list_length(group->calls); i++) {
        call = g_list_nth_data(group->calls, i);
        for (guint i = 0; i < g_ptr_array_len(call->streams); i++) {
            cand = g_ptr_array_index(call->streams, i);

            if (!stream_get_count(cand))
                continue;

            if (cand->type != PACKET_RTP)
                continue;

            if (cand == stream)
                continue;

            // candidate must be between msg and next
            if (stream_is_older(cand, stream) && (!next || stream_is_older(next, cand))) {
                next = cand;
            }
        }
    }

    return next;
}

