/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2026 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2026 Irontec SL. All rights reserved.
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
 * @file test_015.c
 *
 * Unit tests for the call list Codec column (SIP_ATTR_CODEC).
 *
 * Calls are moved through their states with the real sip_call.c functions and
 * SDP medias are built with the real media.c functions. Only message, filter,
 * settings and packet dependencies are stubbed.
 */
#include "config.h"
#ifdef WITH_PCRE2
#define PCRE2_CODE_UNIT_WIDTH 8
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/sip.h"
#include "../src/media.h"
#include "../src/filter.h"
#include "../src/setting.h"

/****************************************************************************
 * Stubs for dependencies outside of the tested code
 ****************************************************************************/
const char *
msg_get_attribute(struct sip_msg *msg, int id, char *value)
{
    return NULL;
}

const char *
msg_get_payload(sip_msg_t *msg)
{
    return "";
}

struct timeval
msg_get_time(sip_msg_t *msg)
{
    struct timeval t = { 0 };
    return t;
}

sip_call_t *
msg_get_call(const sip_msg_t *msg)
{
    return msg->call;
}

void
msg_destroyer(void *msg)
{
}

int
setting_enabled(int id)
{
    return 0;
}

const char *
sip_transport_str(int transport)
{
    return "UDP";
}

vector_iter_t
sip_active_calls_iterator()
{
    vector_iter_t it;
    memset(&it, 0, sizeof(it));
    return it;
}

const char *
filter_get(int type)
{
    return NULL;
}

void
filter_call_state_changed(sip_call_t *call)
{
}

/****************************************************************************
 * Helpers
 ****************************************************************************/

/**
 * @brief Add a message with an optional audio m-line to the call
 *
 * @param port audio port (0 = disabled stream, -1 = no SDP)
 * @param fmt first payload type of the m-line
 * @param rtpmap a=rtpmap value for fmt or NULL
 */
static sip_msg_t *
add_msg(sip_call_t *call, int reqresp, uint32_t cseq, int port, uint32_t fmt, const char *rtpmap)
{
    sip_msg_t *msg = calloc(1, sizeof(sip_msg_t));
    sdp_media_t *media;
    address_t addr = { .port = port };
    assert(msg);

    msg->reqresp = reqresp;
    msg->cseq = cseq;
    msg->medias = vector_create(0, 1);
    if (port >= 0) {
        media = media_create(msg);
        media_set_type(media, "audio");
        media_set_address(media, addr);
        media_set_prefered_format(media, fmt);
        if (rtpmap)
            media_add_format(media, fmt, rtpmap);
        vector_append(msg->medias, media);
    }

    call_add_message(call, msg);
    call_update_state(call, msg);
    return msg;
}

static const char *
codec(sip_call_t *call)
{
    static char value[SIP_ATTR_MAXLEN];
    memset(value, 0, sizeof(value));
    return call_get_attribute(call, SIP_ATTR_CODEC, value);
}

static int
codec_is(sip_call_t *call, const char *expected)
{
    const char *value = codec(call);
    return value && !strcmp(value, expected);
}

#define NO_SDP -1, 0, NULL

int
main()
{
    sip_call_t *call;
    sip_msg_t *msg;
    sdp_media_t *video, *audio;
    address_t addr = { .port = 5000 };

    // Static payload type without rtpmap, offer is ignored
    call = call_create("call-static", "");
    add_msg(call, SIP_METHOD_INVITE, 1, 4000, 0, NULL);
    assert(codec(call) == NULL);
    add_msg(call, 180, 1, NO_SDP);
    assert(codec(call) == NULL);
    add_msg(call, 200, 1, 4002, 9, NULL);
    add_msg(call, SIP_METHOD_ACK, 1, NO_SDP);
    assert(call->state == SIP_CALLSTATE_INCALL);
    assert(codec_is(call, "9/G722/8000"));

    // Successful re-INVITE changes the codec, failed one does not
    add_msg(call, SIP_METHOD_INVITE, 2, 4000, 18, NULL);
    add_msg(call, 200, 2, 4002, 18, NULL);
    assert(codec_is(call, "18/G729/8000"));
    add_msg(call, SIP_METHOD_INVITE, 3, 4000, 8, NULL);
    add_msg(call, 488, 3, 4002, 8, NULL);
    assert(codec_is(call, "18/G729/8000"));

    // Codec is kept after the call is completed
    add_msg(call, SIP_METHOD_BYE, 4, NO_SDP);
    assert(call->state == SIP_CALLSTATE_COMPLETED);
    assert(codec_is(call, "18/G729/8000"));

    // Dynamic payload type uses rtpmap, unknown one shows only the number
    call = call_create("call-dynamic", "");
    add_msg(call, SIP_METHOD_INVITE, 1, 4000, 111, "opus/48000/2");
    msg = add_msg(call, 200, 1, 4002, 111, "opus/48000/2");
    add_msg(call, SIP_METHOD_ACK, 1, NO_SDP);
    assert(codec_is(call, "111/opus/48000/2"));
    ((sdp_media_t *) vector_first(msg->medias))->fmtcode = 96;
    assert(codec_is(call, "96"));

    // Video and disabled audio streams are skipped
    call = call_create("call-media", "");
    add_msg(call, SIP_METHOD_INVITE, 1, 4000, 0, NULL);
    msg = add_msg(call, 200, 1, 0, 8, NULL);
    add_msg(call, SIP_METHOD_ACK, 1, NO_SDP);
    assert(codec(call) == NULL);
    video = media_create(msg);
    media_set_type(video, "video");
    media_set_address(video, addr);
    media_set_prefered_format(video, 34);
    vector_insert(msg->medias, video, 0);
    assert(codec(call) == NULL);
    audio = media_create(msg);
    media_set_type(audio, "audio");
    media_set_address(audio, addr);
    media_set_prefered_format(audio, 8);
    vector_append(msg->medias, audio);
    assert(codec_is(call, "8/PCMA/8000"));

    // Late offer: answer in ACK
    call = call_create("call-late", "");
    add_msg(call, SIP_METHOD_INVITE, 1, NO_SDP);
    add_msg(call, 200, 1, 4002, 0, NULL);
    add_msg(call, SIP_METHOD_ACK, 1, 4000, 8, NULL);
    assert(codec_is(call, "8/PCMA/8000"));

    // Answer in reliable provisional response, 200 OK without SDP
    call = call_create("call-early", "");
    add_msg(call, SIP_METHOD_INVITE, 1, 4000, 0, NULL);
    add_msg(call, 183, 1, 4002, 8, NULL);
    add_msg(call, 200, 1, NO_SDP);
    add_msg(call, SIP_METHOD_ACK, 1, NO_SDP);
    assert(codec_is(call, "8/PCMA/8000"));

    // Rejected and cancelled calls have no codec
    call = call_create("call-rejected", "");
    add_msg(call, SIP_METHOD_INVITE, 1, 4000, 0, NULL);
    add_msg(call, 183, 1, 4002, 0, NULL);
    add_msg(call, 486, 1, NO_SDP);
    assert(call->state == SIP_CALLSTATE_BUSY);
    assert(codec(call) == NULL);

    puts("codec column: OK");
    return 0;
}
