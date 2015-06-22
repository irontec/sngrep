/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013,2014 Ivan Alonso (Kaian)
 ** Copyright (C) 2013,2014 Irontec SL. All rights reserved.
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
 * @file sip.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in sip.h
 *
 * @todo Replace structures for their typedef shorter names
 */
#include "config.h"
#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <search.h>
#include <stdarg.h>
#include "sip.h"
#include "option.h"
#include "setting.h"
#include "capture.h"
#include "filter.h"

/**
 * @brief Linked list of parsed calls
 *
 * All parsed calls will be added to this list, only accesible from
 * this awesome structure, so, keep it thread-safe.
 */
sip_call_list_t calls =
    { 0 };

void
sip_init(int limit, int only_calls, int no_incomplete)
{
    int match_flags;

    // Store capture limit
    calls.limit = limit;
    calls.only_calls = only_calls;
    calls.ignore_incomplete = no_incomplete;

    // Create a vector to store calls
    calls.list = vector_create(500, 50);

    // Create hash table for callid search
    hcreate(calls.limit);

    // Initialize calls lock
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
#if defined(PTHREAD_MUTEX_RECURSIVE) || defined(__FreeBSD__) || defined(BSD) || defined (__OpenBSD__)
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#else
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
#endif
    pthread_mutex_init(&calls.lock, &attr);

    // Initialize payload parsing regexp
    match_flags = REG_EXTENDED | REG_ICASE | REG_NEWLINE;
    regcomp(&calls.reg_method, "^([a-zA-Z]+) sip:[^ ]+ SIP/2.0\r", match_flags & ~REG_NEWLINE);
    regcomp(&calls.reg_callid, "^(Call-ID|i):[ ]*([^ ]+)\r$", match_flags);
    regcomp(&calls.reg_xcallid, "^(X-Call-ID|X-CID):[ ]*([^ ]+)\r$", match_flags);
    regcomp(&calls.reg_response, "^SIP/2.0[ ]*(([0-9]{3}) [^\r]+)\r", match_flags & ~REG_NEWLINE);
    regcomp(&calls.reg_cseq, "^CSeq:[ ]*([0-9]+) .+\r$", match_flags);
    regcomp(&calls.reg_from, "^(From|f):[ ]*[^:]*:(([^@]+)@?[^\r>;]+)", match_flags);
    regcomp(&calls.reg_to, "^(To|t):[ ]*[^:]*:(([^@]+)@?[^\r>;]+)", match_flags);
    regcomp(&calls.reg_sdp, "^Content-Type:[ ]* application/sdp\r$", match_flags);
    regcomp(&calls.reg_sdp_addr, "^c=[^ ]+ [^ ]+ (.+)\r$", match_flags);
    regcomp(&calls.reg_sdp_port, "^m=[^ ]+ ([0-9]+)", match_flags);

}

sip_msg_t *
sip_msg_create(const char *payload)
{
    sip_msg_t *msg;

    if (!(msg = malloc(sizeof(sip_msg_t))))
        return NULL;
    memset(msg, 0, sizeof(sip_msg_t));
    msg->payload = strdup(payload);
    msg->color = 0;
    // Create a vector to store sdp
    msg->medias = vector_create(0, 2);
    return msg;
}

void
sip_msg_destroy(sip_msg_t *msg)
{

    if (!msg)
        return;

    // Free message attribute list
    sip_attr_list_destroy(msg->attrs);

    // Free message payload pointer if not parsed
    if (msg->payload)
        free(msg->payload);

    // Free packet data
    if (msg->pcap_header)
        free(msg->pcap_header);
    if (msg->pcap_packet)
        free(msg->pcap_packet);

    // Free all memory
    free(msg);
}

