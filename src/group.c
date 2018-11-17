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
    group->calls = g_ptr_array_new();
    group->msgs = g_ptr_array_new();
    group->streams = g_ptr_array_new();
    return group;
}

void
call_group_free(SipCallGroup *group)
{
    g_return_if_fail(group != NULL);
    g_ptr_array_free(group->calls, FALSE);
    g_ptr_array_free(group->msgs, FALSE);
    g_free(group);
}

void
call_group_add(SipCallGroup *group, SipCall *call)
{
    g_return_if_fail(group != NULL);
    g_return_if_fail(call != NULL);

    if (!call_group_exists(group, call)) {
        call->locked = TRUE;
        g_ptr_array_add(group->calls, call);
        g_ptr_array_add_array(group->msgs, call->msgs);
    }
}

void
call_group_add_calls(SipCallGroup *group, GPtrArray *calls)
{
    g_ptr_array_add_array(group->calls, calls);
}

void
call_group_remove(SipCallGroup *group, SipCall *call)
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
call_group_remove_all(SipCallGroup *group)
{
    g_return_if_fail(group != NULL);
    g_ptr_array_free(group->calls, FALSE);
    g_ptr_array_free(group->msgs, FALSE);
    g_ptr_array_free(group->streams, FALSE);
}

gboolean
call_group_exists(SipCallGroup *group, SipCall *call)
{
    g_return_val_if_fail(group != NULL, FALSE);
    return g_ptr_array_find(group->calls, call, NULL);
}

gboolean
call_group_changed(SipCallGroup *group)
{
    gboolean changed = FALSE;

    // Check if any of the group has changed
    // We check all the calls even after we found a changed one to reset all
    // the changed pointers
    for (guint i = 0; i < g_ptr_array_len(group->calls); i++) {
        SipCall *call = g_ptr_array_index(group->calls, i);
        if (call->changed) {
            // Reset the change flag
            call->changed = FALSE;
            // Mark the group as changed
            changed = TRUE;

            // Add new messages and streams to the group
            g_ptr_array_add_array(group->msgs, call->msgs);
            g_ptr_array_add_array(group->streams, call->streams);

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
    clone->calls = g_ptr_array_copy(original->calls);
    clone->msgs = g_ptr_array_copy(original->msgs);
    clone->streams = g_ptr_array_copy(original->streams);
    return clone;
}


gint
call_group_color(SipCallGroup *group, SipCall *call)
{
    return (g_ptr_array_data_index(group->calls, call) % 7) + 1;
}

SipCall *
call_group_get_next(SipCallGroup *group, SipCall *call)
{
    g_return_val_if_fail(group != NULL, NULL);
    return g_ptr_array_next(group->calls, call);
}

gint
call_group_count(SipCallGroup *group)
{
    return g_ptr_array_len(group->calls);
}

gint
call_group_msg_count(SipCallGroup *group)
{
    return g_ptr_array_len(group->msgs);
}

SipMsg *
call_group_get_next_msg(SipCallGroup *group, SipMsg *msg)
{
    g_return_val_if_fail(group != NULL, NULL);

    SipMsg *next = g_ptr_array_next(group->msgs, msg);

    // If we have a next message
    if (next != NULL) {
        // Group has no filter mode, any next message is valid
        if (group->sdp_only == 1 && !msg_has_sdp(next)) {
            return call_group_get_next_msg(group, next);
        }
    }

    return next;
}

SipMsg *
call_group_get_prev_msg(SipCallGroup *group, SipMsg *msg)
{
    g_return_val_if_fail(group != NULL, NULL);

    SipMsg *prev = g_ptr_array_next(group->msgs, msg);

    // If we have a next message
    if (prev != NULL) {
        // Group has no filter mode, any next message is valid
        if (group->sdp_only == 1 && !msg_has_sdp(prev)) {
            return call_group_get_prev_msg(group, prev);
        }
    }

    return prev;
}

RtpStream *
call_group_get_next_stream(SipCallGroup *group, RtpStream *stream)
{
    RtpStream *next = g_ptr_array_next(group->streams, stream);

    if (next != NULL) {
        if (next->type != PACKET_RTP)
            return call_group_get_next_stream(group, next);

        if (stream_get_count(next) == 0)
            return call_group_get_next_stream(group, next);
    }

    return next;
}

