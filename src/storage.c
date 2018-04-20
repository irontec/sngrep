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
#include <glib.h>
#include "glib-utils.h"
#include "packet/dissectors/packet_sip.h"
#include "storage.h"
#include "setting.h"
#include "filter.h"

/**
 * @brief Linked list of parsed calls
 *
 * All parsed calls will be added to this list, only accesible from
 * this awesome structure, so, keep it thread-safe.
 */
sip_call_list_t calls =
        {0};

gboolean
storage_init(SStorageCaptureOpts capture_options,
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
storage_deinit()
{
    // Remove all calls
    storage_calls_clear();
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

gint
storage_sorter(gconstpointer a, gconstpointer b, gpointer user_data G_GNUC_UNUSED)
{
    const sip_call_t *calla = a, *callb = b;
    int cmp = call_attr_compare(calla, callb, calls.sort.by);
    return (calls.sort.asc) ? cmp : cmp * -1;
}

sip_msg_t *
storage_check_packet(Packet *packet)
{
    sip_msg_t *msg;
    sip_call_t *call;
    gboolean newcall = false;

    PacketSipData *sip_data = g_ptr_array_index(packet->proto, PACKET_SIP);

    // Create a new message from this data
    msg = msg_create();
    msg->cseq = sip_data->cseq;
    msg->sip_from = sip_data->from;
    msg->sip_to = sip_data->to;
    msg->reqresp = sip_data->reqresp;
    msg->resp_str = sip_data->resp_str;

    // Find the call for this msg
    if (!(call = storage_find_by_callid(sip_data->callid))) {

        // Check if payload matches expression
        if (!storage_check_match_expr(sip_data->payload))
            goto skip_message;

        // User requested only INVITE starting dialogs
        if (calls.match.invite && msg->reqresp != SIP_METHOD_INVITE)
            goto skip_message;

        // Only create a new call if the first msg
        // is a request message in the following gorup
        if (calls.match.complete && msg->reqresp > SIP_METHOD_MESSAGE)
            goto skip_message;

        // Rotate call list if limit has been reached
        if (calls.capture.limit == storage_calls_count())
            storage_calls_rotate();

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
            call_add_xcall(storage_find_by_callid(call->xcallid), call);
        }
    }

    // Add the message to the call
    call_add_message(call, msg);

    // check if message is a retransmission
    call_msg_retrans_check(msg);

    if (call_is_invite(call)) {
        // Parse media data
        storage_register_streams(msg);
        // Update Call State
        call_update_state(call, msg);
        // Check if this call should be in active call list
        if (call_is_active(call)) {
            if (storage_call_is_active(call)) {
                g_sequence_append(calls.active, call);
            }
        } else {
            if (storage_call_is_active(call)) {
                g_sequence_remove_data(calls.active, call);
            }
        }
    }

    if (newcall) {
        // Append this call to the call list
        g_sequence_insert_sorted(calls.list, call, storage_sorter, NULL);
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

gboolean
storage_calls_changed()
{
    gboolean changed = calls.changed;
    calls.changed = false;
    return changed;
}

int
storage_calls_count()
{
    return g_sequence_get_length(calls.list);
}

GSequenceIter *
storage_calls_iterator()
{
    return g_sequence_get_begin_iter(calls.list);
}

gboolean
storage_call_is_active(sip_call_t *call)
{
    return g_sequence_index(calls.active, call) != -1;
}

GSequence *
storage_calls_vector()
{
    return calls.list;
}

GSequence *
storage_active_calls_vector()
{
    return calls.active;
}

sip_stats_t
storage_calls_stats()
{
    sip_stats_t stats = {};
    GSequenceIter *it = g_sequence_get_begin_iter(calls.list);

    // Total number of calls without filtering
    stats.total = g_sequence_iter_length(it);
    // Total number of calls after filtering
    for (; !g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        if (filter_check_call(g_sequence_get(it), NULL))
            stats.displayed++;
    }
    return stats;
}

sip_call_t *
storage_find_by_callid(const char *callid)
{
    return g_hash_table_lookup(calls.callids, callid);
}

void
storage_register_streams(sip_msg_t *msg)
{
    Packet *packet = msg->packet->newpacket;
    Address emptyaddr = {};

    PacketSdpData *sdp = g_ptr_array_index(packet->proto, PACKET_SDP);
    if (sdp == NULL) {
        // Packet with SDP content
        return;
    }

    for (guint i = 0; i < g_list_length(sdp->medias); i++) {
        PacketSdpMedia *media = g_list_nth_data(sdp->medias, i);

        // Add to the message
        g_sequence_append(msg->medias, media);

        // Create RTP stream for this media
        if (rtp_find_call_stream(msg->call, emptyaddr, media->address) == NULL) {
            rtp_stream_t *stream = stream_create(packet, media);
            call_add_stream(msg->call, stream);
        }

        // Create RTCP stream for this media
        if (rtp_find_call_stream(msg->call, emptyaddr, media->address) == NULL) {
            rtp_stream_t *stream = stream_create(packet, media);
            stream->dst.port = (media->rtcpport) ? media->rtcpport : (guint16) (media->rtpport + 1);
            call_add_stream(msg->call, stream);
        }

        // Create RTP stream with source of message as destination address
        if (rtp_find_call_stream(msg->call, msg->packet->src, media->address) == NULL) {
            rtp_stream_t *stream = stream_create(packet, media);
            stream->dst = msg->packet->src;
            stream->dst.port = media->rtpport;
            call_add_stream(msg->call, stream);
        }
    }
}

void
storage_calls_clear()
{
    // Create again the callid hash table
    g_hash_table_remove_all(calls.callids);

    // Remove all items from vector
    g_sequence_remove_all(calls.list);
    g_sequence_remove_all(calls.active);
}

void
storage_calls_clear_soft()
{
    // Create again the callid hash table
    g_hash_table_remove_all(calls.callids);

    // Repopulate list applying current filter
    calls.list = g_sequence_copy(storage_calls_vector(), filter_check_call, NULL);
    calls.active = g_sequence_copy(storage_active_calls_vector(), filter_check_call, NULL);

    // Repopulate callids based on filtered list
    sip_call_t *call;
    GSequenceIter *it = g_sequence_get_begin_iter(calls.list);

    for (; !g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        call = g_sequence_get(it);
        g_hash_table_insert(calls.callids, call->callid, call);
    }
}

void
storage_calls_rotate()
{
    sip_call_t *call;
    GSequenceIter *it = g_sequence_get_begin_iter(calls.list);
    for (; !g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
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
storage_match_expr()
{
    return calls.match.mexpr;
}

int
storage_check_match_expr(const char *payload)
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

void
storage_set_sort_options(SStorageSortOpts sort)
{
    calls.sort = sort;
    g_sequence_sort(calls.list, storage_sorter, NULL);
}

SStorageSortOpts
storage_sort_options()
{
    return calls.sort;
}

