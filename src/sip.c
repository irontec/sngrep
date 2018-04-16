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
 * @file sip.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in sip.h
 */
#include "config.h"
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include "glib-utils.h"
#include "packet/dissectors/packet_sip.h"
#include "sip.h"
#include "option.h"
#include "setting.h"
#include "filter.h"

/**
 * @brief Linked list of parsed calls
 *
 * All parsed calls will be added to this list, only accesible from
 * this awesome structure, so, keep it thread-safe.
 */
sip_call_list_t calls =
{ 0 };

gboolean
sip_init(SStorageCaptureOpts capture_options,
         SStorageMatchOpts match_options,
         SStorageSortOpts sort_options,
         GError **error)
{
    GRegexCompileFlags cflags = G_REGEX_EXTENDED;
    GRegexMatchFlags mflags = G_REGEX_MATCH_NEWLINE_CRLF;

    calls.capture = capture_options;
    calls.match = match_options;
    calls.sort = sort_options;

    // Store capture limit
    calls.last_index = 0;

    // Validate match expression
    if (calls.match.mexpr) {
        // Case insensitive requested
        if (calls.match.micase) {
            cflags |= G_REGEX_CASELESS;
        }

        // Check the expresion is a compilable regexp
        calls.match.mregex = g_regex_new(calls.match.mexpr, cflags, 0, error);
        if (calls.match.mregex == NULL) {
            return FALSE;
        }
    }

    // Create a vector to store calls
    calls.list = g_sequence_new(call_destroy);
    calls.active = g_sequence_new(NULL);

    // Create hash table for callid search
    calls.callids = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    // Set default sorting field
    if (sip_attr_from_name(setting_get_value(SETTING_CL_SORTFIELD)) >= 0) {
        calls.sort.by = sip_attr_from_name(setting_get_value(SETTING_CL_SORTFIELD));
        calls.sort.asc = (!strcmp(setting_get_value(SETTING_CL_SORTORDER), "asc"));
    } else {
        // Fallback to default sorting field
        calls.sort.by = SIP_ATTR_CALLINDEX;
        calls.sort.asc = true;
    }


    return TRUE;
}

void
sip_deinit()
{
    // Remove all calls
    sip_calls_clear();
    // Remove Call-id hash table
    g_hash_table_destroy(calls.callids);
    // Remove calls vector
    g_sequence_free(calls.list);
    g_sequence_free(calls.active);

}

SStorageCaptureOpts
storage_capture_options()
{
    return calls.capture;
}

sip_msg_t *
sip_check_packet(Packet *packet)
{
    sip_msg_t *msg;
    sip_call_t *call;
    bool newcall = false;

    PacketSipData *sip_data = g_ptr_array_index(packet->proto, PACKET_SIP);

    // Create a new message from this data
    msg = msg_create();
    msg->cseq       = sip_data->cseq;
    msg->sip_from   = sip_data->from;
    msg->sip_to     = sip_data->to;
    msg->reqresp    = sip_data->reqresp;
    msg->resp_str   = sip_data->resp_str;

    // Find the call for this msg
    if (!(call = sip_find_by_callid(sip_data->callid))) {

        // Check if payload matches expression
        if (!sip_check_match_expression(sip_data->payload))
            goto skip_message;

        // User requested only INVITE starting dialogs
        if (calls.match.invite && msg->reqresp != SIP_METHOD_INVITE)
            goto skip_message;

        // Only create a new call if the first msg
        // is a request message in the following gorup
        if (calls.match.complete && msg->reqresp > SIP_METHOD_MESSAGE)
            goto skip_message;

        // Rotate call list if limit has been reached
        if (calls.capture.limit == sip_calls_count())
            sip_calls_rotate();

        // Create the call if not found
        if (!(call = call_create(sip_data->callid, sip_data->xcallid)))
            goto skip_message;

        // Add this Call-Id to hash table
        g_hash_table_insert(calls.callids, call->callid, call);

        // Set call index
        call->index = ++calls.last_index;

        // Mark this as a new call
        newcall = true;
    }

    // At this point we know we're handling an interesting SIP Packet
    msg->packet = packet_to_oldpkt(packet);

    // Always dissect first call message
    if (call_msg_count(call) == 0) {
        // If this call has X-Call-Id, append it to the parent call
        if (strlen(call->xcallid)) {
            call_add_xcall(sip_find_by_callid(call->xcallid), call);
        }
    }

    // Add the message to the call
    call_add_message(call, msg);

    // check if message is a retransmission
    call_msg_retrans_check(msg);

    if (call_is_invite(call)) {
        // Parse media data
        sip_parse_msg_media(msg, sip_data->payload);
        // Update Call State
        call_update_state(call, msg);
        // Check if this call should be in active call list
        if (call_is_active(call)) {
            if (sip_call_is_active(call)) {
                g_sequence_append(calls.active, call);
            }
        } else {
            if (sip_call_is_active(call)) {
                g_sequence_remove_data(calls.active, call);
            }
        }
    }

    if (newcall) {
        // Append this call to the call list
        g_sequence_insert_sorted(calls.list, call, sip_list_sorter, NULL);
    }

    // Mark the list as changed
    calls.changed = true;

    // Return the loaded message
    return msg;

skip_message:
    // Deallocate message memory
    msg_destroy(msg);
    return NULL;

}

