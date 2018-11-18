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
 * @brief Source of functions defined in storage.h
 *
 *
 * Contains all stored calls, storage thread, pending packet queue and matching
 * information. Capture threads send their packet information to storage, that provides the
 * data that will be printed in the screen.
 *
 *
 *             +--------------------------+
 *             |                          |
 *        +--->|      User Interface      |
 *        |    |                          |
 *        |    +--------------------------+
 *        |    +--------------------------+
 *        +--->|                          | <----------- You are here.
 *             |         Storage          |
 *        +--->|                          |----+
 * Packet |    +--------------------------+    | Capture
 * Queue  |    +--------------------------+    | Output
 *        |    |                          |    |
 *        +--- |     Capture Manager      |<---+
 *             |                          |
 *             +--------------------------+
 *
 */
#include "config.h"
#include <glib.h>
#include <glib/gprintf.h>
#include "glib-utils.h"
#include "packet/dissectors/packet_sip.h"
#include "storage.h"
#include "setting.h"
#include "filter.h"

/**
 * @brief Global Structure with all storage information
 */
Storage storage = { 0 };

void
storage_add_packet(Packet *packet)
{
    g_async_queue_push(storage.pkt_queue, packet);
}

static gint
storage_sorter(gconstpointer a, gconstpointer b)
{
    const Call *calla = a, *callb = b;
    int cmp = call_attr_compare(calla, callb, storage.sort.by);
    return (storage.sort.asc) ? cmp : cmp * -1;
}

gboolean
storage_calls_changed()
{
    gboolean changed = storage.changed;
    storage.changed = false;
    return changed;
}

guint
storage_calls_count()
{
    return g_ptr_array_len(storage.calls);
}

GPtrArray *
storage_calls()
{
    return storage.calls;
}

sip_stats_t
storage_calls_stats()
{
    sip_stats_t stats = { 0 };

    // Total number of calls without filtering
    stats.total = g_ptr_array_len(storage.calls);

    // Total number of calls after filtering
    for (guint i = 0; i < g_ptr_array_len(storage.calls); i++) {
        if (filter_check_call(g_ptr_array_index(storage.calls, i), NULL))
            stats.displayed++;
    }

    return stats;
}

void
storage_calls_clear()
{
    // Create again the callid hash table
    g_hash_table_remove_all(storage.callids);
    g_hash_table_remove_all(storage.streams);

    // Remove all items from vector
    g_ptr_array_free(storage.calls, TRUE);
}

void
storage_calls_clear_soft()
{
    // Filter current call list
    for (guint i = 0; i < g_ptr_array_len(storage.calls); i++) {
        Call *call = g_ptr_array_index(storage.calls, i);

        // Filtered call, remove from all lists
        if (filter_check_call(call, NULL)) {
            // Remove from Call-Id hash table
            g_hash_table_remove(storage.callids, call->callid);
            // Remove from calls list
            g_ptr_array_remove(storage.calls, call);
        }
    }
}

static void
storage_calls_rotate()
{
    for (guint i = 0; i < g_ptr_array_len(storage.calls); i++) {
        Call *call = g_ptr_array_index(storage.calls, i);

        if (!call->locked) {
            // Remove from callids hash
            g_hash_table_remove(storage.callids, call->callid);
            // Remove first call from active and call lists
            g_ptr_array_remove(storage.calls, call);
            return;
        }
    }
}

static gboolean
storage_check_match_expr(const char *payload)
{
    // Everything matches when there is no match
    if (storage.match.mexpr == NULL)
        return 1;

    // Check if payload matches the given expresion
    if (g_regex_match(storage.match.mregex, payload, 0, NULL)) {
        return 0 == storage.match.minvert;
    } else {
        return 1 == storage.match.minvert;
    }

}

StorageMatchOpts
storage_match_options()
{
    return storage.match;
}

void
storage_set_sort_options(StorageSortOpts sort)
{
    storage.sort = sort;
}

StorageSortOpts
storage_sort_options()
{
    return storage.sort;
}

