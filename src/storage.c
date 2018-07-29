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
#include "glib-utils.h"
#include "packet/dissectors/packet_sip.h"
#include "storage.h"
#include "setting.h"
#include "filter.h"

/**
 * @brief Global Structure with all storage information
 */
Storage storage = {};

void
storage_add_packet(Packet *packet)
{
    g_async_queue_push(storage.pkt_queue, packet);
}

static gint
storage_sorter(gconstpointer a, gconstpointer b, G_GNUC_UNUSED gpointer user_data)
{
    const SipCall *calla = a, *callb = b;
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

int
storage_calls_count()
{
    return g_sequence_get_length(storage.list);
}

GSequenceIter *
storage_calls_iterator()
{
    return g_sequence_get_begin_iter(storage.list);
}

static gboolean
storage_call_is_active(SipCall *call)
{
    return g_sequence_index(storage.active, call) != -1;
}

GSequence *
storage_calls_vector()
{
    return storage.list;
}

GSequence *
storage_active_calls_vector()
{
    return storage.active;
}

sip_stats_t
storage_calls_stats()
{
    sip_stats_t stats = {};
    GSequenceIter *it = g_sequence_get_begin_iter(storage.list);

    // Total number of calls without filtering
    stats.total = g_sequence_iter_length(it);
    // Total number of calls after filtering
    for (; !g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        if (filter_check_call(g_sequence_get(it), NULL))
            stats.displayed++;
    }
    return stats;
}

void
storage_calls_clear()
{
    // Create again the callid hash table
    g_hash_table_remove_all(storage.callids);

    // Remove all items from vector
    g_sequence_remove_all(storage.list);
    g_sequence_remove_all(storage.active);
}

void
storage_calls_clear_soft()
{
    // Create again the callid hash table
    g_hash_table_remove_all(storage.callids);

    // Repopulate list applying current filter
    storage.list = g_sequence_copy(storage_calls_vector(), filter_check_call, NULL);
    storage.active = g_sequence_copy(storage_active_calls_vector(), filter_check_call, NULL);

    // Repopulate callids based on filtered list
    SipCall *call;
    GSequenceIter *it = g_sequence_get_begin_iter(storage.list);

    for (; !g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        call = g_sequence_get(it);
        g_hash_table_insert(storage.callids, call->callid, call);
    }
}

static void
storage_calls_rotate()
{
    SipCall *call;
    GSequenceIter *it = g_sequence_get_begin_iter(storage.list);
    for (; !g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        call = g_sequence_get(it);
        if (!call->locked) {
            // Remove from callids hash
            g_hash_table_remove(storage.callids, call->callid);
            // Remove first call from active and call lists
            g_sequence_remove_data(storage.active, call);
            g_sequence_remove_data(storage.list, call);
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

const StorageMatchOpts
storage_match_options()
{
    return storage.match;
}

void
storage_set_sort_options(StorageSortOpts sort)
{
    storage.sort = sort;
    g_sequence_sort(storage.list, storage_sorter, NULL);
}

const StorageSortOpts
storage_sort_options()
{
    return storage.sort;
}

static SipMsg *
storage_check_sip_packet(Packet *packet)
{
    SipMsg *msg;
    SipCall *call;
    gboolean newcall = false;

    PacketSipData *sip_data = g_ptr_array_index(packet->proto, PACKET_SIP);

    // FIXME Create a new message from this data
    msg = msg_create();
    msg->cseq = sip_data->cseq;
    msg->sip_from = sip_data->from;
    msg->sip_to = sip_data->to;
    msg->reqresp = sip_data->reqresp;
    msg->resp_str = sip_data->resp_str;

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
        g_hash_table_insert(storage.callids, call->callid, call);

        // Set call index
        call->index = ++storage.last_index;

        // Mark this as a new call
        newcall = true;
    }

    // At this point we know we're handling an interesting SIP Packet
    msg->packet = packet;

    // Always dissect first call message
    if (call_msg_count(call) == 0) {
        // If this call has X-Call-Id, append it to the parent call
        if (strlen(call->xcallid)) {
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
        // Check if this call should be in active call list
        if (call_is_active(call)) {
            if (storage_call_is_active(call)) {
                g_sequence_append(storage.active, call);
            }
        } else {
            if (storage_call_is_active(call)) {
                g_sequence_remove_data(storage.active, call);
            }
        }
    }

    if (newcall) {
        // Append this call to the call list
        g_sequence_insert_sorted(storage.list, call, storage_sorter, NULL);
    }

    // Mark the list as changed
    storage.changed = true;

    // Send this packet to all capture outputs
    capture_manager_output_packet(capture_manager(), packet);

    // Return the loaded message
    return msg;
}

static rtp_stream_t *
storage_check_rtp_packet(Packet *packet)
{
    Address src, dst;
    rtp_stream_t *stream;
    rtp_stream_t *reverse;
    u_char format = 0;
    const gchar *payload;
    size_t size, bsize;
    uint16_t len;
    struct rtcp_hdr_generic hdr;
    struct rtcp_hdr_sr hdr_sr;
    struct rtcp_hdr_xr hdr_xr;
    struct rtcp_blk_xr blk_xr;
    struct rtcp_blk_xr_voip blk_xr_voip;

    // Get Addresses from packet
    src = packet_src_address(packet);
    dst = packet_dst_address(packet);

    // Check if packet has RTP data
    PacketRtpData *rtp = g_ptr_array_index(packet->proto, PACKET_RTP);
    if (rtp != NULL) {
        // Get RTP Encoding information
        guint8 format = rtp->encoding->id;

        // Find the matching stream
        stream = stream_find_by_format(src, dst, format);

        // Check if a valid stream has been found
        if (!stream)
            return NULL;

        // We have found a stream, but with different format
        if (stream_is_complete(stream) && stream->fmtcode != format) {
            // Create a new stream for this new format
            stream = stream_create(packet, stream->media);
            stream_complete(stream, src);
            stream_set_format(stream, format);
            call_add_stream(msg_get_call(stream->msg), stream);
        }

        // First packet for this stream, set source data
        if (!(stream_is_complete(stream))) {
            stream_complete(stream, src);
            stream_set_format(stream, format);

            /**
             * TODO This is a mess. Rework required
             *
             * This logic tries to handle a common problem when SDP address and RTP address
             * doesn't match. In some cases one endpoint waits until RTP data is sent to its
             * configured port in SDP and replies its RTP to the source ignoring what the other
             * endpoint has configured in its SDP.
             *
             * For such cases, we create streams 'on the fly', when a stream is completed (the
             * first time its source address is filled), a new stream is created with the
             * opposite src and dst.
             *
             * BUT, there are some cases when this 'reverse' stream should not be created:
             *  - When there already exists a stream with that setup
             *  - When there exists an incomplete stream with that destination (and still no source)
             *  - ...
             *
             */

            // Check if an stream in the opposite direction exists
            if (!(reverse = call_find_stream(stream->msg->call, stream->dst, stream->src))) {
                reverse = stream_create(packet, stream->media);
                stream_complete(reverse, stream->dst);
                stream_set_format(reverse, format);
                call_add_stream(msg_get_call(stream->msg), reverse);
            } else {
                // If the reverse stream has other source configured
                if (reverse->src.port && !addressport_equals(stream->src, reverse->src)) {
                    if (!(reverse = call_find_stream_exact(stream->msg->call, stream->dst, stream->src))) {
                        // Create a new reverse stream
                        reverse = stream_create(packet, stream->media);
                        stream_complete(reverse, stream->dst);
                        stream_set_format(reverse, format);
                        call_add_stream(msg_get_call(stream->msg), reverse);
                    }
                }
            }
        }

        // Add packet to stream
        stream_add_packet(stream, packet);
    }

    // Check if packet has RTP data
    PacketRtcpData *rtcp = g_ptr_array_index(packet->proto, PACKET_RTP);
    if (rtcp != NULL) {
        // Add packet to stream
        stream_complete(stream, src);
        stream_add_packet(stream, packet);
    } else {
        return NULL;
    }

    return stream;
}

void
storage_register_streams(SipMsg *msg)
{
    Packet *packet = msg->packet;
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
        if (call_find_stream(msg->call, emptyaddr, media->address) == NULL) {
            rtp_stream_t *stream = stream_create(packet, media);
            stream->type = PACKET_RTP;
            stream->msg = msg;
            call_add_stream(msg->call, stream);
        }

        // Create RTCP stream for this media
        if (call_find_stream(msg->call, emptyaddr, media->address) == NULL) {
            rtp_stream_t *stream = stream_create(packet, media);
            stream->dst.port = (media->rtcpport) ? media->rtcpport : (guint16) (media->rtpport + 1);
            stream->type = PACKET_RTCP;
            stream->msg = msg;
            call_add_stream(msg->call, stream);
        }

        // Create RTP stream with source of message as destination address
        if (call_find_stream(msg->call, msg_src_address(msg), media->address) == NULL) {
            rtp_stream_t *stream = stream_create(packet, media);
            stream->type = PACKET_RTP;
            stream->msg = msg;
            stream->dst = msg_src_address(msg);
            stream->dst.port = media->rtpport;
            call_add_stream(msg->call, stream);
        }
    }
}

//! Start capturing packets function
static void storage_check_packet()
{
    while (storage.running) {

        Packet *packet = g_async_queue_timeout_pop(storage.pkt_queue, 500000);
        if (packet) {
            if (packet_has_type(packet, PACKET_SIP)) {
                storage_check_sip_packet(packet);
            } else if (packet_has_type(packet, PACKET_RTP)) {
                storage_check_rtp_packet(packet);
            }
        }
    }
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
    storage.list = g_sequence_new(call_destroy);
    storage.active = g_sequence_new(NULL);

    // Create hash table for callid search
    storage.callids = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    // Set default sorting field
    if (sip_attr_from_name(setting_get_value(SETTING_CL_SORTFIELD)) >= 0) {
        storage.sort.by = sip_attr_from_name(setting_get_value(SETTING_CL_SORTFIELD));
        storage.sort.asc = (!strcmp(setting_get_value(SETTING_CL_SORTORDER), "asc"));
    } else {
        // Fallback to default sorting field
        storage.sort.by = SIP_ATTR_CALLINDEX;
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
    // Remove calls vector
    g_sequence_free(storage.list);
    g_sequence_free(storage.active);

}