sip_call_t *
sip_call_create(char *callid)
{
    ENTRY entry;
    int index;

    // Initialize a new call structure
    sip_call_t *call = malloc(sizeof(sip_call_t));
    memset(call, 0, sizeof(sip_call_t));

    // Add this call to the call list
    pthread_mutex_lock(&calls.lock);
    index = vector_append(calls.list, call);
    pthread_mutex_unlock(&calls.lock);

    // Store this call in hash table
    entry.key = strdup(callid);
    entry.data = (void *) call;
    hsearch(entry, ENTER);

    // Create a vector to store call messages
    call->msgs = vector_create(10, 5);
    // Create a vector to store RTP streams
    call->streams = vector_create(4, 2);

    // Initialize call filter status
    call->filtered = -1;

    // Store current call Index
    call_set_attribute(call, SIP_ATTR_CALLINDEX, "%d", index);

    return call;
}

void
sip_call_destroy(sip_call_t *call)
{
    sip_msg_t *msg;
    vector_iter_t msgs;

    // No call to destroy
    if (!call)
        return;

    // Remove call from calls list
    vector_remove(calls.list, call);

    // Remove all call messages
    msgs = vector_iterator(call->msgs);
    while ((msg = vector_iterator_next(&msgs))) {
        sip_msg_destroy(msg);
        vector_remove(call->msgs, msg);
    }


    // Remove all call attributes
    sip_attr_list_destroy(call->attrs);

    // Free it!
    free(call);
}

char *
sip_get_callid(const char* payload)
{
    char *callid = NULL;
    regmatch_t pmatch[3];

    // Try to get Call-ID from payload
    if (regexec(&calls.reg_callid, payload, 3, pmatch, 0) == 0) {
        // Call-ID is the firts regexp match
        callid = malloc(pmatch[2].rm_eo - pmatch[2].rm_so + 1);
        // Allocate memory for Call-Id (caller MUST free it)
        memset(callid, 0, pmatch[2].rm_eo - pmatch[2].rm_so + 1);
        // Copy the matching part of payload
        strncpy(callid, payload + pmatch[2].rm_so, pmatch[2].rm_eo - pmatch[2].rm_so);
    }

    return callid;
}

sip_msg_t *
sip_load_message(const struct pcap_pkthdr *header, const char *src, u_short sport, const char* dst,
                 u_short dport, u_char *payload)
{
    sip_msg_t *msg;
    sip_call_t *call;
    char *callid;

    // Get the Call-ID of this message
    if (!(callid = sip_get_callid((const char*) payload))) {
        return NULL;
    }

    // Create a new message from this data
    if (!(msg = sip_msg_create((const char*) payload))) {
        return NULL;
    }

    // Get Method and request for the following checks
    // There is no need to parse all payload at this point
    // If no response or request code is found, this is not a SIP message
    if (!msg_get_reqresp(msg)) {
        // Deallocate message memory
        sip_msg_destroy(msg);
        return NULL;
    }

    // Store sorce address. Prefix too long IPv6 addresses with two dots
    if (strlen(src) > 15) {
        sprintf(msg->src, "..%s", src + strlen(src) - 13);
    } else {
        strcpy(msg->src, src);
    }

    // Store destination address. Prefix too long IPv6 addresses with two dots
    if (strlen(dst) > 15) {
        sprintf(msg->dst, "..%s", dst + strlen(dst) - 13);
    } else {
        strcpy(msg->dst, dst);
    }

    // Fill message data
    msg->sport = sport;
    msg->dport = dport;

    // Set message PCAP header
    msg->pcap_header = malloc(sizeof(struct pcap_pkthdr));
    memcpy(msg->pcap_header, header, sizeof(struct pcap_pkthdr));

    pthread_mutex_lock(&calls.lock);
    // Find the call for this msg
    if (!(call = call_find_by_callid(callid))) {

        // Check if payload matches expression
        if (!sip_check_match_expression((const char*) payload)) {
            // Deallocate message memory
            sip_msg_destroy(msg);
            pthread_mutex_unlock(&calls.lock);
            return NULL;
        }

        // User requested only INVITE starting dialogs
        if (calls.only_calls) {
            if (msg->reqresp != SIP_METHOD_INVITE) {
                // Deallocate message memory
                sip_msg_destroy(msg);
                pthread_mutex_unlock(&calls.lock);
                return NULL;
            }
        }

        // Only create a new call if the first msg
        // is a request message in the following gorup
        if (calls.ignore_incomplete) {
            if (msg->reqresp > SIP_METHOD_MESSAGE) {
                // Deallocate message memory
                sip_msg_destroy(msg);
                pthread_mutex_unlock(&calls.lock);
                return NULL;
            }
        }

        // Check if this message is ignored by configuration directive
        if (sip_check_msg_ignore(msg)) {
            // Deallocate message memory
            sip_msg_destroy(msg);
            pthread_mutex_unlock(&calls.lock);
            return NULL;
        }

        // Create the call if not found
        if (!(call = sip_call_create(callid))) {
            // Deallocate message memory
            sip_msg_destroy(msg);
            pthread_mutex_unlock(&calls.lock);
            return NULL;
        }
    }

    // Set message callid
    msg_set_attribute(msg, SIP_ATTR_CALLID, callid);
    // Dellocate callid memory
    free(callid);

    // Add the message to the found/created call
    call_add_message(call, msg);

    msg_parse_payload(msg, msg->payload);
    // Parse media data
    msg_parse_media(msg);

    // Update Call State
    call_update_state(call, msg);

    pthread_mutex_unlock(&calls.lock);

    // Return the loaded message
    return msg;
}

