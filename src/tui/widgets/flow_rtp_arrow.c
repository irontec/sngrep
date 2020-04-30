/**************************************************************************
 **
 ** sngrep - SIP Messages flow arrow
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
/**
 * @file flow_rtp_arrow.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#include "config.h"
#include <glib.h>
#include <glib-object.h>
#include "setting.h"
#include "tui/widgets/flow_rtp_arrow.h"

enum
{
    PROP_MESSAGE = 1,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_TYPE(SngFlowRtpArrow, sng_flow_rtp_arrow, SNG_TYPE_FLOW_ARROW)

SngWidget *
sng_flow_rtp_arrow_new(const Stream *stream)
{
    return g_object_new(
        SNG_TYPE_FLOW_RTP_ARROW,
        "stream", stream,
        NULL
    );
}

Stream *
sng_flow_rtp_arrow_get_stream(SngFlowRtpArrow *rtp_arrow)
{
    return rtp_arrow->stream;
}

static guint64
sng_flow_rtp_arrow_get_time(SngFlowArrow *arrow)
{
    guint64 ts = 0;
    g_return_val_if_fail(arrow != NULL, ts);

    SngFlowRtpArrow *rtp_arrow = SNG_FLOW_RTP_ARROW(arrow);
    g_return_val_if_fail(rtp_arrow->stream != NULL, ts);

    return stream_time(rtp_arrow->stream);
}

static gint
sng_flow_rtp_arrow_get_preferred_height(G_GNUC_UNUSED SngWidget *widget)
{
    if (setting_get_enum(SETTING_TUI_CF_SDP_INFO) == SETTING_SDP_COMPRESSED)
        return 1;
    if (setting_disabled(SETTING_TUI_CF_MEDIA))
        return 0;
    return 2;
}

static void
sng_flow_rtp_arrow_draw(SngWidget *widget)
{
    char text[50], time[20];
    Message *msg;
    SngFlowArrow *arrow = SNG_FLOW_ARROW(widget);
    SngFlowRtpArrow *rtp_arrow = SNG_FLOW_RTP_ARROW(widget);

    Stream *stream = sng_flow_rtp_arrow_get_stream(rtp_arrow);

    // Calculate how many lines this message requires
    gint height = sng_widget_get_height(widget);

    // Get arrow text
    g_autofree const gchar *stream_format = stream_get_format(stream);
    guint stream_count = stream_get_count(stream);
    sprintf(text, "RTP (%s) %d", stream_format, stream_count);

    // Message with Stream destination configured in SDP content
    msg = stream->msg;

//    // Reuse the msg arrow columns as destination column
//    if ((msgarrow = sng_flow_viewer_arrow_find(flow_viewer, msg))) {
//        if (address_equals(msgarrow->scolumn->addr, stream->src))
//            arrow->scolumn = msgarrow->scolumn;
//        if (address_equals(msgarrow->scolumn->addr, stream->dst))
//            arrow->dcolumn = msgarrow->scolumn;
//        if (address_equals(msgarrow->dcolumn->addr, stream->src))
//            arrow->scolumn = msgarrow->dcolumn;
//        if (address_equals(msgarrow->dcolumn->addr, stream->dst))
//            arrow->dcolumn = msgarrow->dcolumn;
//    }
//
//    // fallback: Just use any column that have the destination IP printed
//    if (arrow->dcolumn == NULL) {
//        arrow->dcolumn =
//            sng_flow_viewer_column_get_first(flow_viewer, address_strip_port(stream->dst));
//    }
//
//    if (arrow->scolumn == NULL) {
//        arrow->scolumn =
//            sng_flow_viewer_column_get_first(flow_viewer, address_strip_port(stream->src));
//    }
//
//    // Determine start and end position of the arrow line
//    int startpos, endpos;
//    if (arrow->scolumn->pos < arrow->dcolumn->pos) {
//        arrow->dir = CF_ARROW_DIR_RIGHT;
//        startpos = 20 + arrow->scolumn->pos;
//        endpos = 20 + arrow->dcolumn->pos;
//    } else {
//        arrow->dir = CF_ARROW_DIR_LEFT;
//        startpos = 20 + arrow->dcolumn->pos;
//        endpos = 20 + arrow->scolumn->pos;
//    }
//    int distance = 0;
//
//    if (startpos != endpos) {
//        // In compressed mode, we display the src and dst port inside the arrow
//        // so fixup the stard and end position
//        if (setting_get_enum(SETTING_TUI_CF_SDP_INFO) != SETTING_SDP_COMPRESSED) {
//            startpos += 5;
//            endpos -= 5;
//        }
//        distance = abs(endpos - startpos) - 4 + 1;
//    } else {
//        // Fix port positions
//        startpos -= 2;
//        endpos += 2;
//        distance = 1;
//
//        // Fix arrow direction based on ports
//        if (address_get_port(stream->src) < address_get_port(stream->dst)) {
//            arrow->dir = CF_ARROW_DIR_RIGHT;
//        } else {
//            arrow->dir = CF_ARROW_DIR_LEFT;
//        }
//    }
//
//    // Highlight current message
//    if (arrow == g_ptr_array_index(flow_viewer->darrows, flow_viewer->cur_idx)) {
//        switch (setting_get_enum(SETTING_TUI_CF_HIGHTLIGHT)) {
//            case SETTING_ARROW_HIGHLIGH_BOLD:
//                wattron(flow_viewer->arrows_pad, A_BOLD);
//                break;
//            case SETTING_ARROW_HIGHLIGH_REVERSE:
//                wattron(flow_viewer->arrows_pad, A_REVERSE);
//                break;
//            case SETTING_ARROW_HIGHLIGH_REVERSEBOLD:
//                wattron(flow_viewer->arrows_pad, A_REVERSE);
//                wattron(flow_viewer->arrows_pad, A_BOLD);
//                break;
//        }
//    }
//
//    // Check if displayed stream is active
//    gboolean active = stream_is_active(stream);
//
//    // Clear the line
//    mvwprintw(flow_viewer->arrows_pad, cline, startpos + 2, "%*s", distance, "");
//    // Draw RTP arrow text
//    mvwprintw(flow_viewer->arrows_pad, cline, startpos + (distance) / 2 - strlen(text) / 2 + 2, "%s", text);
//
//    if (setting_get_enum(SETTING_TUI_CF_SDP_INFO) != SETTING_SDP_COMPRESSED)
//        cline++;
//
//    // Draw line between columns
//    if (active)
//        mvwhline(flow_viewer->arrows_pad, cline, startpos + 2, '-', distance);
//    else
//        mvwhline(flow_viewer->arrows_pad, cline, startpos + 2, ACS_HLINE, distance);
//
//    // Write the arrow at the end of the message (two arrows if this is a retrans)
//    if (arrow->dir == CF_ARROW_DIR_RIGHT) {
//        if (setting_get_enum(SETTING_TUI_CF_SDP_INFO) != SETTING_SDP_COMPRESSED) {
//            mvwprintw(flow_viewer->arrows_pad, cline, startpos - 4, "%d", address_get_port(stream->src));
//            mvwprintw(flow_viewer->arrows_pad, cline, endpos, "%d", address_get_port(stream->dst));
//        }
//        mvwaddwstr(flow_viewer->arrows_pad, cline, endpos - 2, tui_acs_utf8('>'));
//        if (active) {
//            arrow->rtp_count = stream_get_count(stream);
//            arrow->rtp_ind_pos = (arrow->rtp_ind_pos + 1) % distance;
//            mvwaddwstr(flow_viewer->arrows_pad, cline, startpos + arrow->rtp_ind_pos + 2, tui_acs_utf8('>'));
//        }
//    } else {
//        if (setting_get_enum(SETTING_TUI_CF_SDP_INFO) != SETTING_SDP_COMPRESSED) {
//            mvwprintw(flow_viewer->arrows_pad, cline, endpos, "%d", address_get_port(stream->src));
//            mvwprintw(flow_viewer->arrows_pad, cline, startpos - 4, "%d", address_get_port(stream->dst));
//        }
//        mvwaddwstr(flow_viewer->arrows_pad, cline, startpos + 2, tui_acs_utf8('<'));
//        if (active) {
//            arrow->rtp_count = stream_get_count(stream);
//            arrow->rtp_ind_pos = (arrow->rtp_ind_pos + 1) % distance;
//            mvwaddwstr(flow_viewer->arrows_pad, cline, endpos - arrow->rtp_ind_pos - 2, tui_acs_utf8('<'));
//        }
//    }
//
//    if (setting_get_enum(SETTING_TUI_CF_SDP_INFO) == SETTING_SDP_COMPRESSED)
//        mvwprintw(flow_viewer->arrows_pad, cline, startpos + (distance) / 2 - strlen(text) / 2 + 2, " %s ", text);
//
//    wattroff(flow_viewer->arrows_pad, A_BOLD | A_REVERSE);
//
//    // Print timestamp
//    if (flow_viewer->arrowtime) {
//        date_time_time_to_str(stream_time(stream), time);
//        if (arrow == g_ptr_array_index(flow_viewer->darrows, flow_viewer->cur_idx)) {
//            wattron(flow_viewer->arrows_pad, A_BOLD);
//            mvwprintw(flow_viewer->arrows_pad, cline - 1, 2, "%s", time);
//            wattroff(flow_viewer->arrows_pad, A_BOLD);
//        } else {
//            mvwprintw(flow_viewer->arrows_pad, cline - 1, 2, "%s", time);
//        }
//
//    }
//
//    return arrow->height;
}

static const gchar *
sng_flow_rtp_get_detail(SngFlowArrow *arrow)
{
    SngFlowRtpArrow *rtp_arrow = SNG_FLOW_RTP_ARROW(arrow);
    g_return_val_if_fail(rtp_arrow != NULL, NULL);
    Stream *stream = rtp_arrow->stream;
    g_return_val_if_fail(stream != NULL, NULL);

    GString *detail = g_string_new(NULL);
    g_string_append(detail, "RTP Stream Analysis\n");
    g_string_append(detail, "――――――――――――――――――――――――――――――――――――");
    g_string_append_printf(detail, "Source: %s:%hu\n",
                           address_get_ip(stream->src), address_get_port(stream->src));
    g_string_append_printf(detail, "Destination: %s:%hu\n",
                           address_get_ip(stream->dst), address_get_port(stream->dst));
    g_string_append_printf(detail, "SSRC: 0x%X\n", stream->ssrc);
    g_string_append_printf(detail, "Packets: %d / %d\n", stream->packet_count, stream->stats.expected);
    g_string_append_printf(detail, "Lost: %d (%.1f%%)\n", stream->stats.lost,
                           (gdouble) stream->stats.lost / stream->stats.expected * 100);
    g_string_append_printf(detail, "Out of sequence: %d (%.1f%%)\n", stream->stats.oos,
                           (gdouble) stream->stats.oos / stream->packet_count * 100);
    g_string_append_printf(detail, "Max Delta: %.2f ms\n", stream->stats.max_delta);
    g_string_append_printf(detail, "Max Jitter: %.2f ms\n", stream->stats.max_jitter);
    g_string_append_printf(detail, "Mean Jitter: %.2f ms\n", stream->stats.mean_jitter);
    g_string_append_printf(detail, "Problems: %s\n", (stream->stats.lost) ? "Yes" : "No");
    g_string_append(detail, "RTCP VoIP Metrics Report\n");
    g_string_append(detail, "――――――――――――――――――――――――――――――――――――\n");
    return detail->str;
}

static void
sng_flow_rtp_arrow_set_property(GObject *self, guint property_id, const GValue *value, GParamSpec *pspec)
{
    SngFlowRtpArrow *rtp_arrow = SNG_FLOW_RTP_ARROW(self);

    switch (property_id) {
        case PROP_MESSAGE:
            rtp_arrow->stream = g_value_get_pointer(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}

static void
sng_flow_rtp_arrow_get_property(GObject *self, guint property_id, GValue *value, GParamSpec *pspec)
{
    SngFlowRtpArrow *rtp_arrow = SNG_FLOW_RTP_ARROW(self);
    switch (property_id) {
        case PROP_MESSAGE:
            g_value_set_pointer(value, rtp_arrow->stream);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}

static void
sng_flow_rtp_arrow_class_init(SngFlowRtpArrowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = sng_flow_rtp_arrow_get_property;
    object_class->set_property = sng_flow_rtp_arrow_set_property;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->draw = sng_flow_rtp_arrow_draw;
    widget_class->preferred_height = sng_flow_rtp_arrow_get_preferred_height;

    SngFlowArrowClass *arrow_class = SNG_FLOW_ARROW_CLASS(klass);
    arrow_class->get_time = sng_flow_rtp_arrow_get_time;
    arrow_class->get_detail = sng_flow_rtp_get_detail;

    obj_properties[PROP_MESSAGE] =
        g_param_spec_pointer("stream",
                             "Arrow RTP stream",
                             "Arrow RTP stream",
                             G_PARAM_CONSTRUCT | G_PARAM_READWRITE
        );

    g_object_class_install_properties(
        object_class,
        N_PROPERTIES,
        obj_properties
    );
}

static void
sng_flow_rtp_arrow_init(G_GNUC_UNUSED SngFlowRtpArrow *rtp_arrow)
{
}
