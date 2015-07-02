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
    int index;
    sip_call_t *call;

    // Initialize a new call structure
    if (!(call = malloc(sizeof(sip_call_t))))
        return NULL;
    memset(call, 0, sizeof(sip_call_t));

    // Store Call-Id
    call->callid = strdup(callid);

    // Create a vector to store call messages
    call->msgs = vector_create(10, 5);
    vector_set_destroyer(call->msgs, msg_destroyer);

    // Create a vector to store call attributes
    call->attrs = vector_create(1, 1);
    vector_set_destroyer(call->attrs, sip_attr_destroyer);

    // Create a vector to store RTP streams
    call->streams = vector_create(0, 2);
    vector_set_destroyer(call->streams, vector_generic_destroyer);

    // Initialize call filter status
    call->filtered = -1;
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
    // Free it!
    free(call->callid);
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
    vector_append(call->msgs, msg);
    // Store message count
    call_set_attribute(call, SIP_ATTR_MSGCNT, "%d", vector_count(call->msgs));
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

void
call_update_state(sip_call_t *call, sip_msg_t *msg)
{
    const char *callstate;
    char dur[20];
    int reqresp;
    sip_msg_t *first;

    // Sanity check
    if (!call || !call->msgs || !msg)
        return;

    // Get the first message in the call
    first = vector_first(call->msgs);

    // Check First message of Call has INVITE method
    if (first->reqresp != SIP_METHOD_INVITE) {
        return;
    }

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

