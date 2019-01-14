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
 * @file call.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage SIP call data
 *
 * This file contains the functions and structure to manage SIP call data
 *
 */
#include "config.h"
#include <glib.h>
#include "glib-extra.h"
#include "call.h"
#include "capture/packet/packet_sip.h"
#include "storage.h"
#include "setting.h"

Call *
call_create(const gchar *callid, const gchar *xcallid)
{
    // Initialize a new call structure
    Call *call = g_malloc0(sizeof(Call));

    // Create a vector to store call messages
    call->msgs = g_ptr_array_new_with_free_func((GDestroyNotify) msg_free);

    // Create an empty vector to store rtp packets
    call->rtp_packets = g_sequence_new((GDestroyNotify) packet_free);

    // Create an empty vector to strore stream data
    call->streams = g_ptr_array_new_with_free_func((GDestroyNotify) stream_free);

    // Create an empty vector to store x-calls
    call->xcalls = g_ptr_array_new();

    // Initialize call filter status
    call->filtered = -1;

    // Set message callid
    call->callid = callid;
    call->xcallid = xcallid;

    return call;
}

void
call_destroy(gpointer item)
{
    Call *call = item;
    // Remove all call messages
    g_ptr_array_free(call->msgs, TRUE);
    // Remove all call streams
    g_ptr_array_free(call->streams, TRUE);
    // Remove all call rtp packets
    g_sequence_free(call->rtp_packets);
    // Remove all xcalls
    g_ptr_array_free(call->xcalls, TRUE);

    // Deallocate call memory
    g_free(call->reasontxt);
    g_free(call);
}

void
call_add_message(Call *call, Message *msg)
{
    // Set the message owner
    msg->call = call;
    // Put this msg at the end of the msg list
    g_ptr_array_add(call->msgs, msg);
    // Flag this call as changed
    call->changed = TRUE;
    // Check if message is a retransmission
    msg->retrans = msg_is_retrans(msg);
}

void
call_add_stream(Call *call, RtpStream *stream)
{
    // Store stream
    g_ptr_array_add(call->streams, stream);
    // Flag this call as changed
    call->changed = TRUE;
}

guint
call_msg_count(const Call *call)
{
    return g_ptr_array_len(call->msgs);
}

enum CallState
call_state(Call *call)
{
    return call->state;
}

int
call_is_invite(Call *call)
{
    Message *first = g_ptr_array_first(call->msgs);
    g_return_val_if_fail(first != NULL, 0);

    return packet_sip_method(first->packet) == SIP_METHOD_INVITE;
}

void
call_update_state(Call *call, Message *msg)
{
    if (!call_is_invite(call))
        return;

    // Get current message Method / Response Code
    guint msg_reqresp = packet_sip_method(msg->packet);
    guint64 msg_cseq = packet_sip_cseq(msg->packet);

    // If this message is actually a call, get its current state
    if (call->state) {
        if (call->state == CALL_STATE_CALLSETUP) {
            if (msg_reqresp == SIP_METHOD_ACK && call->invitecseq == msg_cseq) {
                // Alice and Bob are talking
                call->state = CALL_STATE_INCALL;
                call->cstart_msg = msg;
            } else if (msg_reqresp == SIP_METHOD_CANCEL) {
                // Alice is not in the mood
                call->state = CALL_STATE_CANCELLED;
            } else if ((msg_reqresp == 480) || (msg_reqresp == 486) || (msg_reqresp == 600)) {
                // Bob is busy
                call->state = CALL_STATE_BUSY;
            } else if (msg_reqresp > 400 && call->invitecseq == msg_cseq) {
                // Bob is not in the mood
                call->state = CALL_STATE_REJECTED;
            } else if (msg_reqresp > 300) {
                // Bob has diversion
                call->state = CALL_STATE_DIVERTED;
            }
        } else if (call->state == CALL_STATE_INCALL) {
            if (msg_reqresp == SIP_METHOD_BYE) {
                // Thanks for all the fish!
                call->state = CALL_STATE_COMPLETED;
                call->cend_msg = msg;
            }
        } else if (msg_reqresp == SIP_METHOD_INVITE && call->state != CALL_STATE_INCALL) {
            // Call is being setup (after proper authentication)
            call->invitecseq = msg_cseq;
            call->state = CALL_STATE_CALLSETUP;
        }
    } else {
        // This is actually a call
        if (msg_reqresp == SIP_METHOD_INVITE) {
            call->invitecseq = msg_cseq;
            call->state = CALL_STATE_CALLSETUP;
        }
    }
}