bool
sip_calls_has_changed()
{
    bool changed = calls.changed;
    calls.changed = false;
    return changed;
}

int
sip_calls_count()
{
    return g_sequence_get_length(calls.list);
}

GSequenceIter *
sip_calls_iterator()
{
    return g_sequence_get_begin_iter(calls.list);
}

GSequenceIter *
sip_active_calls_iterator()
{
    return g_sequence_get_begin_iter(calls.active);
}

bool
sip_call_is_active(sip_call_t *call)
{
    return g_sequence_index(calls.active, call) != -1;
}

GSequence *
sip_calls_vector()
{
    return calls.list;
}

GSequence *
sip_active_calls_vector()
{
    return calls.active;
}

sip_stats_t
sip_calls_stats()
{
    sip_stats_t stats = {};
    GSequenceIter *it = g_sequence_get_begin_iter(calls.list);

    // Total number of calls without filtering
    stats.total = g_sequence_iter_length(it);
    // Total number of calls after filtering
    for (;!g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        if (filter_check_call(g_sequence_get(it), NULL))
            stats.displayed++;
    }
    return stats;
}

sip_call_t *
sip_find_by_index(int index)
{
    return g_sequence_nth(calls.list, index);
}

sip_call_t *
sip_find_by_callid(const char *callid)
{
    return g_hash_table_lookup(calls.callids, callid);
}

const char *
sip_get_msg_reqresp_str(sip_msg_t *msg)
{
    // Check if code has non-standard text
    if (msg->resp_str) {
        return msg->resp_str;
    } else {
        return sip_method_str(msg->reqresp);
    }
}

void
sip_parse_msg_media(sip_msg_t *msg, const u_char *payload)
{

#define ADD_STREAM(stream) \
    if (stream) { \
        if (!rtp_find_call_stream(call, src, stream->dst)) { \
          call_add_stream(call, stream); \
      } else { \
          sng_free(stream); \
          stream = NULL; \
      } \
    }

    Address dst, src = { };
    rtp_stream_t *rtp_stream = NULL, *rtcp_stream = NULL, *msg_rtp_stream = NULL;
    char media_type[MEDIATYPELEN] = { };
    char media_format[30] = { };
    uint32_t media_fmt_pref;
    uint32_t media_fmt_code;
    sdp_media_t *media = NULL;
    char *payload2, *tofree, *line;
    sip_call_t *call = msg_get_call(msg);

    // Parse each line of payload looking for sdp information
    tofree = payload2 = strdup((char*)payload);
    while ((line = strsep(&payload2, "\r\n")) != NULL) {
        // Check if we have a media string
        if (!strncmp(line, "m=", 2)) {
            if (sscanf(line, "m=%s %hu RTP/%*s %u", media_type, &dst.port, &media_fmt_pref) == 3) {

                // Add streams from previous 'm=' line to the call
                ADD_STREAM(msg_rtp_stream);
                ADD_STREAM(rtp_stream);
                ADD_STREAM(rtcp_stream);

                // Create a new media structure for this message
                if ((media = media_create(msg))) {
                    media_set_type(media, media_type);
                    media_set_address(media, dst);
                    media_set_prefered_format(media, media_fmt_pref);
                    msg_add_media(msg, media);

                    /**
                     * From SDP we can only guess destination address port. RTP Capture proccess
                     * will determine when the stream has been completed, getting source address
                     * and port of the stream.
                     */
                    // Create a new stream with this destination address:port

                    // Create RTP stream with source of message as destination address
                    msg_rtp_stream = stream_create(media, dst, PACKET_RTP);
                    msg_rtp_stream->dst = msg->packet->src;
                    msg_rtp_stream->dst.port = dst.port;

                    // Create RTP stream
                    rtp_stream = stream_create(media, dst, PACKET_RTP);

                    // Create RTCP stream
                    rtcp_stream = stream_create(media, dst, PACKET_RTCP);
                    rtcp_stream->dst.port++;
                }
            }
        }

        // Check if we have a connection string
        if (!strncmp(line, "c=", 2)) {
            if (sscanf(line, "c=IN IP4 %s", dst.ip) && media) {
                media_set_address(media, dst);
                strcpy(rtp_stream->dst.ip, dst.ip);
                strcpy(rtcp_stream->dst.ip, dst.ip);
            }
        }

        // Check if we have attribute format string
        if (!strncmp(line, "a=rtpmap:", 9)) {
            if (media && sscanf(line, "a=rtpmap:%u %[^ ]", &media_fmt_code, media_format)) {
                media_add_format(media, media_fmt_code, media_format);
            }
        }

        // Check if we have attribute format RTCP port
        if (!strncmp(line, "a=rtcp:", 7) && rtcp_stream) {
            sscanf(line, "a=rtcp:%hu", &rtcp_stream->dst.port);
        }


    }

    // Add streams from last 'm=' line to the call
    ADD_STREAM(msg_rtp_stream);
    ADD_STREAM(rtp_stream);
    ADD_STREAM(rtcp_stream);

    sng_free(tofree);

#undef ADD_STREAM
}



