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

sip_call_t *
call_create(char *callid, char *xcallid)
{
    sip_call_t *call;

    // Initialize a new call structure
    if (!(call = sng_malloc(sizeof(sip_call_t))))
        return NULL;

    // Create a vector to store call messages
    call->msgs = g_sequence_new(msg_destroy);

    // Create an empty vector to store rtp packets
    call->rtp_packets = g_sequence_new(packet_destroy);

    // Create an empty vector to strore stream data
    call->streams = g_sequence_new(sng_free);

    // Create an empty vector to store x-calls
    call->xcalls = g_sequence_new(NULL);

    // Initialize call filter status
    call->filtered = -1;

    // Set message callid
    call->callid = strdup(callid);
    call->xcallid = (xcallid) ? strdup(xcallid) : strdup("");

    return call;
}

void
call_destroy(gpointer item)
{
    sip_call_t *call = item;
    // Remove all call messages
    g_sequence_free(call->msgs);
    // Remove all call streams
    g_sequence_free(call->streams);
    // Remove all call rtp packets
    g_sequence_free(call->rtp_packets);
    // Remove all xcalls
    g_sequence_free(call->xcalls);
    // Deallocate call memory
    sng_free(call->callid);
    sng_free(call->xcallid);
    sng_free(call->reasontxt);
    sng_free(call);
}

bool
call_has_changed(sip_call_t *call)
{
    return call->changed;
}

void
call_add_message(sip_call_t *call, sip_msg_t *msg)
{
    // Set the message owner
    msg->call = call;
    // Put this msg at the end of the msg list
    g_sequence_append(call->msgs, msg);
    msg->index = g_sequence_get_length(call->msgs);
    // Flag this call as changed
    call->changed = true;
}

void
call_add_stream(sip_call_t *call, rtp_stream_t *stream)
{
    // Store stream
    g_sequence_append(call->streams, stream);
    // Flag this call as changed
    call->changed = true;
}

void
call_add_rtp_packet(sip_call_t *call, packet_t *packet)
{
    // Store packet
    g_sequence_append(call->rtp_packets, packet);
    // Flag this call as changed
    call->changed = true;
}

int
call_msg_count(const sip_call_t *call)
{
    return g_sequence_get_length(call->msgs);
}

int
call_is_active(sip_call_t *call)
{
    return (call->state == SIP_CALLSTATE_CALLSETUP || call->state == SIP_CALLSTATE_INCALL);
}

int
call_is_invite(sip_call_t *call)
{
    sip_msg_t *first;
    if ((first = g_sequence_first(call->msgs)))
        return (first->reqresp == SIP_METHOD_INVITE);

    return 0;
}

void
call_msg_retrans_check(sip_msg_t *msg)
{
    sip_msg_t *prev = NULL;
    GSequenceIter *it;

    // Get previous message in call with same origin and destination
    it = g_sequence_get_end_iter(msg->call->msgs);

    // Skip already added message
    it = g_sequence_iter_prev(it);

    while(!g_sequence_iter_is_begin(it)) {
        it = g_sequence_iter_prev(it);
        prev = g_sequence_get(it);
        // Same addresses
        if (addressport_equals(prev->packet->src, msg->packet->src) &&
            addressport_equals(prev->packet->dst, msg->packet->dst)) {
            // Same payload
            if (!strcasecmp(msg_get_payload(msg), msg_get_payload(prev))) {
                // Store the flag that determines if message is retrans
                msg->retrans = prev;
                break;
            }
        }
    }

}

sip_msg_t *
call_msg_with_media(sip_call_t *call, Address dst)
{
    sip_msg_t *msg;
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
call_update_state(sip_call_t *call, sip_msg_t *msg)
{
    int reqresp;

    if (!call_is_invite(call))
        return;

    // Get current message Method / Response Code
    reqresp = msg->reqresp;

    // If this message is actually a call, get its current state
    if (call->state) {
        if (call->state == SIP_CALLSTATE_CALLSETUP) {
            if (reqresp == SIP_METHOD_ACK && call->invitecseq == msg->cseq) {
                // Alice and Bob are talking
                call->state = SIP_CALLSTATE_INCALL;
                call->cstart_msg = msg;
            } else if (reqresp == SIP_METHOD_CANCEL) {
                // Alice is not in the mood
                call->state = SIP_CALLSTATE_CANCELLED;
            } else if ((reqresp == 480) || (reqresp == 486) || (reqresp == 600 )) {
                // Bob is busy
                call->state = SIP_CALLSTATE_BUSY;
            } else if (reqresp > 400 && call->invitecseq == msg->cseq) {
                // Bob is not in the mood
                call->state = SIP_CALLSTATE_REJECTED;
            } else if (reqresp > 300) {
                // Bob has diversion
                call->state = SIP_CALLSTATE_DIVERTED;
            }
        } else if (call->state == SIP_CALLSTATE_INCALL) {
            if (reqresp == SIP_METHOD_BYE) {
                // Thanks for all the fish!
                call->state = SIP_CALLSTATE_COMPLETED;
                call->cend_msg = msg;
            }
        } else if (reqresp == SIP_METHOD_INVITE && call->state !=  SIP_CALLSTATE_INCALL) {
            // Call is being setup (after proper authentication)
            call->invitecseq = msg->cseq;
            call->state = SIP_CALLSTATE_CALLSETUP;
        }
    } else {
        // This is actually a call
        if (reqresp == SIP_METHOD_INVITE) {
            call->invitecseq = msg->cseq;
            call->state = SIP_CALLSTATE_CALLSETUP;
        }
    }
}

const char *
call_get_attribute(const sip_call_t *call, enum sip_attr_id id, char *value)
{
    sip_msg_t *first, *last;

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
            break;
    }

    return strlen(value) ? value : NULL;
}

const char *
call_state_to_str(int state)
{
    switch (state) {
        case SIP_CALLSTATE_CALLSETUP:
            return "CALL SETUP";
        case SIP_CALLSTATE_INCALL:
            return "IN CALL";
        case SIP_CALLSTATE_CANCELLED:
            return "CANCELLED";
        case SIP_CALLSTATE_REJECTED:
            return "REJECTED";
        case SIP_CALLSTATE_BUSY:
            return "BUSY";
        case SIP_CALLSTATE_DIVERTED:
            return "DIVERTED";
        case SIP_CALLSTATE_COMPLETED:
            return "COMPLETED";
    }
    return "";
}

int
call_attr_compare(const sip_call_t *one, const sip_call_t *two, enum sip_attr_id id)
{
    char onevalue[256], twovalue[256];
    int oneintvalue, twointvalue;
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
call_add_xcall(sip_call_t *call, sip_call_t *xcall)
{
    if (!call || !xcall)
        return;

    // Mark this call as changed
    call->changed = true;
    // Add the xcall to the list
    g_sequence_append(call->xcalls, xcall);
}

rtp_stream_t *
call_find_stream(struct sip_call *call, Address src, Address dst)
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
call_find_stream_exact(struct sip_call *call, Address src, Address dst)
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