int
sip_calls_count()
{
    return vector_count(calls.list);
}

vector_iter_t
sip_calls_iterator()
{
    return vector_iterator(calls.list);
}

void
sip_calls_stats(int *total, int *displayed)
{
    vector_iter_t it = vector_iterator(calls.list);

    pthread_mutex_lock(&calls.lock);
    // Total number of calls without filtering
    *total = vector_iterator_count(&it);
    // Total number of calls after filtering
    vector_iterator_set_filter(&it, filter_check_call);
    *displayed = vector_iterator_count(&it);
    pthread_mutex_unlock(&calls.lock);
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

sip_call_t *
call_find_by_callid(const char *callid)
{
    ENTRY entry, *eptr;

    entry.key = (char *) callid;
    if ((eptr = hsearch(entry, FIND)))
        return eptr->data;

    return NULL;
}

sip_call_t *
call_find_by_xcallid(const char *xcallid)
{
    const char *cur_xcallid;
    sip_call_t *cur;
    int i;

    for (i=0; i < vector_count(calls.list); i++) {
        cur = vector_item(calls.list, i);
        cur_xcallid = call_get_attribute(cur, SIP_ATTR_XCALLID);
        if (cur_xcallid && !strcmp(cur_xcallid, xcallid)) {
            return cur;
        }
    }
    return NULL;
}

int
call_msg_count(sip_call_t *call)
{
    return vector_count(call->msgs);
}

int
msg_media_count(sip_msg_t *msg)
{
    return vector_count(msg->medias);
}

sip_call_t *
call_get_xcall(sip_call_t *call)
{
    sip_call_t *xcall;
    pthread_mutex_lock(&calls.lock);
    if (call_get_attribute(call, SIP_ATTR_XCALLID)) {
        xcall = call_find_by_callid(call_get_attribute(call, SIP_ATTR_XCALLID));
    } else {
        xcall = call_find_by_xcallid(call_get_attribute(call, SIP_ATTR_CALLID));
    }
    pthread_mutex_unlock(&calls.lock);
    return xcall;
}

sip_msg_t *
call_get_next_msg(sip_call_t *call, sip_msg_t *msg)
{
    sip_msg_t *ret;
    pthread_mutex_lock(&calls.lock);
    if (msg == NULL) {
        ret = vector_first(call->msgs);
    } else {
        ret = vector_item(call->msgs, vector_index(call->msgs, msg) + 1);
    }

    // Parse message if not parsed
    if (ret && !ret->parsed)
        msg_parse_payload(ret, ret->payload);

    pthread_mutex_unlock(&calls.lock);
    return ret;
}

sip_msg_t *
call_get_prev_msg(sip_call_t *call, sip_msg_t *msg)
{
    sip_msg_t *ret = NULL;
    pthread_mutex_lock(&calls.lock);
    ret = vector_item(call->msgs, vector_index(call->msgs, msg) - 1);

    // Parse message if not parsed
    if (ret && !ret->parsed)
        msg_parse_payload(ret, ret->payload);

    pthread_mutex_unlock(&calls.lock);
    return ret;
}

int
call_is_active(void *item)
{
    // TODO
    sip_call_t *call = (sip_call_t *)item;
    return call->active;
}

int
msg_has_sdp(void *item)
{
    // TODO
    sip_msg_t *msg = (sip_msg_t *)item;
    return msg->sdp;
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
    reqresp = msg_get_reqresp(msg);

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
                call_set_attribute(call, SIP_ATTR_TOTALDUR, sip_calculate_duration(first, msg, dur));
                call->active = 0;
            } else if (reqresp > 400) {
                // Bob is not in the mood
                call_set_attribute(call, SIP_ATTR_CALLSTATE, SIP_CALLSTATE_REJECTED);
                // Store total call duration
                call_set_attribute(call, SIP_ATTR_TOTALDUR, sip_calculate_duration(first, msg, dur));
                call->active = 0;
            }
        } else if (!strcmp(callstate, SIP_CALLSTATE_INCALL)) {
            if (reqresp == SIP_METHOD_BYE) {
                // Thanks for all the fish!
                call_set_attribute(call, SIP_ATTR_CALLSTATE, SIP_CALLSTATE_COMPLETED);
                // Store Conversation duration
                call_set_attribute(call, SIP_ATTR_CONVDUR,
                                   sip_calculate_duration(call->cstart_msg, msg, dur));
                call->active = 0;
            }
        } else if (reqresp == SIP_METHOD_INVITE && strcmp(callstate, SIP_CALLSTATE_INCALL)) {
            // Call is being setup (after proper authentication)
            call_set_attribute(call, SIP_ATTR_CALLSTATE, SIP_CALLSTATE_CALLSETUP);
            call->active = 1;
        } else {
            // Store total call duration
            call_set_attribute(call, SIP_ATTR_TOTALDUR, sip_calculate_duration(first, msg, dur));
        }
    } else {
        // This is actually a call
        if (reqresp == SIP_METHOD_INVITE) {
            call_set_attribute(call, SIP_ATTR_CALLSTATE, SIP_CALLSTATE_CALLSETUP);
            call->active = 1;
        }
    }
}

