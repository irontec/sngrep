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
    RtpStream *stream;
    //! Background thread where loop runs
    GThread *player_thread;
    //! Glib Pulseaudio Main loop
    pa_mainloop *pa_ml;
    //! Glib Pulseuadio Main loop Context
    pa_context *pa_ctx;
    //! Glib Pulseaudio Main loop api
    pa_mainloop_api *pa_mlapi;
    //! Pulseaudio Player information
    pa_stream *playback;

    //! Decoded RTP data
    gint16 *decoded;
    //! Decoded RTP data length
    gsize decoded_len;
    //! Current player position
    gsize player_pos;
};

void
rtp_player_set_stream(Window *window, RtpStream *stream);

Window *
rtp_player_win_new();

#endif // __SNGREP_RTP_PLAYER_WIN_H
