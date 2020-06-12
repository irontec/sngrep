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
#include "glib-extra/glib.h"
#include "storage/datetime.h"
#include "packet/packet.h"
#include "packet/packet_sip.h"
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

    date_time_date_to_str(packet_frame_microseconds(frame), date);
    date_time_time_to_str(packet_frame_microseconds(frame), time);

    Address src = packet_src_address(packet);
    Address dst = packet_dst_address(packet);

    g_autofree const gchar *payload = packet_sip_payload_str(packet);

    fprintf(txt->file, "%s %s %s:%u -> %s:%u\n%s\n\n",
            date, time,
            address_get_ip(src),
            address_get_port(src),
            address_get_ip(dst),
            address_get_port(dst),
            payload
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
    capture_output_set_sink(CAPTURE_OUTPUT(txt), filename);

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

