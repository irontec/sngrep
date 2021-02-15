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

#include "sip_call.h"
#include "sip.h"
#include "setting.h"

sip_call_t *
call_create(char *callid, char *xcallid)
{
    sip_call_t *call;

    // Initialize a new call structure
    if (!(call = sng_malloc(sizeof(sip_call_t))))
        return NULL;

    // Create a vector to store call messages
    call->msgs = vector_create(2, 2);
    vector_set_destroyer(call->msgs, msg_destroyer);

    // Create an empty vector to store rtp packets
    if (setting_enabled(SETTING_CAPTURE_RTP)) {
        call->rtp_packets = vector_create(0, 40);
        vector_set_destroyer(call->rtp_packets, packet_destroyer);
    }

    // Create an empty vector to strore stream data
    call->streams = vector_create(0, 2);
    vector_set_destroyer(call->streams, vector_generic_destroyer);

    // Create an empty vector to store x-calls
    call->xcalls = vector_create(0, 1);

    // Create an empty vector to store mrcp_channelids 
    call->mrcp_channelids = vector_create(0, 1);

    // Initialize call filter status
    call->filtered = -1;

    // Set message callid
    call->callid = strdup(callid);
    call->xcallid = strdup(xcallid);

    return call;
}

void
call_destroy(sip_call_t *call)
{
    // Remove all call messages
    vector_destroy(call->msgs);
    // Remove all call streams
    vector_destroy(call->streams);
    // Remove all call rtp packets
    vector_destroy(call->rtp_packets);
    // Remove all xcalls
    vector_destroy(call->xcalls);

    // Remove all MRCP channel-identifiers
    char *channelid;
    vector_iter_t it = vector_iterator(call->mrcp_channelids);
    while ((channelid = vector_iterator_next(&it))) {
        sip_remove_mrcp_channelid(channelid);
        sng_free(channelid);
    }
    vector_destroy(call->mrcp_channelids);

    // Deallocate call memory
    sng_free(call->callid);
    sng_free(call->xcallid);
    sng_free(call->reasontxt);
    sng_free(call);
}

void
call_destroyer(void *call)
{
    call_destroy((sip_call_t*)call);
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
    msg->index = vector_append(call->msgs, msg);
    // Flag this call as changed
    call->changed = true;
}

void
call_add_stream(sip_call_t *call, rtp_stream_t *stream)
{
    // Store stream
    vector_append(call->streams, stream);
    // Flag this call as changed
    call->changed = true;
}

void
call_add_rtp_packet(sip_call_t *call, packet_t *packet)
{
    // Store packet
    vector_append(call->rtp_packets, packet);
    // Flag this call as changed
    call->changed = true;
}

int
call_msg_count(sip_call_t *call)
{
    return vector_count(call->msgs);
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
    if ((first = vector_first(call->msgs)))
        return (first->reqresp == SIP_METHOD_INVITE);

    return 0;
}

void
call_msg_retrans_check(sip_msg_t *msg)
{
    sip_msg_t *prev = NULL;
    vector_iter_t it;

    // Get previous message in call with same origin and destination
    it = vector_iterator(msg->call->msgs);
    vector_iterator_set_current(&it, vector_index(msg->call->msgs, msg));
    while ((prev = vector_iterator_prev(&it))) {
        if (addressport_equals(prev->packet->src, msg->packet->src) &&
                addressport_equals(prev->packet->dst, msg->packet->dst))
            break;
    }

    // Store the flag that determines if message is retrans
    if (prev && !strcasecmp(msg_get_payload(msg), msg_get_payload(prev))) {
        msg->retrans = prev;
    }
}

sip_msg_t *
call_msg_with_media(sip_call_t *call, address_t dst)
{
    sip_msg_t *msg;
    sdp_media_t *media;
    vector_iter_t itmsg;
    vector_iter_t itmedia;

    // Get message with media address configured in given dst
    itmsg = vector_iterator(call->msgs);
    while ((msg = vector_iterator_next(&itmsg))) {
        itmedia = vector_iterator(msg->medias);
        while ((media = vector_iterator_next(&itmedia))) {
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
            } else if (reqresp == 181 || reqresp == 302 || reqresp == 301) {
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
call_get_attribute(sip_call_t *call, enum sip_attr_id id, char *value)
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
            sprintf(value, "%d", vector_count(call->msgs));
            break;
        case SIP_ATTR_CALLSTATE:
            sprintf(value, "%s", call_state_to_str(call->state));
            break;
        case SIP_ATTR_TRANSPORT:
            first = vector_first(call->msgs);
            sprintf(value, "%s", sip_transport_str(first->packet->type));
            break;
        case SIP_ATTR_CONVDUR:
            timeval_to_duration(msg_get_time(call->cstart_msg), msg_get_time(call->cend_msg), value);
            break;
        case SIP_ATTR_TOTALDUR:
            first = vector_first(call->msgs);
            last = vector_last(call->msgs);
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
            return msg_get_attribute(vector_first(call->msgs), id, value);
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
call_attr_compare(sip_call_t *one, sip_call_t *two, enum sip_attr_id id)
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
    vector_append(call->xcalls, xcall);
}
