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

#include "config.h"
#include <glib.h>
#include <pulse/simple.h>
#include "capture/codecs/codec_g711a.h"
#ifdef WITH_G729
#include "capture/codecs/codec_g729.h"
#endif
#include "ncurses/keybinding.h"
#include "ncurses/theme.h"
#include "ncurses/dialog.h"
#include "glib-extra.h"
#include "rtp_player_win.h"

static RtpPlayerInfo *
rtp_player_win_info(Window *window)
{
    return (RtpPlayerInfo *) panel_userptr(window->panel);
}

static gint
rtp_player_draw(Window *window)
{
    RtpPlayerInfo *info = rtp_player_win_info(window);
    g_return_val_if_fail(info != NULL, 1);

    if (info->pa_state == PA_CONTEXT_TERMINATED ||
        info->pa_state == PA_CONTEXT_FAILED) {
        mvwprintw(window->win, 4, 3, "Failed to connect to PulseAudio server.");
        return 0;
    }

    if (info->pa_state != PA_CONTEXT_READY) {
        mvwprintw(window->win, 4, 3, "Connecting to PulseAudio server...");
        return 0;
    }

    gint width = getmaxx(window->win);
    mvwhline(window->win, 4, 4, '-', width - 19);
    mvwaddch(window->win, 4, 3, '[');
    mvwaddch(window->win, 4, width - 15, ']');

    mvwprintw(window->win, 4, width - 13, "%02d:%02d/%02d:%02d",
              (info->player_pos / 8000) / 60,      /* current minutes */
              (info->player_pos / 8000) % 60,      /* current seconds */
              (info->decoded_len / 8000 / 2) / 60, /* decoded minutes */
              (info->decoded_len / 8000 / 2) % 60  /* decoded seconds */
    );

    gint perc = ((float) (info->player_pos * 2) / info->decoded_len) * 100;
    if (perc > 0 && perc <= 100) {
        mvwhline(window->win, 4, 4, ACS_CKBOARD, ((width - 19) * ((float) perc / 100)));
    }

    return 0;
}

static int
rtp_player_handle_key(Window *window, int key)
{
    // Sanity check, this should not happen
    RtpPlayerInfo *info = rtp_player_win_info(window);
    g_return_val_if_fail(info != NULL, KEY_NOT_HANDLED);

    // Check actions for this key
    enum KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ACTION_UNKNOWN) {
        // Check if we handle this action
        switch (action) {
            case ACTION_RIGHT:
                if (info->player_pos + 2 * 8000 > info->decoded_len / 2) {
                    info->player_pos = info->decoded_len / 2;
                } else {
                    info->player_pos += 2 * 8000;
                }
                break;
            case ACTION_LEFT:
                if (info->player_pos < 2 * 8000) {
                    info->player_pos = 0;
                } else {
                    info->player_pos -= 2 * 8000;
                }
                break;
            case ACTION_UP:
                if (info->player_pos + 10 * 8000 > info->decoded_len / 2) {
                    info->player_pos = info->decoded_len / 2;
                } else {
                    info->player_pos += 10 * 8000;
                }
                break;
            case ACTION_DOWN:
                if (info->player_pos < 10 * 8000) {
                    info->player_pos = 0;
                } else {
                    info->player_pos -= 10 * 8000;
                }
                break;
            case ACTION_BEGIN:
                info->player_pos = 0;
                break;
            case ACTION_END:
                info->player_pos = info->decoded_len / 2;
                break;
            default:
                // Parse next actionº
                continue;
        }

        // We've handled this key, stop checking actions
        break;
    }

    // Return if this panel has handled or not the key
    return (action == ERR) ? KEY_NOT_HANDLED : KEY_HANDLED;
}

static void
rtp_player_write_cb(pa_stream *s, size_t length, gpointer userdata)
{
    RtpPlayerInfo *info = rtp_player_win_info(userdata);
    g_return_if_fail(info != NULL);

    if (info->player_pos * 2 + length > info->decoded_len) {
        info->player_pos = info->decoded_len / 2;
        return;
    }

    if (length > info->decoded_len) {
        length = info->decoded_len;
    }

    pa_stream_write(s, info->decoded + info->player_pos, length, NULL, 0LL, PA_SEEK_RELATIVE);
    if (info->player_pos < info->decoded_len) {
        info->player_pos += length / 2;
    }
}

void
rtp_player_underflow_cb(pa_stream *s, gpointer userdata)
{
    RtpPlayerInfo *info = rtp_player_win_info(userdata);
    g_return_if_fail(info != NULL);

    info->underflow++;
    if (info->underflow >= 6 && info->latency < 2000000) {
        info->latency = (info->latency * 3) / 2;
        info->bufattr.maxlength = (guint32) pa_usec_to_bytes(info->latency, &info->ss);
        info->bufattr.tlength = (guint32) pa_usec_to_bytes(info->latency, &info->ss);
        pa_stream_set_buffer_attr(s, &info->bufattr, NULL, NULL);
        info->underflow = 0;
    }
}