const gchar *
call_state_to_str(enum CallState state)
{
    switch (state) {
        case CALL_STATE_CALLSETUP:
            return "CALL SETUP";
        case CALL_STATE_INCALL:
            return "IN CALL";
        case CALL_STATE_CANCELLED:
            return "CANCELLED";
        case CALL_STATE_REJECTED:
            return "REJECTED";
        case CALL_STATE_BUSY:
            return "BUSY";
        case CALL_STATE_DIVERTED:
            return "DIVERTED";
        case CALL_STATE_COMPLETED:
            return "COMPLETED";
    }
    return "";
}

gint
call_attr_compare(const Call *one, const Call *two, enum AttributeId id)
{
    const gchar *onevalue = NULL, *twovalue = NULL;
    int oneintvalue = 0, twointvalue = 0;
    int comparetype; /* TODO 0 = string compare, 1 = int comprare */
    Message *msg_one = g_ptr_array_first(one->msgs);
    Message *msg_two = g_ptr_array_first(two->msgs);

    switch (id) {
        case ATTR_CALLINDEX:
            oneintvalue = one->index;
            twointvalue = two->index;
            comparetype = 1;
            break;
        case ATTR_MSGCNT:
            oneintvalue = call_msg_count(one);
            twointvalue = call_msg_count(two);
            comparetype = 1;
            break;
        default:
            // Get attribute values
            onevalue = msg_get_attribute(msg_one, id);
            twovalue = msg_get_attribute(msg_two, id);
            comparetype = 0;
            break;
    }

    switch (comparetype) {
        case 0:
            if (strlen(twovalue) == 0 && strlen(onevalue) == 0)
                return 0;
            if (strlen(twovalue) == 0)
                return 1;
            if (strlen(onevalue) == 0)
                return -1;
            return strcmp(onevalue, twovalue);
        case 1:
            if (oneintvalue == twointvalue) return 0;
            if (oneintvalue > twointvalue) return 1;
            if (oneintvalue < twointvalue) return -1;
            /* fall-thru */
        default:
            return 0;
    }
}

void
call_add_xcall(Call *call, Call *xcall)
{
    if (!call || !xcall)
        return;

    // Mark this call as changed
    call->changed = TRUE;

    // Add the xcall to the list
    g_ptr_array_add(call->xcalls, xcall);
}

RtpStream *
call_find_stream(Call *call, Address src, Address dst)
{
    RtpStream *stream;

    // Look for an incomplete stream with this destination
    for (guint i = 0; i < g_ptr_array_len(call->streams); i++) {
        stream = g_ptr_array_index(call->streams, i);
        if (addressport_equals(dst, stream->dst)) {
            if (!src.port) {
                return stream;
            } else {
                if (!stream->packets->len) {
                    return stream;
                }
            }
        }
    }

    // Try to look for a complete stream with this destination
    if (src.port) {
        return call_find_stream_exact(call, src, dst);
    }

    // Nothing found
    return NULL;
}

RtpStream *
call_find_stream_exact(Call *call, Address src, Address dst)
{
    RtpStream *stream;

    // Create an iterator for call streams
    for (guint i = 0; i < g_ptr_array_len(call->streams); i++) {
        stream = g_ptr_array_index(call->streams, i);
        if (addressport_equals(src, stream->src) &&
            addressport_equals(dst, stream->dst)) {
            return stream;
        }
    }

    // Nothing found
    return NULL;
}

Message *
call_find_message_cseq(Call *call, guint64 cseq)
{
    // TODO CSeq is not enough, Direction must be checked

    // Search for the fist message with requested CSeq in the same direction
    for (guint i = 0; i < g_ptr_array_len(call->msgs); i++) {
        Message *msg = g_ptr_array_index(call->msgs, i);
        if (msg_get_cseq(msg) == cseq) {
            return msg;
        }
    }

    return NULL;
}
