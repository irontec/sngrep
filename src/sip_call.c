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
 * @file sip_call.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage SIP call data
 *
 * This file contains the functions and structure to manage SIP call data
 *
 */
#include "config.h"
#include <glib.h>
#include "glib-utils.h"
#include "sip_call.h"
#include "packet/dissectors/packet_sip.h"
#include "storage.h"
#include "setting.h"

SipCall *
call_create(char *callid, char *xcallid)
{
    // Initialize a new call structure
    SipCall *call = g_malloc0(sizeof(SipCall));

    // Create a vector to store call messages
    call->msgs = g_sequence_new(msg_destroy);

    // Create an empty vector to store rtp packets
    call->rtp_packets = g_sequence_new((GDestroyNotify) packet_free);

    // Create an empty vector to strore stream data
    call->streams = g_sequence_new(g_free);

    // Create an empty vector to store x-calls
    call->xcalls = g_sequence_new(NULL);

    // Initialize call filter status
    call->filtered = -1;

    // Set message callid
    call->callid = g_strdup(callid);
    call->xcallid = (xcallid) ? g_strdup(xcallid) : g_strdup("");

    return call;
}

void
call_destroy(gpointer item)
{
    SipCall *call = item;
    // Remove all call messages
    g_sequence_free(call->msgs);
    // Remove all call streams
    g_sequence_free(call->streams);
    // Remove all call rtp packets
    g_sequence_free(call->rtp_packets);
    // Remove all xcalls
    g_sequence_free(call->xcalls);

    // Deallocate call memory
    g_free(call->callid);
    g_free(call->xcallid);
    g_free(call->reasontxt);
    g_free(call);
}

void
call_add_message(SipCall *call, SipMsg *msg)
{
    // Set the message owner
    msg->call = call;
    // Put this msg at the end of the msg list
    g_sequence_append(call->msgs, msg);
    // Flag this call as changed
    call->changed = true;
    // Check if message is a retransmission
    msg->retrans = msg_is_retrans(msg);
}

void
call_add_stream(SipCall *call, rtp_stream_t *stream)
{
    // Store stream
    g_sequence_append(call->streams, stream);
    // Flag this call as changed
    call->changed = true;
}

void
call_add_rtp_packet(SipCall *call, Packet *packet)
{
    // Store packet
    g_sequence_append(call->rtp_packets, packet);
    // Flag this call as changed
    call->changed = true;
}

guint
call_msg_count(const SipCall *call)
{
    return (guint) g_sequence_get_length(call->msgs);
}

int
call_is_active(SipCall *call)
{
    return (call->state == CALL_STATE_CALLSETUP || call->state == CALL_STATE_INCALL);
}

int
call_is_invite(SipCall *call)
{
    SipMsg *first;
    if ((first = g_sequence_first(call->msgs)))
        return (packet_sip_method(first->packet) == SIP_METHOD_INVITE);

    return 0;
}

SipMsg *
call_msg_with_media(SipCall *call, Address dst)
{
    SipMsg *msg;
    PacketSdpMedia *media;
    GSequenceIter *itmsg;
    GSequenceIter *itmedia;

    // Get message with media address configured in given dst
    itmsg = g_sequence_get_begin_iter(call->msgs);
    for (;!g_sequence_iter_is_end(itmsg); itmsg = g_sequence_iter_next(itmsg)) {
        msg = g_sequence_get(itmsg);
        itmedia = g_sequence_get_begin_iter(msg->medias);
        for (;!g_sequence_iter_is_end(itmedia); itmedia = g_sequence_iter_next(itmedia)) {
            media = g_sequence_get(itmedia);
            if (addressport_equals(dst, media->address)) {
                return msg;
            }
        }
    }

    return NULL;
}

