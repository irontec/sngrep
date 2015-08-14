/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2015 Ivan Alonso (Kaian)
 ** Copyright (C) 2015 Irontec SL. All rights reserved.
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

#include "sip_call.h"

sip_call_t *
call_create(char *callid)
{
    sip_call_t *call;

    // Initialize a new call structure
    if (!(call = malloc(sizeof(sip_call_t))))
        return NULL;
    memset(call, 0, sizeof(sip_call_t));

    // Create a vector to store call messages
    call->msgs = vector_create(2, 2);
    vector_set_destroyer(call->msgs, msg_destroyer);

    // Create a vector to store call attributes
    call->attrs = vector_create(1, 1);
    vector_set_destroyer(call->attrs, sip_attr_destroyer);

    // Initialize call filter status
    call->filtered = -1;

    // Set message callid
    call_set_attribute(call, SIP_ATTR_CALLID, callid);
    return call;
}

void
call_destroy(sip_call_t *call)
{
    // Remove all call messages
    vector_destroy(call->msgs);
    // Remove all call streams
    vector_destroy(call->streams);
    // Remove all call attributes
    vector_destroy(call->attrs);
    // Remove all call rtp packets
    vector_destroy(call->rtp_packets);
    // Deallocate call memory
    free(call);
}

void
call_destroyer(void *call)
{
    call_destroy((sip_call_t*)call);
}

void
call_add_message(sip_call_t *call, sip_msg_t *msg)
{
    // Set the message owner
    msg->call = call;
    // Put this msg at the end of the msg list
    msg->index = vector_append(call->msgs, msg) - 1;
    // Store message count
    call_set_attribute(call, SIP_ATTR_MSGCNT, "%d", vector_count(call->msgs));
}

void
call_add_stream(sip_call_t *call, rtp_stream_t *stream)
{
    if (!call->streams) {
        // Create a vector to store RTP streams
        call->streams = vector_create(2, 2);
        vector_set_destroyer(call->streams, vector_generic_destroyer);
    }
    vector_append(call->streams, stream);
}

void
call_add_rtp_packet(sip_call_t *call, capture_packet_t *packet)
{
    if (!call->rtp_packets) {
        // Create a vector to store RTP streams
        call->rtp_packets = vector_create(20, 40);
        vector_set_destroyer(call->rtp_packets, capture_packet_destroyer);
    }
    vector_append(call->rtp_packets, packet);
}

int
call_msg_count(sip_call_t *call)
{
    return vector_count(call->msgs);
}

int
call_is_active(void *item)
{
    // TODO
    sip_call_t *call = (sip_call_t *)item;
    return call->active;
}

int
call_is_invite(sip_call_t *call)
{
    sip_msg_t *first;
    if ((first = vector_first(call->msgs)))
        return (first->reqresp == SIP_METHOD_INVITE);

    return 0;
}

int
call_msg_is_retrans(sip_msg_t *msg)
{
    sip_msg_t *prev = NULL;
    vector_iter_t it;

    // Get previous message in call
    it = vector_iterator(msg->call->msgs);
    vector_iterator_set_current(&it, vector_index(msg->call->msgs, msg));
    prev = vector_iterator_prev(&it);

    return (prev && !strcasecmp(msg_get_payload(msg), msg_get_payload(prev)));
}


void
call_update_state(sip_call_t *call, sip_msg_t *msg)
{
    const char *callstate;
    char dur[20];
    int reqresp;
    sip_msg_t *first;

    if (!call_is_invite(call))
        return;

    // Get the first message in the call
    first = vector_first(call->msgs);

    // Get current message Method / Response Code
    reqresp = msg->reqresp;

    // If this message is actually a call, get its current state
    if ((callstate = call_get_attribute(call, SIP_ATTR_CALLSTATE))) {
        if (!strcmp(callstate, SIP_CALLSTATE_CALLSETUP)) {
            if (reqresp == 200) {
                // Alice and Bob are talking
                call_set_attribute(call, SIP_ATTR_CALLSTATE, SIP_CALLSTATE_INCALL);
                // Store the timestap where call has started
                call->active = 1;
                call->cstart_msg = msg;
            } else if (reqresp == SIP_METHOD_CANCEL) {
                // Alice is not in the mood
                call_set_attribute(call, SIP_ATTR_CALLSTATE, SIP_CALLSTATE_CANCELLED);
                // Store total call duration
                call_set_attribute(call, SIP_ATTR_TOTALDUR, timeval_to_duration(msg_get_time(first), msg_get_time(msg), dur));
                call->active = 0;
            } else if (reqresp > 400) {
                // Bob is not in the mood
                call_set_attribute(call, SIP_ATTR_CALLSTATE, SIP_CALLSTATE_REJECTED);
                // Store total call duration
                call_set_attribute(call, SIP_ATTR_TOTALDUR, timeval_to_duration(msg_get_time(first), msg_get_time(msg), dur));
                call->active = 0;
            }
        } else if (!strcmp(callstate, SIP_CALLSTATE_INCALL)) {
            if (reqresp == SIP_METHOD_BYE) {
                // Thanks for all the fish!
                call_set_attribute(call, SIP_ATTR_CALLSTATE, SIP_CALLSTATE_COMPLETED);
                // Store Conversation duration
                call_set_attribute(call, SIP_ATTR_CONVDUR,
                                   timeval_to_duration(msg_get_time(call->cstart_msg), msg_get_time(msg), dur));
                call->active = 0;
            }
        } else if (reqresp == SIP_METHOD_INVITE && strcmp(callstate, SIP_CALLSTATE_INCALL)) {
            // Call is being setup (after proper authentication)
            call_set_attribute(call, SIP_ATTR_CALLSTATE, SIP_CALLSTATE_CALLSETUP);
            call->active = 1;
        } else {
            // Store total call duration
            call_set_attribute(call, SIP_ATTR_TOTALDUR, timeval_to_duration(msg_get_time(first), msg_get_time(msg), dur));
        }
    } else {
        // This is actually a call
        if (reqresp == SIP_METHOD_INVITE) {
            call_set_attribute(call, SIP_ATTR_CALLSTATE, SIP_CALLSTATE_CALLSETUP);
            call->active = 1;
        }
    }
}
void
call_set_attribute(sip_call_t *call, enum sip_attr_id id, const char *fmt, ...)
{
    char value[512];

    // Get the actual value for the attribute
    va_list ap;
    va_start(ap, fmt);
    vsprintf(value, fmt, ap);
    va_end(ap);

    sip_attr_set(call->attrs, id, value);
}

const char *
call_get_attribute(sip_call_t *call, enum sip_attr_id id)
{
    if (!call)
        return NULL;

    switch (id) {
        case SIP_ATTR_CALLINDEX:
        case SIP_ATTR_CALLID:
        case SIP_ATTR_MSGCNT:
        case SIP_ATTR_CALLSTATE:
        case SIP_ATTR_CONVDUR:
        case SIP_ATTR_TOTALDUR:
            return sip_attr_get_value(call->attrs, id);
        default:
            return msg_get_attribute(vector_first(call->msgs), id);
    }

    return NULL;
}