rtp_stream_t *
call_find_stream(sip_call_t *call, const char *ip_src, u_short sport, const char *ip_dst, u_short dport)
{
    rtp_stream_t *stream, *ret = NULL;
    vector_iter_t it;

    it = vector_iterator(call->streams);

    while ((stream = vector_iterator_next(&it))) {
        if (!strcmp(ip_src, stream->ip_src) && sport == stream->sport &&
            !strcmp(ip_dst, stream->ip_dst) && dport == stream->dport) {
            ret = stream;
        }
    }
    return ret;
}

int
msg_get_reqresp(sip_msg_t *msg)
{
    regmatch_t pmatch[3];
    char reqresp[20];
    const char *payload = msg->payload;

    // If not already parsed
    if (!msg->reqresp) {
        // Initialize variables
        memset(reqresp, 0, sizeof(reqresp));

        // Method & CSeq
        if (regexec(&calls.reg_method, payload, 2, pmatch, 0) == 0) {
            sprintf(reqresp, "%.*s", pmatch[1].rm_eo - pmatch[1].rm_so, payload + pmatch[1].rm_so);
            msg_set_attribute(msg, SIP_ATTR_METHOD, reqresp);
        }

        // Response code
        if (regexec(&calls.reg_response, payload, 3, pmatch, 0) == 0) {
            msg_set_attribute(msg, SIP_ATTR_METHOD, "%.*s", pmatch[1].rm_eo - pmatch[1].rm_so,
                              payload + pmatch[1].rm_so);
            sprintf(reqresp, "%.*s", pmatch[2].rm_eo - pmatch[2].rm_so, payload + pmatch[2].rm_so);
        }

        // Get Request/Response Code
        msg->reqresp = sip_method_from_str(reqresp);
    }

    return msg->reqresp;
}

