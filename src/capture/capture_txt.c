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
#include "glib/glib-extra.h"
#include "storage/datetime.h"
#include "storage/packet/packet.h"
#include "storage/packet/packet_sip.h"
#include "capture/capture_txt.h"

G_DEFINE_TYPE(CaptureOutputTxt, capture_output_txt, CAPTURE_TYPE_OUTPUT)

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

    CaptureOutputTxt *txt = CAPTURE_OUTPUT_TXT(output);
    g_return_if_fail(txt != NULL);
    g_return_if_fail(txt->file != NULL);

    // Get packet first frame
    PacketFrame *frame = g_list_nth_data(packet->frames, 0);
    g_return_if_fail(frame != NULL);

    // Get packet data
    g_autoptr(GDateTime) frame_date = g_date_time_new_from_timeval(
        packet_frame_seconds(frame),
        packet_frame_microseconds(frame)
    );
    date_time_date_to_str(frame_date, date);
    date_time_time_to_str(frame_date, time);
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
    CaptureOutputTxt *txt = CAPTURE_OUTPUT_TXT(output);
    g_return_if_fail(txt != NULL);
    g_return_if_fail(txt->file != NULL);

    fclose(txt->file);
}

CaptureOutput *
capture_output_txt(const gchar *filename, GError **error)
{
    // Allocate private output data
    CaptureOutputTxt *txt = g_object_new(CAPTURE_TYPE_OUTPUT_TXT, NULL);

    if (!(txt->file = fopen(filename, "w"))) {
        g_set_error(error,
                    CAPTURE_TXT_ERROR,
                    CAPTURE_TXT_ERROR_OPEN,
                    "Unable to open file: %s",
                    g_strerror(errno));
        return NULL;
    }

    return CAPTURE_OUTPUT(txt);
}

static void
capture_output_txt_class_init(CaptureOutputTxtClass *klass)
{
    CaptureOutputClass *capture_output_class = CAPTURE_OUTPUT_CLASS(klass);
    capture_output_class->write = capture_output_txt_write;
    capture_output_class->close = capture_output_txt_close;
}

static void
capture_output_txt_init(CaptureOutputTxt *self)
{
    capture_output_set_tech(CAPTURE_OUTPUT(self), CAPTURE_TECH_TXT);
}