static Message *
storage_check_sip_packet(Packet *packet)
{
    Message *msg;
    Call *call;
    gboolean newcall = false;

    PacketSipData *sip_data = g_ptr_array_index(packet->proto, PACKET_SIP);

    // Find the call for this msg
    if (!(call = g_hash_table_lookup(storage.callids, sip_data->callid))) {

        // Check if payload matches expression
        if (!storage_check_match_expr(sip_data->payload))
            return NULL;

        // User requested only INVITE starting dialogs
        if (storage.match.invite && sip_data->reqresp != SIP_METHOD_INVITE)
            return NULL;

        // Only create a new call if the first msg
        // is a request message in the following gorup
        if (storage.match.complete && sip_data->reqresp > SIP_METHOD_MESSAGE)
            return NULL;

        // Rotate call list if limit has been reached
        if (storage.capture.limit == storage_calls_count())
            storage_calls_rotate();

        // Create the call if not found
        if (!(call = call_create(sip_data->callid, sip_data->xcallid)))
            return NULL;

        // Add this Call-Id to hash table
        g_hash_table_insert(storage.callids, g_strdup(call->callid), call);

        // Set call index
        call->index = ++storage.last_index;

        // Mark this as a new call
        newcall = true;
    }

    // At this point we know we're handling an interesting SIP Packet
    // FIXME Create a new message from this data
    msg = msg_create();
    msg->packet = packet;

    // Always dissect first call message
    if (call_msg_count(call) == 0) {
        // If this call has X-Call-Id, append it to the parent call
        if (call->xcallid) {
            call_add_xcall(g_hash_table_lookup(storage.callids, call->xcallid), call);
        }
    }

    // Add the message to the call
    call_add_message(call, msg);

    if (call_is_invite(call)) {
        // Parse media data
        storage_register_streams(msg);
        // Update Call State
        call_update_state(call, msg);
    }

    if (newcall) {
        // Append this call to the call list
        g_ptr_array_add(storage.calls, call);
        g_ptr_array_sort(storage.calls, storage_sorter);
    }

    // Mark the list as changed
    storage.changed = true;

    // Send this packet to all capture outputs
    capture_manager_output_packet(capture_manager(), packet);

    // Return the loaded message
    return msg;
}

static void
storage_register_stream(RtpStream *stream)
{
    gchar *key = g_strdup_printf("%s:%hu", stream->dst.ip, stream->dst.port);
    g_hash_table_insert(storage.streams, key, stream->msg);
}

static RtpStream *
storage_check_rtp_packet(Packet *packet)
{
    PacketRtpData *rtp = g_ptr_array_index(packet->proto, PACKET_RTP);
    g_return_val_if_fail(rtp != NULL, NULL);

    // Get Addresses from packet
    Address src = packet_src_address(packet);
    Address dst = packet_dst_address(packet);

    // Find the stream by destination
    gchar hashkey[ADDRESSLEN + 5 + 1];
    g_sprintf(hashkey, "%s:%hu", dst.ip, dst.port);
    Message *msg = g_hash_table_lookup(storage.streams, hashkey);

    // No call has setup this stream
    if (msg == NULL) return NULL;

    // Get Call streams
    Call *call = msg_get_call(msg);

    // Find a matching stream in the call
    RtpStream *stream = NULL;
    for (guint i = 0; i < g_ptr_array_len(call->streams); i++) {
        stream = g_ptr_array_index(call->streams, i);

        if (address_equals(stream->dst, dst)) {
            // First packet of early setup stream from SDP
            if (address_empty(stream->src)) {
                stream_set_src(stream, src);
                stream_set_format(stream, rtp->encoding->id);

                // Create an exact stream for the opposite direction
                RtpStream *reverse = stream_new(STREAM_RTP, msg, stream->media);
                stream_set_data(reverse, dst, src);
                stream_set_format(reverse, rtp->encoding->id);
                call_add_stream(call, reverse);
                storage_register_stream(reverse);
            }

            // Add packet to existing matching stream
            if (address_equals(stream->src, src) && stream->fmtcode == rtp->encoding->id) {
                stream_add_packet(stream, packet);
                break;
            }
        }

        // Try next stream
        stream = NULL;
    }

    // If no stream matches this packet
    if (stream == NULL) {
        // Create a new stream for this source
        stream = stream_new(STREAM_RTP, msg, msg_media_for_addr(msg, dst));
        stream_set_data(stream, src, dst);
        stream_set_format(stream, rtp->encoding->id);
        call_add_stream(call, stream);
        storage_register_stream(stream);
    }

    return stream;
}

static RtpStream *
storage_check_rtcp_packet(Packet *packet)
{
    PacketRtcpData *rtcp = g_ptr_array_index(packet->proto, PACKET_RTP);
    g_return_val_if_fail(rtcp != NULL, NULL);

    // Get Addresses from packet
    Address src = packet_src_address(packet);
    Address dst = packet_dst_address(packet);

    // Find the stream by destination
    gchar hashkey[ADDRESSLEN + 5 + 1];
    g_sprintf(hashkey, "%s:%hu", dst.ip, dst.port);
    GList *streams = g_hash_table_lookup(storage.streams, hashkey);

    // Check if one of the destination has the configured format
    for (GList *l = streams; l != NULL; l = l->next) {
        RtpStream *stream = l->data;
        // Add packet to stream
        stream_set_data(stream, src, dst);
        stream_add_packet(stream, packet);
        return stream;
    }

    return NULL;
}