int
msg_parse_payload(sip_msg_t *msg, const char *payload)
{
    regmatch_t pmatch[4];
    char date[12], time[20], cseq[11];

    // If message has already been parsed, we've finished
    if (msg->parsed)
        return 0;

    // Get Method and request for the following checks
    msg_get_reqresp(msg);

    // CSeq
    if (regexec(&calls.reg_cseq, payload, 2, pmatch, 0) == 0) {
        sprintf(cseq, "%.*s", pmatch[1].rm_eo - pmatch[1].rm_so, payload + pmatch[1].rm_so);
        msg->cseq = atoi(cseq);
    }

    // X-Call-Id
    if (regexec(&calls.reg_xcallid, payload, 3, pmatch, 0) == 0) {
        msg_set_attribute(msg, SIP_ATTR_XCALLID, "%.*s", pmatch[2].rm_eo - pmatch[2].rm_so,
                          payload + pmatch[2].rm_so);
    }

    // From
    if (regexec(&calls.reg_from, payload, 4, pmatch, 0) == 0) {
        msg_set_attribute(msg, SIP_ATTR_SIPFROM, "%.*s", pmatch[2].rm_eo - pmatch[2].rm_so,
                          payload + pmatch[2].rm_so);
        msg_set_attribute(msg, SIP_ATTR_SIPFROMUSER, "%.*s", pmatch[3].rm_eo - pmatch[3].rm_so,
                          payload + pmatch[3].rm_so);
    }

    // To
    if (regexec(&calls.reg_to, payload, 4, pmatch, 0) == 0) {
        msg_set_attribute(msg, SIP_ATTR_SIPTO, "%.*s", pmatch[2].rm_eo - pmatch[2].rm_so,
                          payload + pmatch[2].rm_so);
        msg_set_attribute(msg, SIP_ATTR_SIPTOUSER, "%.*s", pmatch[3].rm_eo - pmatch[3].rm_so,
                          payload + pmatch[3].rm_so);
    }

    // Set Source and Destination attributes
    msg_set_attribute(msg, SIP_ATTR_SRC, "%s:%u", msg->src, msg->sport);
    msg_set_attribute(msg, SIP_ATTR_DST, "%s:%u", msg->dst, msg->dport);

    // Set message Date and Time attribute
    msg_set_attribute(msg, SIP_ATTR_DATE, timeval_to_date(msg->pcap_header->ts, date));
    msg_set_attribute(msg, SIP_ATTR_TIME, timeval_to_time(msg->pcap_header->ts, time));

    // Message payload has been parsed
    msg->parsed = 1;

    return 0;
}