void
call_update_state(SipCall *call, SipMsg *msg)
{
    if (!call_is_invite(call))
        return;

    // Get current message Method / Response Code
    guint msg_reqresp = packet_sip_method(msg->packet);
    gint msg_cseq = atoi(packet_sip_header(msg->packet, SIP_HEADER_CSEQ));

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
            } else if ((msg_reqresp == 480) || (msg_reqresp == 486) || (msg_reqresp == 600 )) {
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
        } else if (msg_reqresp == SIP_METHOD_INVITE && call->state !=  CALL_STATE_INCALL) {
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

const char *
call_get_attribute(const SipCall *call, enum sip_attr_id id, char *value)
{
    SipMsg *first, *last;

    if (!call)
        return NULL;

    switch (id) {
        case SIP_ATTR_CALLINDEX:
            sprintf(value, "%d", call->index);
            break;
        case SIP_ATTR_CALLID:
            sprintf(value, "%s", call->callid);
            break;
        case SIP_ATTR_XCALLID:
            sprintf(value, "%s", call->xcallid);
            break;
        case SIP_ATTR_MSGCNT:
            sprintf(value, "%d", g_sequence_get_length(call->msgs));
            break;
        case SIP_ATTR_CALLSTATE:
            sprintf(value, "%s", call_state_to_str(call->state));
            break;
        case SIP_ATTR_TRANSPORT:
            first = g_sequence_first(call->msgs);
//@todo            sprintf(value, "%s", sip_transport_str(first->packet->type));
            break;
        case SIP_ATTR_CONVDUR:
            timeval_to_duration(msg_get_time(call->cstart_msg), msg_get_time(call->cend_msg), value);
            break;
        case SIP_ATTR_TOTALDUR:
            first = g_sequence_first(call->msgs);
            last = g_sequence_last(call->msgs);
            timeval_to_duration(msg_get_time(first), msg_get_time(last), value);
            break;
        case SIP_ATTR_REASON_TXT:
            if (call->reasontxt)
                sprintf(value, "%s", call->reasontxt);
            break;
        case SIP_ATTR_WARNING:
            if (call->warning)
                sprintf(value, "%d", call->warning);
            break;
        default:
            return msg_get_attribute(g_sequence_first(call->msgs), id, value);
    }

    return strlen(value) ? value : NULL;
}

const gchar *
call_state_to_str(enum call_state state)
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
call_attr_compare(const SipCall *one, const SipCall *two, enum sip_attr_id id)
{
    char onevalue[256], twovalue[256];
    int oneintvalue = 0, twointvalue = 0;
    int comparetype; /* TODO 0 = string compare, 1 = int comprare */

    switch (id) {
        case SIP_ATTR_CALLINDEX:
            oneintvalue = one->index;
            twointvalue = two->index;
            comparetype = 1;
            break;
        case SIP_ATTR_MSGCNT:
            oneintvalue = call_msg_count(one);
            twointvalue = call_msg_count(two);
            comparetype = 1;
            break;
        default:
            // Get attribute values
            memset(onevalue, 0, sizeof(onevalue));
            memset(twovalue, 0, sizeof(twovalue));
            call_get_attribute(one, id, onevalue);
            call_get_attribute(two, id, twovalue);
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
            /* no break */
        default:
            return 0;
    }
}

void
call_add_xcall(SipCall *call, SipCall *xcall)
{
    if (!call || !xcall)
        return;

    // Mark this call as changed
    call->changed = true;
    // Add the xcall to the list
    g_sequence_append(call->xcalls, xcall);
}

rtp_stream_t *
call_find_stream(SipCall *call, Address src, Address dst)
{
    rtp_stream_t *stream;
    GSequenceIter *it;

    // Create an iterator for call streams
    it = g_sequence_get_end_iter(call->streams);

    // Look for an incomplete stream with this destination
    while(!g_sequence_iter_is_begin(it)) {
        it = g_sequence_iter_prev(it);
        stream = g_sequence_get(it);
        if (addressport_equals(dst, stream->dst)) {
            if (!src.port) {
                return stream;
            } else {
                if (!stream->pktcnt) {
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

rtp_stream_t *
call_find_stream_exact(SipCall *call, Address src, Address dst)
{
    rtp_stream_t *stream;
    GSequenceIter *it;

    // Create an iterator for call streams
    it = g_sequence_get_end_iter(call->streams);

    while(!g_sequence_iter_is_begin(it)) {
        it = g_sequence_iter_prev(it);
        stream = g_sequence_get(it);
        if (addressport_equals(src, stream->src) &&
            addressport_equals(dst, stream->dst)) {
            return stream;
        }
    }

    // Nothing found
    return NULL;
}
