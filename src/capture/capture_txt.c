/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2019 Ivan Alonso (Kaian)
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
 * @file capture_txt.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in pcap.h
 *
 * sngrep can parse a pcap file to display call flows.
 * This file include the functions that uses libpcap to do so.
 *
 */
#include "config.h"
#include <glib.h>
#include <errno.h>
#include <stdio.h>
#include "storage/datetime.h"
#include "parser/packet.h"
#include "parser/dissectors/packet_sip.h"
#include "capture/capture_txt.h"

GQuark
capture_txt_error_quark()
{
    return g_quark_from_static_string("capture-txt");
}

static void
capture_output_txt_write(CaptureOutput *output, Packet *packet)
{
    gchar date[20], time[20];

    g_return_if_fail(packet != NULL);

    CaptureTxt *txt = output->priv;
    g_return_if_fail(txt != NULL);
    g_return_if_fail(txt->file != NULL);

    // Get packet first frame
    PacketFrame *frame = g_list_nth_data(packet->frames, 0);
    g_return_if_fail(frame != NULL);

    // Get packet data
    date_time_date_to_str(frame->ts, date);
    date_time_time_to_str(frame->ts, time);
    Address *src = packet_src_address(packet);
    g_return_if_fail(src != NULL);
    Address *dst = packet_dst_address(packet);
    g_return_if_fail(dst != NULL);

    fprintf(txt->file, "%s %s %s:%u -> %s:%u\n%s\n\n",
            date, time, src->ip, src->port, dst->ip, dst->port,
            packet_sip_payload(packet)
    );
}

static void
capture_output_txt_close(CaptureOutput *output)
{
    CaptureTxt *txt = output->priv;
    g_return_if_fail(txt != NULL);
    g_return_if_fail(txt->file != NULL);

    fclose(txt->file);
}

CaptureOutput *
capture_output_txt(const gchar *filename, GError **error)
{
    // Allocate private output data
    CaptureTxt *txt = g_malloc0(sizeof(CaptureTxt));

    if (!(txt->file = fopen(filename, "w"))) {
        g_set_error(error,
                    CAPTURE_TXT_ERROR,
                    CAPTURE_TXT_ERROR_OPEN,
                    "Unable to open file: %s",
                    g_strerror(errno));
        return NULL;
    }

    // Create a new structure to handle this capture dumper
    CaptureOutput *output = g_malloc0(sizeof(CaptureOutput));
    output->priv = txt;
    output->write = capture_output_txt_write;
    output->close = capture_output_txt_close;
    return output;
}