void
msg_parse_media(sip_msg_t *msg)
{
    regmatch_t pmatch[4];
    char address[ADDRESSLEN];
    char media_address[ADDRESSLEN] = { };
    char media_type[15] = { };
    char media_format[30] = { };
    int media_port;
    int media_fmt_pref;
    int media_fmt_code;
    sdp_media_t *media = NULL;
    rtp_stream_t *stream, *active;
    int port = 0;
    char *payload, *tofree, *line;

    // Check if this message has sdp
    if (regexec(&calls.reg_sdp, msg->payload, 0, 0, 0) != 0)
        return;

    // Initialize variables
    memset(address, 0, sizeof(address));

    // SDP Address
    if (regexec(&calls.reg_sdp_addr, msg->payload, 2, pmatch, 0) == 0) {
        sprintf(address, "%.*s", pmatch[1].rm_eo - pmatch[1].rm_so, msg->payload + pmatch[1].rm_so);
        msg_set_attribute(msg, SIP_ATTR_SDP_ADDRESS, address);
    }

    // SDP Port
    if (regexec(&calls.reg_sdp_port, msg->payload, 2, pmatch, 0) == 0) {
        msg_set_attribute(msg, SIP_ATTR_SDP_PORT, "%.*s", pmatch[1].rm_eo - pmatch[1].rm_so,
                          msg->payload + pmatch[1].rm_so);
        port = atoi(msg_get_attribute(msg, SIP_ATTR_SDP_PORT));
    }

    if (!strlen(address) || !port)
        return;

    // Message has SDP
    msg->sdp = 1;

    // Parse each line of payload looking for sdp information
    tofree = payload = strdup(msg->payload);
    while ((line = strsep(&payload, "\r\n")) != NULL) {
        // Check if we have a media string
        if (!strncmp(line, "m=", 2)) {
            if (sscanf(line, "m=%s %d RTP/AVP %d", media_type, &media_port, &media_fmt_pref) == 3) {
                // Create a new media structure for this message
                if ((media = media_create(msg))) {
                    media_set_type(media, media_type);
                    media_set_port(media, media_port);
                    media_set_address(media, media_address);
                    media_set_format_code(media, media_fmt_pref);
                    vector_append(msg->medias, media);

                    if (!msg_is_retrans(msg)) {
                        // Create a new stream with this dest address:port
                        stream = stream_create(media);
                        strcpy(stream->ip_dst, media_address);
                        stream->dport = media_port;
                        vector_append(msg->call->streams, stream);

                        vector_iter_t it = vector_iterator(msg->call->streams);

                        // If there is a pending stream to confirm
                        while ((active = vector_iterator_next(&it))) {
                            // Only consider confirmation of other stream with different message type
                            if (msg_is_request(active->media->msg) == msg_is_request(msg))
                                continue;
                            // Only consider confirmation of stream in same transaction
                            if (active->media->msg->cseq != msg->cseq)
                                continue;
                            // Created from the other side of this dialog
                            if (strcmp(msg->dst, active->media->msg->src))
                                continue;
                            // That is not still completed
                            if (active->complete)
                                continue;

                            // Complete current stream
                            strcpy(stream->ip_src, active->ip_dst);
                            stream->sport = active->dport;
                            stream->complete = 1;

                            // Complete pending stream
                            strcpy(active->ip_src, stream->ip_dst);
                            active->sport = stream->dport;
                            active->complete = 1;
                            break;
                        }

                        // If no stream to complete has been found to complete
                        if (!active) {
                            vector_iterator_reset(&it);
                            // Check if there was an active stream previously configured
                            while ((active = vector_iterator_next(&it))) {
                                // We're looking for a confirmed stream
                                if (!active->complete)
                                    continue;
                                // That was confirmed from us
                                if (strcmp(msg->dst, active->media->msg->dst))
                                    continue;
                                // Assume that source address, but not mark this as completed
                                strcpy(stream->ip_src, active->ip_src);
                                stream->sport = active->sport;
                            }
                        }
                    }
                }
            }
        }

        // Check if we have a connection string
        if (!strncmp(line, "c=", 2)) {
            if (sscanf(line, "c=IN IP4 %s", media_address) && media) {
                    media_set_address(media, media_address);
            }
        }

        // Check if we have attribute format string
        if (!strncmp(line, "a=rtpmap:", 9)) {
            if (sscanf(line, "a=rtpmap:%d %[^ ]", &media_fmt_code, media_format)) {
                if (media && media_fmt_pref == media_fmt_code) {
                    media_set_format(media, media_format);
                }
            }
        }
    }
    free(tofree);
}

