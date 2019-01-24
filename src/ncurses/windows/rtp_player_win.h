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
#ifndef __SNGREP_RTP_PLAYER_WIN_H
#define __SNGREP_RTP_PLAYER_WIN_H

#include <glib.h>
#include <pulse/pulseaudio.h>
#include "ncurses/manager.h"
#include "stream.h"

typedef struct _RtpPlayerInfo RtpPlayerInfo;

/**
 * @brief Rtp Player Window information
 *
 * This data stores the actual status of the panel. It's stored in the
 * PANEL user pointer.
 */
struct _RtpPlayerInfo
{
    //! Stream to be played
    Stream *stream;
    //! Glib Pulseaudio Main loop
    pa_threaded_mainloop *pa_ml;
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
rtp_player_set_stream(Window *window, Stream *stream);

Window *
rtp_player_win_new();

#endif // __SNGREP_RTP_PLAYER_WIN_H
