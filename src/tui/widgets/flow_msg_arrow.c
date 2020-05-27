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
 * @file flow_msg_arrow.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#include "config.h"
#include <glib.h>
#include <ncurses.h>
#include <glib-object.h>
#include "setting.h"
#include "tui/widgets/flow_msg_arrow.h"

enum
{
    PROP_MESSAGE = 1,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_TYPE(SngFlowMsgArrow, sng_flow_msg_arrow, SNG_TYPE_FLOW_ARROW)

SngWidget *
sng_flow_msg_arrow_new(const Message *msg)
{
    return g_object_new(
        SNG_TYPE_FLOW_MSG_ARROW,
        "message", msg,
        NULL
    );
}

Message *
sng_flow_msg_arrow_get_message(SngFlowMsgArrow *msg_arrow)
{
    return msg_arrow->msg;
}

static guint64
sng_flow_msg_arrow_get_time(SngFlowArrow *arrow)
{
    guint64 ts = 0;
    g_return_val_if_fail(arrow != NULL, ts);

    SngFlowMsgArrow *msg_arrow = SNG_FLOW_MSG_ARROW(arrow);
    g_return_val_if_fail(msg_arrow->msg != NULL, ts);

    return msg_get_time(msg_arrow->msg);
}

static void
sng_flow_msg_arrow_draw(SngWidget *widget)
{
    // Draw horizontal line below column address
    WINDOW *win = sng_widget_get_ncurses_window(widget);

    SngFlowArrow *arrow = SNG_FLOW_ARROW(widget);
    SngFlowMsgArrow *msg_arrow = SNG_FLOW_MSG_ARROW(widget);
    SngFlowArrowDir direction = sng_flow_arrow_get_direction(arrow);

    // Calculate how many lines this message requires
    gint height = sng_widget_get_height(widget);
    gint width = sng_widget_get_width(widget);

    Message *msg = msg_arrow->msg;
    g_return_if_fail(msg != NULL);

    Call *call = msg_get_call(msg);
    g_return_if_fail(call != NULL);

    // Packet SDP data
    PacketSdpData *sdp_data = packet_sdp_data(msg->packet);
    PacketSdpMedia *media = NULL;
    if (sdp_data != NULL) {
        media = g_list_nth_data(sdp_data->medias, 0);
    }

    // Get Message method (include extra info)
    GString *method = g_string_new(msg_get_method_str(msg));
    g_string_set_size(method, MIN((gint) method->len, width - 4));

    // If message has sdp information
    if (setting_get_enum(SETTING_TUI_CF_SDP_INFO) == SETTING_SDP_OFF
        || setting_get_enum(SETTING_TUI_CF_SDP_INFO) == SETTING_SDP_COMPRESSED) {
        if (msg_has_sdp(msg)) {
            g_string_append(method, " (SDP)");
        }
    }

    if (media && setting_get_enum(SETTING_TUI_CF_SDP_INFO) == SETTING_SDP_FIRST) {
        g_string_printf(
            method,
            "%.3s (%s:%u)",
            msg_get_method_str(msg),
            (media->sconn != NULL) ? media->sconn->address : sdp_data->sconn->address,
            media->rtpport
        );
    }

    if (media && setting_get_enum(SETTING_TUI_CF_SDP_INFO) == SETTING_SDP_FULL) {
        g_string_printf(
            method,
            "%.3s (%s)",
            msg_get_method_str(msg),
            (media->sconn != NULL) ? media->sconn->address : sdp_data->sconn->address
        );
    }

    // Get arrow columns
    SngFlowColumn *scolumn = sng_flow_arrow_get_src_column(arrow);
    SngFlowColumn *dcolumn = sng_flow_arrow_get_dst_column(arrow);
    gint scolumn_xpos = sng_widget_get_xpos(SNG_WIDGET(scolumn));
    gint dcolumn_xpos = sng_widget_get_xpos(SNG_WIDGET(dcolumn));

    // Determine start and end position of the arrow line
    if (scolumn == dcolumn) {
        sng_flow_arrow_set_direction(arrow, CF_ARROW_DIR_SPIRAL_RIGHT);
    } else if (scolumn_xpos < dcolumn_xpos) {
        sng_flow_arrow_set_direction(arrow, CF_ARROW_DIR_RIGHT);
    } else {
        sng_flow_arrow_set_direction(arrow, CF_ARROW_DIR_LEFT);
    }

    // Highlight current message
    if (sng_widget_has_focus(widget)) {
        switch (setting_get_enum(SETTING_TUI_CF_HIGHTLIGHT)) {
            case SETTING_ARROW_HIGHLIGH_BOLD:
                wattron(win, A_BOLD);
                break;
            case SETTING_ARROW_HIGHLIGH_REVERSE:
                wattron(win, A_REVERSE);
                break;
            case SETTING_ARROW_HIGHLIGH_REVERSEBOLD:
                wattron(win, A_REVERSE);
                wattron(win, A_BOLD);
                break;
        }
    }

    // Color the message {
    switch (setting_get_enum(SETTING_TUI_COLORMODE)) {
        case SETTING_COLORMODE_REQUEST:
            // Color by request / response
            wattron(win, COLOR_PAIR((msg_is_request(msg)) ? CP_RED_ON_DEF : CP_GREEN_ON_DEF));
            break;
        case SETTING_COLORMODE_CALLID:
            // Color by call-id
//            wattron(win, call_group_color(flow_viewer->group, msg->call));
            break;
        case SETTING_COLORMODE_CSEQ:
            // Color by CSeq within the same call
            wattron(win, COLOR_PAIR(msg_get_cseq(msg) % 7 + 1));
            break;
    }

    gint arrow_row = MAX(height - 1, 1);
    if (sng_widget_has_focus(widget)) {
        tui_whline(win, arrow_row, 0, ACS_HLINE, width);
    } else if (sng_flow_arrow_is_selected(arrow)) {
        tui_whline(win, arrow_row, 0, '=', width);
    } else {
        tui_whline(win, arrow_row, 0, ACS_HLINE, width);
    }

    // Draw method
    switch (direction) {
        case CF_ARROW_DIR_SPIRAL_RIGHT:
        case CF_ARROW_DIR_SPIRAL_LEFT:
            mvwprintw(win, 0, 5, "%.26s", method->str);
            break;
        default:
            mvwprintw(win, 0, (width - (gint) method->len) / 2, method->str);
            break;
    }

    // Draw arrow
    switch (direction) {
        case CF_ARROW_DIR_SPIRAL_RIGHT:
        case CF_ARROW_DIR_SPIRAL_LEFT:
            break;
        case CF_ARROW_DIR_RIGHT:
            mvwaddwstr(win, arrow_row, width - 1, tui_acs_utf8('>'));
            if (msg_is_retransmission(msg)) {
                mvwaddwstr(win, arrow_row, width - 2, tui_acs_utf8('>'));
                mvwaddwstr(win, arrow_row, width - 3, tui_acs_utf8('>'));
            }
            break;
        case CF_ARROW_DIR_LEFT:
            mvwaddwstr(win, arrow_row, 0, tui_acs_utf8('<'));
            if (msg_is_retransmission(msg)) {
                mvwaddwstr(win, arrow_row, 1, tui_acs_utf8('<'));
                mvwaddwstr(win, arrow_row, 2, tui_acs_utf8('<'));
            }
        default:
            break;
    }




//    // Draw media information
//    if (msg_has_sdp(msg) && setting_get_enum(SETTING_TUI_CF_SDP_INFO) == SETTING_SDP_FULL) {
//        arrow->line += g_list_length(sdp_data->medias);
//        for (GList *l = sdp_data->medias; l != NULL; l = l->next) {
//            arrow->line++;
//            PacketSdpMedia *media = l->data;
//            sprintf(mediastr, "%s %d (%s)",
//                    packet_sdp_media_type_str(media->type),
//                    media->rtpport,
//                    msg_get_preferred_codec_alias(msg));
//            mvwprintw(win, arrow->line, startpos + distance / 2 - strlen(mediastr) / 2 + 2, mediastr);
//        }
//    }
//
//    if (arrow->dir != CF_ARROW_DIR_SPIRAL_RIGHT && arrow->dir != CF_ARROW_DIR_SPIRAL_LEFT) {
//        if (arrow == sng_flow_viewer_arrow_selected(flow_viewer)) {
//            mvwhline(win, arrow->line, startpos + 2, '=', distance);
//        } else {
//            mvwhline(win, arrow->line, startpos + 2, ACS_HLINE, distance);
//        }
//    }
//
//    // Write the arrow at the end of the message (two arrows if this is a retrans)
//    if (arrow->dir == CF_ARROW_DIR_SPIRAL_RIGHT) {
//        mvwaddwstr(win, arrow->line, startpos + 2, tui_acs_utf8('<'));
//        if (msg_is_retransmission(msg)) {
//            mvwaddwstr(win, arrow->line, startpos + 3, tui_acs_utf8('<'));
//            mvwaddwstr(win, arrow->line, startpos + 4, tui_acs_utf8('<'));
//        }
//        // If multiple lines are available, print a spiral icon
//        if (arrow->line != arrow->line) {
//            mvwaddch(win, arrow->line, startpos + 3, ACS_LRCORNER);
//            mvwaddch(win, arrow->line - 1, startpos + 3, ACS_URCORNER);
//            mvwaddch(win, arrow->line - 1, startpos + 2, ACS_HLINE);
//        }
//    } else if (arrow->dir == CF_ARROW_DIR_SPIRAL_LEFT) {
//        mvwaddwstr(win, arrow->line, startpos - 2, tui_acs_utf8('>'));
//        if (msg_is_retransmission(msg)) {
//            mvwaddwstr(win, arrow->line, startpos - 3, tui_acs_utf8('>'));
//            mvwaddwstr(win, arrow->line, startpos - 4, tui_acs_utf8('>'));
//        }
//        // If multiple lines are available, print a spiral icon
//        if (arrow->line != arrow->line) {
//            mvwaddch(win, arrow->line, startpos - 3, ACS_LLCORNER);
//            mvwaddch(win, arrow->line - 1, startpos - 3, ACS_ULCORNER);
//            mvwaddch(win, arrow->line - 1, startpos - 2, ACS_HLINE);
//        }
//    } else if (arrow->dir == CF_ARROW_DIR_RIGHT) {
//        mvwaddwstr(win, arrow->line, endpos - 2, tui_acs_utf8('>'));
//        if (msg_is_retransmission(msg)) {
//            mvwaddwstr(win, arrow->line, endpos - 3, tui_acs_utf8('>'));
//            mvwaddwstr(win, arrow->line, endpos - 4, tui_acs_utf8('>'));
//        }
//    } else {
//        mvwaddwstr(win, arrow->line, startpos + 2, tui_acs_utf8('<'));
//        if (msg_is_retransmission(msg)) {
//            mvwaddwstr(win, arrow->line, startpos + 3, tui_acs_utf8('<'));
//            mvwaddwstr(win, arrow->line, startpos + 4, tui_acs_utf8('<'));
//        }
//    }
//
//    if (setting_get_enum(SETTING_TUI_CF_SDP_INFO) == SETTING_SDP_COMPRESSED)
//        mvwprintw(win, arrow->line, startpos + distance / 2 - msglen / 2 + 2, " %.26s ", method);

    // Turn off colors
    wattroff(win, COLOR_PAIR(CP_RED_ON_DEF));
    wattroff(win, COLOR_PAIR(CP_GREEN_ON_DEF));
    wattroff(win, COLOR_PAIR(CP_CYAN_ON_DEF));
    wattroff(win, COLOR_PAIR(CP_YELLOW_ON_DEF));
    wattroff(win, A_BOLD | A_REVERSE);

//    // Print timestamp
//    if (flow_viewer->arrowtime) {
//        if (arrow == sng_flow_viewer_arrow_selected(flow_viewer))
//            wattron(win, COLOR_PAIR(CP_CYAN_ON_DEF));
//
//        if (arrow == g_ptr_array_index(flow_viewer->darrows, flow_viewer->cur_idx)) {
//            wattron(win, A_BOLD);
//            mvwprintw(win, arrow->line, 2, "%s", msg_time);
//            wattroff(win, A_BOLD);
//        } else {
//            mvwprintw(win, arrow->line, 2, "%s", msg_time);
//        }
//
//        // Print delta from selected message
//        if (setting_get_enum(SETTING_TUI_CF_SDP_INFO) != SETTING_SDP_COMPRESSED) {
//            if (flow_viewer->selected == -1) {
//                if (setting_enabled(SETTING_TUI_CF_DELTA)) {
//                    date_time_to_delta(
//                        msg_get_time(msg),
//                        msg_get_time(call_group_get_next_msg(flow_viewer->group, msg)),
//                        delta
//                    );
//                }
//            } else if (arrow == g_ptr_array_index(flow_viewer->darrows, flow_viewer->cur_idx)) {
//                date_time_to_delta(
//                    msg_get_time(sng_flow_viewer_arrow_message(sng_flow_viewer_arrow_selected(flow_viewer))),
//                    msg_get_time(msg),
//                    delta
//                );
//            }
//
//            if (strlen(delta)) {
//                wattron(win, COLOR_PAIR(CP_CYAN_ON_DEF));
//                mvwprintw(win, arrow->line + 1, 2, "%15s", delta);
//            }
//            wattroff(win, COLOR_PAIR(CP_CYAN_ON_DEF));
//        }
//    }
    wattroff(win, COLOR_PAIR(CP_CYAN_ON_DEF));
}


static gint
sng_flow_msg_arrow_get_preferred_height(SngWidget *widget)
{
    SngFlowMsgArrow *msg_arrow = SNG_FLOW_MSG_ARROW(widget);

    if (setting_enabled(SETTING_TUI_CF_ONLYMEDIA))
        return 0;
    if (setting_get_enum(SETTING_TUI_CF_SDP_INFO) == SETTING_SDP_COMPRESSED)
        return 1;
    if (!msg_has_sdp(msg_arrow->msg))
        return 2;
    if (setting_get_enum(SETTING_TUI_CF_SDP_INFO) == SETTING_SDP_OFF)
        return 2;
    if (setting_get_enum(SETTING_TUI_CF_SDP_INFO) == SETTING_SDP_FIRST)
        return 2;
    if (setting_get_enum(SETTING_TUI_CF_SDP_INFO) == SETTING_SDP_FULL)
        return msg_media_count(msg_arrow->msg) + 2;

    return 0;
}

static void
sng_flow_msg_arrow_set_property(GObject *self, guint property_id, const GValue *value, GParamSpec *pspec)
{
    SngFlowMsgArrow *msg_arrow = SNG_FLOW_MSG_ARROW(self);

    switch (property_id) {
        case PROP_MESSAGE:
            msg_arrow->msg = g_value_get_pointer(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}

static void
sng_flow_msg_arrow_get_property(GObject *self, guint property_id, GValue *value, GParamSpec *pspec)
{
    SngFlowMsgArrow *msg_arrow = SNG_FLOW_MSG_ARROW(self);
    switch (property_id) {
        case PROP_MESSAGE:
            g_value_set_pointer(value, msg_arrow->msg);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}

static void
sng_flow_msg_arrow_class_init(SngFlowMsgArrowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = sng_flow_msg_arrow_get_property;
    object_class->set_property = sng_flow_msg_arrow_set_property;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->draw = sng_flow_msg_arrow_draw;
    widget_class->preferred_height = sng_flow_msg_arrow_get_preferred_height;

    SngFlowArrowClass *arrow_class = SNG_FLOW_ARROW_CLASS(klass);
    arrow_class->get_time = sng_flow_msg_arrow_get_time;

    obj_properties[PROP_MESSAGE] =
        g_param_spec_pointer("message",
                             "Arrow SIP Message",
                             "Arrow SIP Message",
                             G_PARAM_CONSTRUCT | G_PARAM_READWRITE
        );

    g_object_class_install_properties(
        object_class,
        N_PROPERTIES,
        obj_properties
    );
}

static void
sng_flow_msg_arrow_init(G_GNUC_UNUSED SngFlowMsgArrow *msg_arrow)
{
}