int
msg_is_retrans(sip_msg_t *msg)
{
    sip_msg_t *prev = NULL;
    vector_iter_t it;

    // Sanity check
    if (!msg || !msg->call || !msg->payload)
        return 0;

    // Get previous message in call
    it = vector_iterator(msg->call->msgs);
    vector_iterator_set_current(&it, vector_index(msg->call->msgs, msg));
    prev = vector_iterator_prev(&it);

    return (prev && !strcasecmp(msg->payload, prev->payload));
}

int
msg_is_request(sip_msg_t *msg)
{
    return msg->reqresp < SIP_METHOD_SENTINEL;
}

char *
msg_get_header(sip_msg_t *msg, char *out)
{
    // Source and Destination address
    char from_addr[80], to_addr[80];

    // We dont use Message attributes here because it contains truncated data
    // This should not overload too much as all results should be already cached
    sprintf(from_addr, "%s:%u", sip_address_format(msg->src), msg->sport);
    sprintf(to_addr, "%s:%u", sip_address_format(msg->dst), msg->dport);

    // Get msg header
    sprintf(out, "%s %s %s -> %s", DATE(msg), TIME(msg), from_addr, to_addr);
    return out;
}

const char *
msg_get_time_delta(sip_msg_t *one, sip_msg_t *two, char *out)
{
    long diff;
    int nsec, nusec;
    int sign;

    if (!one || !two || !out)
        return NULL;

    diff = two->pcap_header->ts.tv_sec  * 1000000 + two->pcap_header->ts.tv_usec;
    diff -= one->pcap_header->ts.tv_sec * 1000000 + one->pcap_header->ts.tv_usec;

    nsec = diff / 1000000;
    nusec = abs(diff - (nsec * 1000000));

    sign = (diff >= 0) ? '+' : '-';

    sprintf(out, "%c%d.%06d", sign, abs(nsec), nusec);
    return out;
}

void
sip_calls_clear()
{
    pthread_mutex_lock(&calls.lock);
    // Clear all elements in vector
    vector_clear(calls.list);
    // Create again the callid hash table
    hdestroy();
    hcreate(calls.limit);

    pthread_mutex_unlock(&calls.lock);
}

const char *
sip_calculate_duration(const sip_msg_t *start, const sip_msg_t *end, char *dur)
{
    int seconds;
    char duration[20];
    // Differnce in secons
    seconds = end->pcap_header->ts.tv_sec - start->pcap_header->ts.tv_sec;
    // Set Human readable format
    sprintf(duration, "%d:%02d", seconds / 60, seconds % 60);
    sprintf(dur, "%7s", duration);
    return dur;
}

int
sip_set_match_expression(const char *expr, int insensitive, int invert)
{
    // Store expression text
    calls.match_expr = expr;
    // Set invert flag
    calls.match_invert = invert;

#ifdef WITH_PCRE
    const char *re_err = NULL;
    int32_t err_offset;
    int32_t pflags = PCRE_UNGREEDY;

    if (insensitive)
        pflags |= PCRE_CASELESS;

    // Check if we have a valid expression
    calls.match_regex = pcre_compile(expr, pflags, &re_err, &err_offset, 0);
    return calls.match_regex == NULL;
#else
    int cflags = REG_EXTENDED;

    // Case insensitive requested
    if (insensitive)
    cflags |= REG_ICASE;

    // Check the expresion is a compilable regexp
    return regcomp(&calls.match_regex, expr, cflags) != 0;
#endif
}

int
sip_check_match_expression(const char *payload)
{
    // Everything matches when there is no match
    if (!calls.match_expr)
        return 1;

#ifdef WITH_PCRE
    switch (pcre_exec(calls.match_regex, 0, payload, strlen(payload), 0, 0, 0, 0)) {
        case PCRE_ERROR_NOMATCH:
            return 1 == calls.match_invert;
    }

    return 0 == calls.match_invert;
#else
    // Check if payload matches the given expresion
    return (regexec(&calls.match_regex, payload, 0, NULL, 0) == calls.match_invert);
#endif
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
            return sip_attr_get(call->attrs, id);
        default:
            return msg_get_attribute(vector_first(call->msgs), id);
    }

    return NULL;
}

