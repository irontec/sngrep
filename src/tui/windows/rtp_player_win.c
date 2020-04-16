/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2019 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2019 Irontec SL. All rights reserved.
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
#include "codecs/codec.h"
#include "tui/keybinding.h"
#include "tui/theme.h"
#include "tui/dialog.h"
#include "rtp_player_win.h"

G_DEFINE_TYPE(RtpPlayerWindow, rtp_player_win, SNG_TYPE_WINDOW)

static void
rtp_player_win_decode_stream(SngWindow *window, Stream *stream)
{
    RtpPlayerWindow *self = TUI_RTP_PLAYER(window);
    g_return_if_fail(self != NULL);

    GError *error = NULL;
    self->decoded = codec_stream_decode(stream, self->decoded, &error);

    if (error != NULL) {
        dialog_run("error: %s", error->message);
        return;
    }
}

static gint
rtp_player_win_draw(SngWidget *widget)
{
    SngWindow *window = SNG_WINDOW(widget);
    RtpPlayerWindow *self = TUI_RTP_PLAYER(window);
    g_return_val_if_fail(self != NULL, 1);
    WINDOW *win = sng_window_get_ncurses_window(window);

    if (getenv("PULSE_SERVER")) {
        mvwprintw(win, 6, 3, "Server: %s", getenv("PULSE_SERVER"));
    } else {
        mvwprintw(win, 6, 3, "Server: Local");
    }

    mvwprintw(win, 6, 30, "Server: Local");
    switch (self->pa_state) {
        case PA_CONTEXT_TERMINATED:
        case PA_CONTEXT_FAILED:
            wattron(win, COLOR_PAIR(CP_RED_ON_DEF));
            mvwprintw(win, 6, 38, "Error     ");
            wattroff(win, COLOR_PAIR(CP_RED_ON_DEF));
            if (getenv("PULSE_SERVER")) {
                dialog_run("Unable to connect to pulseaudio server at %s.\n"
                           "Maybe you need to allow remote connections by running: \n\n"
                           "pactl load-module module-native-protocol-tcp auth-anonymous=1",
                           getenv("PULSE_SERVER")
                );
                return 1;
            }
            break;
        case PA_CONTEXT_READY:
            wattron(win, COLOR_PAIR(CP_GREEN_ON_DEF));
            mvwprintw(win, 6, 38, "Ready     ");
            wattroff(win, COLOR_PAIR(CP_GREEN_ON_DEF));
            break;
        default:
            mvwprintw(win, 6, 38, "Connecting");
            break;
    }

    if (self->pa_state == PA_CONTEXT_READY && !self->connected) {
        pa_stream_connect_playback(self->playback, NULL, &self->bufattr,
                                   PA_STREAM_INTERPOLATE_TIMING
                                   | PA_STREAM_ADJUST_LATENCY
                                   | PA_STREAM_AUTO_TIMING_UPDATE, NULL, NULL);
        self->connected = true;
    }

    mvwprintw(win, 6, 50, "Latency: %d ms", self->latency / 1000);

    if (self->stream->changed) {
        rtp_player_win_decode_stream(window, self->stream);
        self->stream->changed = FALSE;
    }

    if (self->decoded->len == 0) {
        return 1;
    }

    gint width = getmaxx(win);
    mvwhline(win, 4, 4, '-', width - 19);
    mvwaddch(win, 4, 3, '[');
    mvwaddch(win, 4, width - 15, ']');

    mvwprintw(win, 4, width - 13, "%02d:%02d/%02d:%02d",
              (self->player_pos / 8000) / 60,      /* current minutes */
              (self->player_pos / 8000) % 60,      /* current seconds */
              (self->decoded->len / 8000 / 2) / 60, /* decoded minutes */
              (self->decoded->len / 8000 / 2) % 60  /* decoded seconds */
    );

    gint perc = ((float) (self->player_pos * 2) / self->decoded->len) * 100;
    if (perc > 0 && perc <= 100) {
        mvwhline(win, 4, 4, ACS_CKBOARD, ((width - 19) * ((float) perc / 100)));
    }

    return 0;
}

