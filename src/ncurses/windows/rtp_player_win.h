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
#ifndef __SNGREP_RTP_PLAYER_WIN_H__
#define __SNGREP_RTP_PLAYER_WIN_H__

#include <glib.h>
#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include "ncurses/manager.h"
#include "storage/stream.h"

G_BEGIN_DECLS

#define WINDOW_TYPE_RTP_PLAYER rtp_player_win_get_type()
G_DECLARE_FINAL_TYPE(RtpPlayerWindow, rtp_player_win, NCURSES, RTP_PLAYER, Window)

/**
 * @brief Rtp Player Window information
 *
 * This data stores the actual status of the panel. It's stored in the
 * PANEL user pointer.
 */
struct _RtpPlayerWindow
{
    //! Parent object attributes
    Window parent;
    //! Stream to be played
    Stream *stream;
    //! Glib Pulseaudio Main loop
    pa_glib_mainloop *pa_ml;
    //! Glib Pulseuadio Main loop Context
    pa_context *pa_ctx;
    //! Glib Pulseaudio Main loop api
    pa_mainloop_api *pa_mlapi;
    //! Pulseaudio Player information
    pa_stream *playback;
    //! Pulseadio Player connected flag
    gboolean connected;

    //! Last packet index parsed of the stream
    guint last_packet;
    //! Decoded RTP data
    GByteArray *decoded;
    //! Current player position
    gsize player_pos;
    //! Player context status
    pa_context_state_t pa_state;
    //! Player stream information
    pa_sample_spec ss;
    //! Player current latency
    pa_usec_t latency;
    //! Player buffer attributes
    pa_buffer_attr bufattr;
    //! Underflow counter
    gint underflow;

};

void
rtp_player_win_set_stream(Window *window, Stream *stream);

void
rtp_player_win_free(Window *window);

Window *
rtp_player_win_new();

#endif /* __SNGREP_RTP_PLAYER_WIN_H__ */