void
msg_set_attribute(sip_msg_t *msg, enum sip_attr_id id, const char *fmt, ...)
{
    char value[512];

    // Get the actual value for the attribute
    va_list ap;
    va_start(ap, fmt);
    vsprintf(value, fmt, ap);
    va_end(ap);

    sip_attr_set(msg->attrs, id, value);
}

const char *
msg_get_attribute(sip_msg_t *msg, enum sip_attr_id id)
{
    if (!msg)
        return NULL;

    return sip_attr_get(msg->attrs, id);
}

int
sip_check_msg_ignore(sip_msg_t *msg)
{
    int i;
    sip_attr_hdr_t *header;

    // Check if an ignore option exists
    for (i = 0; i < SIP_ATTR_COUNT; i++) {
        header = sip_attr_get_header(i);
        if (is_ignored_value(header->name, msg_get_attribute(msg, header->id))) {
            return 1;
        }
    }
    return 0;
}

const char *
sip_method_str(enum sip_methods method)
{
    switch (method) {
        case SIP_METHOD_REGISTER:
            return "REGISTER";
        case SIP_METHOD_INVITE:
            return "INVITE";
        case SIP_METHOD_SUBSCRIBE:
            return "SUBSCRIBE";
        case SIP_METHOD_NOTIFY:
            return "NOTIFY";
        case SIP_METHOD_OPTIONS:
            return "OPTIONS";
        case SIP_METHOD_PUBLISH:
            return "PUBLISH";
        case SIP_METHOD_MESSAGE:
            return "MESSAGE";
        case SIP_METHOD_CANCEL:
            return "CANCEL";
        case SIP_METHOD_BYE:
            return "BYE";
        case SIP_METHOD_ACK:
            return "ACK";
        case SIP_METHOD_PRACK:
            return "PRACK";
        case SIP_METHOD_INFO:
            return "INFO";
        case SIP_METHOD_REFER:
            return "REFER";
        case SIP_METHOD_UPDATE:
            return "UPDATE";
        case SIP_METHOD_SENTINEL:
            return "";
    }
    return NULL;
}

int
sip_method_from_str(const char *method)
{
    int i;
    for (i = 1; i < SIP_METHOD_SENTINEL; i++)
        if (!strcmp(method, sip_method_str(i)))
            return i;
    return atoi(method);
}

const char *
sip_address_format(const char *address)
{

    // Return address formatted depending on active settings
    if (setting_enabled(SETTING_DISPLAY_ALIAS)) {
        return get_alias_value(address);
    } else if (setting_enabled(SETTING_DISPLAY_ALIAS)) {
        return lookup_hostname(address);
    } else {
        return address;
    }
}

const char *
sip_address_port_format(const char *addrport)
{
    static char aport[50];
    char address[50];
    int port;

    strncpy(aport, addrport, 50);
    if (sscanf(aport, "%[^:]:%d", address, &port) == 2) {
        sprintf(aport, "%s:%d", sip_address_format(address), port);
    }

    return aport;
}

const char *
timeval_to_date(struct timeval time, char *out)
{
    time_t t = (time_t) time.tv_sec;
    struct tm *timestamp = localtime(&t);
    strftime(out, 11, "%Y/%m/%d", timestamp);
    return out;
}


const char *
timeval_to_time(struct timeval time, char *out)
{
    time_t t = (time_t) time.tv_sec;
    struct tm *timestamp = localtime(&t);
    strftime(out, 11, "%H:%M:%S", timestamp);
    sprintf(out + 8, ".%06d", (int) time.tv_usec);
    return out;
}