static int
rtp_player_win_handle_key(SngWidget *widget, int key)
{
    // Sanity check, this should not happen
    SngWindow *window = SNG_WINDOW(widget);
    RtpPlayerWindow *self = TUI_RTP_PLAYER(window);
    g_return_val_if_fail(self != NULL, KEY_NOT_HANDLED);

    // Check actions for this key
    KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ACTION_UNKNOWN) {
        // Check if we handle this action
        switch (action) {
            case ACTION_RIGHT:
                if (self->player_pos + 2 * 8000 > self->decoded->len / 2) {
                    self->player_pos = self->decoded->len / 2;
                } else {
                    self->player_pos += 2 * 8000;
                }
                break;
            case ACTION_LEFT:
                if (self->player_pos < 2 * 8000) {
                    self->player_pos = 0;
                } else {
                    self->player_pos -= 2 * 8000;
                }
                break;
            case ACTION_UP:
                if (self->player_pos + 10 * 8000 > self->decoded->len / 2) {
                    self->player_pos = self->decoded->len / 2;
                } else {
                    self->player_pos += 10 * 8000;
                }
                break;
            case ACTION_DOWN:
                if (self->player_pos < 10 * 8000) {
                    self->player_pos = 0;
                } else {
                    self->player_pos -= 10 * 8000;
                }
                break;
            case ACTION_BEGIN:
                self->player_pos = 0;
                break;
            case ACTION_END:
                self->player_pos = self->decoded->len / 2;
                break;
            default:
                // Parse next action
                continue;
        }

        // We've handled this key, stop checking actions
        break;
    }

    // Return if this panel has handled or not the key
    return (action == ERR) ? KEY_NOT_HANDLED : KEY_HANDLED;
}

static void
rtp_player_win_write_cb(pa_stream *s, size_t length, gpointer userdata)
{
    RtpPlayerWindow *self = TUI_RTP_PLAYER(userdata);
    g_return_if_fail(self != NULL);

    if (self->player_pos * 2 + length > self->decoded->len) {
        self->player_pos = self->decoded->len / 2;
        return;
    }

    if (length > self->decoded->len) {
        length = self->decoded->len;
    }

    pa_stream_write(s, ((gint16 *) self->decoded->data) + self->player_pos, length, NULL, 0LL, PA_SEEK_RELATIVE);
    if (self->player_pos < self->decoded->len) {
        self->player_pos += length / 2;
    }
}

void
rtp_player_win_underflow_cb(pa_stream *s, gpointer userdata)
{
    RtpPlayerWindow *self = TUI_RTP_PLAYER(userdata);
    g_return_if_fail(self != NULL);

    self->underflow++;
    if (self->underflow >= 6 && self->latency < 2000000) {
        self->latency = (self->latency * 3) / 2;
        self->bufattr.maxlength = (guint32) pa_usec_to_bytes(self->latency, &self->ss);
        self->bufattr.tlength = (guint32) pa_usec_to_bytes(self->latency, &self->ss);
        pa_stream_set_buffer_attr(s, &self->bufattr, NULL, NULL);
        self->underflow = 0;
    }
}

void
rtp_player_win_set_stream(SngWindow *window, Stream *stream)
{
    RtpPlayerWindow *self = TUI_RTP_PLAYER(window);
    g_return_if_fail(self != NULL);
    g_return_if_fail(stream != NULL);

    self->stream = stream;
    rtp_player_win_decode_stream(window, stream);

    // Stream information
    self->ss.format = PA_SAMPLE_S16NE;
    self->ss.channels = 1;
    self->ss.rate = 8000;

    // Create a new stream with decoded data
    self->playback = pa_stream_new(self->pa_ctx, "sngrep RTP stream", &self->ss, NULL);
    g_return_if_fail(self->playback != NULL);
    pa_stream_set_write_callback(self->playback, rtp_player_win_write_cb, window);
    pa_stream_set_underflow_callback(self->playback, rtp_player_win_underflow_cb, window);

    self->latency = 20000;
    self->bufattr.fragsize = (guint32) -1;
    self->bufattr.maxlength = (guint32) pa_usec_to_bytes(self->latency, &self->ss);
    self->bufattr.minreq = (guint32) pa_usec_to_bytes(0, &self->ss);
    self->bufattr.prebuf = (guint32) -1;
    self->bufattr.tlength = (guint32) pa_usec_to_bytes(self->latency, &self->ss);
}