void
sip_calls_clear()
{
    // Create again the callid hash table
    g_hash_table_remove_all(calls.callids);

    // Remove all items from vector
    g_sequence_remove_all(calls.list);
    g_sequence_remove_all(calls.active);
}

void
sip_calls_clear_soft()
{
    // Create again the callid hash table
    g_hash_table_remove_all(calls.callids);

    // Repopulate list applying current filter
    calls.list = g_sequence_copy(sip_calls_vector(), filter_check_call, NULL);
    calls.active = g_sequence_copy(sip_active_calls_vector(), filter_check_call, NULL);

    // Repopulate callids based on filtered list
    sip_call_t *call;
    GSequenceIter *it = g_sequence_get_begin_iter(calls.list);

    for (;!g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        call = g_sequence_get(it);
        g_hash_table_insert(calls.callids, call->callid, call);
    }
}

void
sip_calls_rotate()
{
    sip_call_t *call;
    GSequenceIter *it = g_sequence_get_begin_iter(calls.list);
    for (;!g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        call = g_sequence_get(it);
        if (!call->locked) {
            // Remove from callids hash
            g_hash_table_remove(calls.callids, call->callid);
            // Remove first call from active and call lists
            g_sequence_remove_data(calls.active, call);
            g_sequence_remove_data(calls.list, call);
            return;
        }
    }
}

const char *
sip_get_match_expression()
{
    return calls.match.mexpr;
}

int
sip_check_match_expression(const char *payload)
{
    // Everything matches when there is no match
    if (calls.match.mexpr == NULL)
        return 1;

    // Check if payload matches the given expresion
    if (g_regex_match(calls.match.mregex, payload, 0, NULL)) {
        return 0 == calls.match.minvert;
    } else {
        return 1 == calls.match.minvert;
    }

}



const char *
sip_transport_str(int transport)
{
    switch(transport)
    {
        case PACKET_OLD_SIP_UDP:
            return "UDP";
        case PACKET_OLD_SIP_TCP:
            return "TCP";
        case PACKET_OLD_SIP_TLS:
            return "TLS";
        case PACKET_OLD_SIP_WS:
            return "WS";
        case PACKET_OLD_SIP_WSS:
            return "WSS";
    }
    return "";
}

char *
sip_get_msg_header(sip_msg_t *msg, char *out)
{
    char from_addr[80], to_addr[80], time[80], date[80];

    // Source and Destination address
    msg_get_attribute(msg, SIP_ATTR_DATE, date);
    msg_get_attribute(msg, SIP_ATTR_TIME, time);
    msg_get_attribute(msg, SIP_ATTR_SRC, from_addr);
    msg_get_attribute(msg, SIP_ATTR_DST, to_addr);

    // Get msg header
    sprintf(out, "%s %s %s -> %s", date, time, from_addr, to_addr);
    return out;
}

void
sip_set_sort_options(SStorageSortOpts sort)
{
    calls.sort = sort;
    sip_sort_list();
}

SStorageSortOpts
sip_sort_options()
{
    return calls.sort;
}

void
sip_sort_list()
{
    g_sequence_sort(calls.list, sip_list_sorter, NULL);
}

gint
sip_list_sorter(gconstpointer a, gconstpointer b, gpointer user_data)
{
    const sip_call_t *calla = a, *callb = b;
    int cmp = call_attr_compare(calla, callb, calls.sort.by);
    return (calls.sort.asc) ? cmp : cmp * -1;
}
