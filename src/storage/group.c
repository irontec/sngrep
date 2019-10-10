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
 * @file group.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source code of functions defined in group.h
 *
 */
#include "config.h"
#include <glib.h>
#include <string.h>
#include "glib/glib-extra.h"
#include "group.h"
#include "setting.h"

CallGroup *
call_group_new()
{
    CallGroup *group = g_malloc0(sizeof(CallGroup));
    group->calls = g_ptr_array_new();
    group->msgs = g_ptr_array_new();
    group->streams = g_ptr_array_new();
    return group;
}

void
call_group_free(CallGroup *group)
{
    g_return_if_fail(group != NULL);
    g_ptr_array_free(group->calls, FALSE);
    g_ptr_array_free(group->msgs, FALSE);
    g_ptr_array_free(group->streams, FALSE);
    g_free(group);
}

void
call_group_add(CallGroup *group, Call *call)
{
    g_return_if_fail(group != NULL);
    g_return_if_fail(call != NULL);

    if (!call_group_exists(group, call)) {
        call->locked = TRUE;
        g_ptr_array_add(group->calls, call);
        g_ptr_array_add_array(group->msgs, call->msgs);
        g_ptr_array_add_array(group->streams, call->streams);
    }
}

static void
call_group_add_call_cb(Call *call, CallGroup *group)
{
    call_group_add(group, call);
}

void
call_group_add_calls(CallGroup *group, GPtrArray *calls)
{
    g_ptr_array_foreach(calls, (GFunc) call_group_add_call_cb, group);
}

void
call_group_remove(CallGroup *group, Call *call)
{
    g_return_if_fail(group != NULL);
    g_return_if_fail(call != NULL);

    // Call is no longer locked
    call->locked = FALSE;

    // Remove the call from the group
    g_ptr_array_remove(group->calls, call);
    g_ptr_array_remove_array(group->msgs, call->msgs);
    g_ptr_array_remove_array(group->streams, call->streams);
}

void
call_group_remove_all(CallGroup *group)
{
    g_return_if_fail(group != NULL);
    g_ptr_array_remove_all(group->calls);
    g_ptr_array_remove_all(group->msgs);
    g_ptr_array_remove_all(group->streams);
}

gboolean
call_group_exists(CallGroup *group, Call *call)
{
    g_return_val_if_fail(group != NULL, FALSE);
    return g_ptr_array_find(group->calls, call, NULL);
}

gboolean
call_group_changed(CallGroup *group)
{
    gboolean changed = FALSE;

    // Check if any of the group has changed
    // We check all the calls even after we found a changed one to reset all
    // the changed pointers
    for (guint i = 0; i < g_ptr_array_len(group->calls); i++) {
        Call *call = g_ptr_array_index(group->calls, i);
        if (call->changed) {
            // Reset the change flag
            call->changed = FALSE;
            // Mark the group as changed
            changed = TRUE;

            // Add new messages and streams to the group
            g_ptr_array_add_array(group->msgs, call->msgs);
            g_ptr_array_add_array(group->streams, call->streams);

            // If this group is based on a Call-Id, check there are no new call related
            if (group->callid && g_strcmp0(group->callid, call->callid) == 0) {
                call_group_add_calls(group, call->xcalls);
            }
        }

        // Check if any of the call streams is still active
        for (guint j = 0; j < g_ptr_array_len(call->streams); j++) {
            Stream *stream = g_ptr_array_index(call->streams, j);
            if (stream_is_active(stream)) {
                changed = TRUE;
            }
        }
    }

    // Return if any of the calls have changed
    return changed;
}

CallGroup *
call_group_clone(CallGroup *original)
{
    g_return_val_if_fail(original != NULL, NULL);

    // Copy Calls and messages
    CallGroup *clone = g_malloc0(sizeof(CallGroup));
    clone->calls = g_ptr_array_deep_copy(original->calls);
    clone->msgs = g_ptr_array_deep_copy(original->msgs);
    clone->streams = g_ptr_array_deep_copy(original->streams);
    return clone;
}


gint
call_group_color(CallGroup *group, Call *call)
{
    return (g_ptr_array_data_index(group->calls, call) % 7) + 1;
}

Call *
call_group_get_next(CallGroup *group, Call *call)
{
    g_return_val_if_fail(group != NULL, NULL);
    return g_ptr_array_next(group->calls, call);
}

gint
call_group_count(CallGroup *group)
{
    return g_ptr_array_len(group->calls);
}

gint
call_group_msg_count(CallGroup *group)
{
    return g_ptr_array_len(group->msgs);
}

Message *
call_group_get_next_msg(CallGroup *group, Message *msg)
{
    g_return_val_if_fail(group != NULL, NULL);

    Message *next = g_ptr_array_next(group->msgs, msg);

    // If we have a next message
    if (next != NULL) {
        // Group has no filter mode, any next message is valid
        if (group->sdp_only == 1 && !msg_has_sdp(next)) {
            return call_group_get_next_msg(group, next);
        }
        // If duplicates are hidden, go to next message
        if (setting_enabled(SETTING_CF_HIDEDUPLICATE) && msg_is_duplicate(next)) {
            return call_group_get_next_msg(group, next);
        }
    }

    return next;
}

Message *
call_group_get_prev_msg(CallGroup *group, Message *msg)
{
    g_return_val_if_fail(group != NULL, NULL);

    Message *prev = g_ptr_array_next(group->msgs, msg);

    // If we have a next message
    if (prev != NULL) {
        // Group has no filter mode, any next message is valid
        if (group->sdp_only == 1 && !msg_has_sdp(prev)) {
            return call_group_get_prev_msg(group, prev);
        }
        // If duplicates are hidden, go to previous message
        if (setting_enabled(SETTING_CF_HIDEDUPLICATE) && msg_is_duplicate(prev)) {
            return call_group_get_next_msg(group, prev);
        }

    }

    return prev;
}

Stream *
call_group_get_next_stream(CallGroup *group, Stream *stream)
{
    Stream *next = g_ptr_array_next(group->streams, stream);

    if (next != NULL) {
        if (next->type != STREAM_RTP)
            return call_group_get_next_stream(group, next);

        if (stream_get_count(next) == 0)
            return call_group_get_next_stream(group, next);
    }

    return next;
}