void
rtp_player_win_free(SngWindow *window)
{
    g_object_unref(window);
}

static void
rtp_player_win_finalize(GObject *object)
{
    RtpPlayerWindow *self = TUI_RTP_PLAYER(object);
    g_return_if_fail(self != NULL);

    pa_stream_disconnect(self->playback);
    pa_context_disconnect(self->pa_ctx);
    pa_context_unref(self->pa_ctx);
    pa_glib_mainloop_free(self->pa_ml);
    g_byte_array_free(self->decoded, TRUE);

    // Chain-up parent finalize function
    G_OBJECT_CLASS(rtp_player_win_parent_class)->finalize(object);
}

static void
rtp_player_win_state_cb(pa_context *ctx, gpointer userdata)
{
    RtpPlayerWindow *self = TUI_RTP_PLAYER(userdata);
    g_return_if_fail(self != NULL);

    self->pa_state = pa_context_get_state(ctx);
}

SngWindow *
rtp_player_win_new()
{
    return g_object_new(
        WINDOW_TYPE_RTP_PLAYER,
        "height", 11,
        "width", 68,
        NULL
    );
}

static void
rtp_player_win_constructed(GObject *object)
{
    // Chain-up parent constructed
    G_OBJECT_CLASS(rtp_player_win_parent_class)->constructed(object);

    RtpPlayerWindow *self = TUI_RTP_PLAYER(object);
    SngWindow *parent = SNG_WINDOW(self);
    WINDOW *win = sng_window_get_ncurses_window(parent);
    PANEL *panel = sng_window_get_ncurses_panel(parent);

    gint height = sng_window_get_height(parent);
    gint width = sng_window_get_width(parent);

    // Set window boxes
    wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    // Window border
    title_foot_box(panel);

    // Header and footer lines
    mvwhline(win, height - 3, 1, ACS_HLINE, width - 1);
    mvwaddch(win, height - 3, 0, ACS_LTEE);
    mvwaddch(win, height - 3, width - 1, ACS_RTEE);
    mvwprintw(win, height - 2, 12, "Use arrow keys to change playback position");
    wattroff(win, COLOR_PAIR(CP_BLUE_ON_DEF));

    mvwprintw(win, 1, 27, "RTP Stream Player");

    // Setup Pulseaudio server based on environment variables
    if (getenv("PULSE_SERVER") == NULL) {
        // Try to detect origin IP from SSH_CLIENT environment
        gchar *ssh_client_str = getenv("SSH_CLIENT");
        if (ssh_client_str != NULL) {
            gchar **ssh_data = g_strsplit(ssh_client_str, " ", 2);

            if (g_strv_length(ssh_data) > 0) {
                setenv("PULSE_SERVER", ssh_data[0], FALSE);
            }
            g_strfreev(ssh_data);
        }
    }

    // Create decoded data array
    self->decoded = g_byte_array_new();

    // Create Pulseaudio Main loop
    self->pa_ml = pa_glib_mainloop_new(NULL);
    self->pa_mlapi = pa_glib_mainloop_get_api(self->pa_ml);
    self->pa_ctx = pa_context_new(self->pa_mlapi, "sngrep RTP Player");
    pa_context_connect(self->pa_ctx, NULL, 0, NULL);
    pa_context_set_state_callback(self->pa_ctx, rtp_player_win_state_cb, parent);
}

static void
rtp_player_win_class_init(RtpPlayerWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = rtp_player_win_constructed;
    object_class->finalize = rtp_player_win_finalize;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->draw = rtp_player_win_draw;
    widget_class->key_pressed = rtp_player_win_handle_key;

}

static void
rtp_player_win_init(RtpPlayerWindow *self)
{
    // Initialize attributes
    sng_window_set_window_type(SNG_WINDOW(self), WINDOW_RTP_PLAYER);
}