void
storage_register_streams(Message *msg)
{
    Packet *packet = msg->packet;
    Address emptyaddr = { 0 };

    PacketSdpData *sdp = g_ptr_array_index(packet->proto, PACKET_SDP);
    if (sdp == NULL) {
        // Packet with SDP content
        return;
    }

    for (guint i = 0; i < g_list_length(sdp->medias); i++) {
        PacketSdpMedia *media = g_list_nth_data(sdp->medias, i);

        // Add to the message
        msg->medias = g_list_append(msg->medias, media);

        // Create RTP stream for this media
        if (call_find_stream(msg->call, emptyaddr, media->address) == NULL) {
            RtpStream *stream = stream_new(STREAM_RTP, msg, media);
            stream_set_dst(stream, media->address);
            call_add_stream(msg->call, stream);
            storage_register_stream(stream);
        }

        // Create RTCP stream for this media
        if (call_find_stream(msg->call, emptyaddr, media->address) == NULL) {
            RtpStream *stream = stream_new(STREAM_RTCP, msg, media);
            stream_set_dst(stream, media->address);
            stream->dst.port = (media->rtcpport) ? media->rtcpport : (guint16) (media->rtpport + 1);
            call_add_stream(msg->call, stream);
            storage_register_stream(stream);
        }

        // Create RTP stream with source of message as destination address
        if (call_find_stream(msg->call, msg_src_address(msg), media->address) == NULL) {
            RtpStream *stream = stream_new(STREAM_RTP, msg, media);
            stream_set_dst(stream, msg_src_address(msg));
            stream->dst.port = media->rtpport;
            call_add_stream(msg->call, stream);
            storage_register_stream(stream);
        }
    }
}

//! Start capturing packets function
static gpointer
storage_check_packet()
{
    while (storage.running) {

        Packet *packet = g_async_queue_timeout_pop(storage.pkt_queue, 500000);
        if (packet) {
            if (packet_has_type(packet, PACKET_SIP)) {
                storage_check_sip_packet(packet);
            } else if (packet_has_type(packet, PACKET_RTP)) {
                storage_check_rtp_packet(packet);
            } else if (packet_has_type(packet, PACKET_RTCP)) {
                storage_check_rtcp_packet(packet);
            }

        }
    }

    return NULL;
}

gboolean
storage_init(StorageCaptureOpts capture_options,
             StorageMatchOpts match_options,
             StorageSortOpts sort_options,
             GError **error)
{
    GRegexCompileFlags cflags = G_REGEX_EXTENDED;
    GRegexMatchFlags mflags = G_REGEX_MATCH_NEWLINE_CRLF;

    storage.capture = capture_options;
    storage.match = match_options;
    storage.sort = sort_options;

    // Store capture limit
    storage.last_index = 0;

    // Validate match expression
    if (storage.match.mexpr) {
        // Case insensitive requested
        if (storage.match.micase) {
            cflags |= G_REGEX_CASELESS;
        }

        // Check the expresion is a compilable regexp
        storage.match.mregex = g_regex_new(storage.match.mexpr, cflags, mflags, error);
        if (storage.match.mregex == NULL) {
            return FALSE;
        }
    }

    // Initialize storage packet queue
    storage.pkt_queue = g_async_queue_new();

    // Create a vector to store calls
    storage.calls = g_ptr_array_new_with_free_func(call_destroy);

    // Create hash tables for fast call and stream search
    storage.callids = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    storage.streams = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    // Set default sorting field
    if (attr_find_by_name(setting_get_value(SETTING_CL_SORTFIELD)) >= 0) {
        storage.sort.by = attr_find_by_name(setting_get_value(SETTING_CL_SORTFIELD));
        storage.sort.asc = (!strcmp(setting_get_value(SETTING_CL_SORTORDER), "asc"));
    } else {
        // Fallback to default sorting field
        storage.sort.by = ATTR_CALLINDEX;
        storage.sort.asc = true;
    }

    storage.running = TRUE;
    storage.thread = g_thread_new(NULL, (GThreadFunc) storage_check_packet, NULL);

    return TRUE;
}

void
storage_deinit()
{
    // Stop storage thread
    storage.running = FALSE;
    g_thread_join(storage.thread);
    // Remove all calls
    storage_calls_clear();
    // Remove Call-id hash table
    g_hash_table_destroy(storage.callids);
}