void
rtp_player_set_stream(Window *window, RtpStream *stream)
{
    RtpPlayerInfo *info = rtp_player_win_info(window);
    g_return_if_fail(info != NULL);
    g_return_if_fail(stream != NULL);

    info->stream = stream;

    GByteArray *rtp_payload = g_byte_array_new();

    for (guint i = 0; i < g_ptr_array_len(stream->packets); i++) {
        Packet *packet = g_ptr_array_index(stream->packets, i);
        PacketRtpData *rtp = g_ptr_array_index(packet->proto, PACKET_RTP);
        g_byte_array_append(rtp_payload, rtp->payload->data, rtp->payload->len);
    }

    switch (stream->fmtcode) {
        case RTP_CODEC_G711A:
            info->decoded = codec_g711a_decode(rtp_payload, &info->decoded_len);
            break;
#ifdef WITH_G729
        case RTP_CODEC_G729:
            info->decoded = codec_g729_decode(rtp_payload, &info->decoded_len);
            break;
#endif
        default:
            dialog_run("Unsupported RTP payload type %d", stream->fmtcode);
            return;
    }

    // Failed to decode data
    if (info->decoded == NULL) {
        dialog_run("error: Failed to decode RTP payload");
        return;
    }

    // Stream information
    info->ss.format = PA_SAMPLE_S16NE;
    info->ss.channels = 1;
    info->ss.rate = 8000;

    // Create a new stream with decoded data
    info->playback = pa_stream_new(info->pa_ctx, "sngrep RTP stream", &info->ss, NULL);
    g_return_if_fail(info->playback != NULL);
    pa_stream_set_write_callback(info->playback, rtp_player_write_cb, window);
    pa_stream_set_underflow_callback(info->playback, rtp_player_underflow_cb, window);

    info->latency = 50000;
    info->bufattr.fragsize = (guint32) -1;
    info->bufattr.maxlength = (guint32) pa_usec_to_bytes(info->latency, &info->ss);
    info->bufattr.minreq = (guint32) pa_usec_to_bytes(0, &info->ss);
    info->bufattr.prebuf = (guint32) -1;
    info->bufattr.tlength = (guint32) pa_usec_to_bytes(info->latency, &info->ss);
    pa_stream_connect_playback(info->playback, NULL, &info->bufattr,
                               PA_STREAM_INTERPOLATE_TIMING
                               | PA_STREAM_ADJUST_LATENCY
                               | PA_STREAM_AUTO_TIMING_UPDATE, NULL, NULL);
}

static void
rtp_player_free(Window *window)
{
    RtpPlayerInfo *info = rtp_player_win_info(window);
    g_return_if_fail(info != NULL);

    pa_threaded_mainloop_stop(info->pa_ml);
    pa_stream_disconnect(info->playback);
    pa_context_disconnect(info->pa_ctx);
    pa_context_unref(info->pa_ctx);
    pa_threaded_mainloop_free(info->pa_ml);
    g_free(window);
}

static void
rtp_player_state_cb(pa_context *ctx, gpointer userdata)
{
    RtpPlayerInfo *info = rtp_player_win_info(userdata);
    g_return_if_fail(info != NULL);

    info->pa_state = pa_context_get_state(ctx);
}

Window *
rtp_player_win_new()
{
    Window *window = g_malloc0(sizeof(Window));
    window->type = WINDOW_RTP_PLAYER;
    window->destroy = rtp_player_free;
    window->draw = rtp_player_draw;
    window->handle_key = rtp_player_handle_key;

    // Cerate a new indow for the panel and form
    window_init(window, 9, 68);

    // Initialize save panel specific data
    RtpPlayerInfo *info = g_malloc0(sizeof(RtpPlayerInfo));
    set_panel_userptr(window->panel, (void *) info);

    // Set window boxes
    wattron(window->win, COLOR_PAIR(CP_BLUE_ON_DEF));
    // Window border
    title_foot_box(window->panel);

    // Header and footer lines
    mvwhline(window->win, window->height - 3, 1, ACS_HLINE, window->width - 1);
    mvwaddch(window->win, window->height - 3, 0, ACS_LTEE);
    mvwaddch(window->win, window->height - 3, window->width - 1, ACS_RTEE);
    mvwprintw(window->win, 7, 12, "Use arrow keys to change playback position");
    wattroff(window->win, COLOR_PAIR(CP_BLUE_ON_DEF));

    mvwprintw(window->win, 1, 27, "RTP Stream Player");

    // Create Pulseadio Main loop
    info->pa_ml = pa_threaded_mainloop_new();
    info->pa_mlapi = pa_threaded_mainloop_get_api(info->pa_ml);
    info->pa_ctx = pa_context_new(info->pa_mlapi, "sngrep RTP Player");
    pa_context_connect(info->pa_ctx, NULL, 0, NULL);
    pa_context_set_state_callback(info->pa_ctx, rtp_player_state_cb, window);
    pa_threaded_mainloop_start(info->pa_ml);

    return window;
}
