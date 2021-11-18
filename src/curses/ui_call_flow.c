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
/**
 * @file ui_call_flow.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_call_flow.h
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "capture.h"
#include "ui_manager.h"
#include "ui_call_flow.h"
#include "ui_call_raw.h"
#include "ui_msg_diff.h"
#include "ui_save.h"
#include "util.h"
#include "vector.h"
#include "option.h"

#define METHOD_MAXLEN 80

/***
 *
 * Some basic ascii art of this panel.
 *
 * +--------------------------------------------------------+
 * |                     Title                              |
 * |   addr1  addr2  addr3  addr4 | Selected Raw Message    |
 * |   -----  -----  -----  ----- | preview                 |
 * | Tmst|      |      |      |   |                         |
 * | Tmst|----->|      |      |   |                         |
 * | Tmst|      |----->|      |   |                         |
 * | Tmst|      |<-----|      |   |                         |
 * | Tmst|      |      |----->|   |                         |
 * | Tmst|<-----|      |      |   |                         |
 * | Tmst|      |----->|      |   |                         |
 * | Tmst|      |<-----|      |   |                         |
 * | Tmst|      |------------>|   |                         |
 * | Tmst|      |<------------|   |                         |
 * |     |      |      |      |   |                         |
 * |     |      |      |      |   |                         |
 * |     |      |      |      |   |                         |
 * | Useful hotkeys                                         |
 * +--------------------------------------------------------+
 *
 */

/**
 * Ui Structure definition for Call Flow panel
 */
ui_t ui_call_flow = {
    .type = PANEL_CALL_FLOW,
    .panel = NULL,
    .create = call_flow_create,
    .destroy = call_flow_destroy,
    .redraw = call_flow_redraw,
    .draw = call_flow_draw,
    .handle_key = call_flow_handle_key,
    .help = call_flow_help
};

void
call_flow_create(ui_t *ui)
{
    // Create a new panel to fill all the screen
    ui_panel_create(ui, LINES, COLS);

    // Initialize Call List specific data
    call_flow_info_t *info = malloc(sizeof(call_flow_info_t));
    memset(info, 0, sizeof(call_flow_info_t));

    // Display timestamp next to each arrow
    info->arrowtime = true;

    // Calculate available printable area for messages
    info->flow_win = subwin(ui->win, ui->height - 6, ui->width - 2, 4, 0);
    info->scroll = ui_set_scrollbar(info->flow_win, SB_VERTICAL, SB_LEFT);

    // Create vectors for columns and flow arrows
    info->columns = vector_create(2, 1);
    info->arrows = vector_create(20, 5);
    vector_set_sorter(info->arrows, call_flow_arrow_sorter);

    // Store it into panel userptr
    set_panel_userptr(ui->panel, (void*) info);
}

void
call_flow_destroy(ui_t *ui)
{
    call_flow_info_t *info;

    // Free the panel information
    if ((info = call_flow_info(ui))) {
        // Delete panel columns
        vector_destroy_items(info->columns);
        // Delete panel arrows
        vector_destroy_items(info->arrows);
        // Delete panel windows
        delwin(info->flow_win);
        delwin(info->raw_win);
        // Delete displayed call group
        call_group_destroy(info->group);
        // Free panel info
        free(info);
    }
    ui_panel_destroy(ui);
}

call_flow_info_t *
call_flow_info(ui_t *ui)
{
    return (call_flow_info_t*) panel_userptr(ui->panel);
}

bool
call_flow_redraw(ui_t *ui)
{
    int maxx, maxy;

    // Get panel information
    call_flow_info_t *info = call_flow_info(ui);
    // Get current screen dimensions
    getmaxyx(stdscr, maxy, maxx);

    // Change the main window size
    wresize(ui->win, maxy, maxx);

    // Store new size
    ui->width = maxx;
    ui->height = maxy;

    // Calculate available printable area
    wresize(info->flow_win, maxy - 6, maxx);

    // Force flow redraw
    call_flow_draw(ui);

    // Check if any of the group has changed
    // return call_group_has_changed(info->group);
    return 0;
}

int
call_flow_draw(ui_t *ui)
{
    char title[256];

    // Get panel information
    call_flow_info_t *info = call_flow_info(ui);

    // Get window of main panel
    werase(ui->win);

    // Set title
    if (info->group->callid) {
        sprintf(title, "Extended Call flow for %s", info->group->callid);
    } else if (call_group_count(info->group) == 1) {
        sip_call_t *call = call_group_get_next(info->group, NULL);
        sprintf(title, "Call flow for %s", call->callid);
    } else {
        sprintf(title, "Call flow for %d dialogs", call_group_count(info->group));
    }

    // Print color mode in title
    if (setting_has_value(SETTING_COLORMODE, "request"))
        strcat(title, " (Color by Request/Response)");
    if (setting_has_value(SETTING_COLORMODE, "callid"))
        strcat(title, " (Color by Call-Id)");
    if (setting_has_value(SETTING_COLORMODE, "cseq"))
        strcat(title, " (Color by CSeq)");

    // Draw panel title
    ui_set_title(ui, title);

    // Show some keybinding
    call_flow_draw_footer(ui);

    // Redraw columns
    call_flow_draw_columns(ui);

    // Redraw arrows
    call_flow_draw_arrows(ui);

    // Redraw preview
    call_flow_draw_preview(ui);

    // Draw the scrollbar
    vector_iter_t it = vector_iterator(info->darrows);
    call_flow_arrow_t *arrow = NULL;
    info->scroll.max = info->scroll.pos = 0;
    while ((arrow = vector_iterator_next(&it))) {
        // Store current position arrow
        if (vector_iterator_current(&it) == info->first_arrow) {
            info->scroll.pos = info->scroll.max;
        }
        info->scroll.max += call_flow_arrow_height(ui, arrow);
    }
    ui_scrollbar_draw(info->scroll);

    // Redraw flow win
    wnoutrefresh(info->flow_win);
    return 0;
}

void
call_flow_draw_footer(ui_t *ui)
{
    const char *keybindings[] = {
        key_action_key_str(ACTION_PREV_SCREEN), "Calls List",
        key_action_key_str(ACTION_CONFIRM), "Raw",
        key_action_key_str(ACTION_SELECT), "Compare",
        key_action_key_str(ACTION_SHOW_HELP), "Help",
        key_action_key_str(ACTION_SDP_INFO), "SDP",
        key_action_key_str(ACTION_TOGGLE_MEDIA), "RTP",
        key_action_key_str(ACTION_SHOW_FLOW_EX), "Extended",
        key_action_key_str(ACTION_COMPRESS), "Compressed",
        key_action_key_str(ACTION_SHOW_RAW), "Raw",
        key_action_key_str(ACTION_CYCLE_COLOR), "Colour by",
        key_action_key_str(ACTION_INCREASE_RAW), "Increase Raw"
    };

    ui_draw_bindings(ui, keybindings, 22);
}

int
call_flow_draw_columns(ui_t *ui)
{
    call_flow_info_t *info;
    call_flow_column_t *column;
    sip_call_t *call = NULL;
    rtp_stream_t *stream;
    sip_msg_t *msg = NULL;
    vector_iter_t streams;
    vector_iter_t columns;
    char coltext[MAX_SETTING_LEN];
    address_t addr;

    // Get panel information
    info = call_flow_info(ui);

    // In extended call flow, columns can have multiple call-ids
    if (info->group->callid) {
        info->maxcallids = call_group_count(info->group);
    } else {
        info->maxcallids = 2;
    }

    // Load columns
    while((msg = call_group_get_next_msg(info->group, msg))) {
        call_flow_column_add(ui, msg->call->callid, msg->packet->src);
        call_flow_column_add(ui, msg->call->callid, msg->packet->dst);
    }

    // Add RTP columns FIXME Really
    if (!setting_disabled(SETTING_CF_MEDIA)) {
        while ((call = call_group_get_next(info->group, call)) ) {
            streams = vector_iterator(call->streams);

            while ((stream = vector_iterator_next(&streams))) {
                if (stream->type == PACKET_RTP && stream_get_count(stream)) {
                    addr = stream->src;
                    addr.port = 0;
                    call_flow_column_add(ui, NULL, addr);
                    addr = stream->dst;
                    addr.port = 0;
                    call_flow_column_add(ui, NULL, addr);
                }
            }
        }
    }

    // Draw columns
    columns = vector_iterator(info->columns);
    while ((column = vector_iterator_next(&columns))) {
        mvwvline(info->flow_win, 0, 20 + 30 * column->colpos, ACS_VLINE, ui->height - 6);
        mvwhline(ui->win, 3, 10 + 30 * column->colpos, ACS_HLINE, 20);
        mvwaddch(ui->win, 3, 20 + 30 * column->colpos, ACS_TTEE);

        // Set bold to this address if it's local
        if (setting_enabled(SETTING_CF_LOCALHIGHLIGHT)) {
            if (address_is_local(column->addr))
                wattron(ui->win, A_BOLD);
        }

        if (setting_enabled(SETTING_CF_SPLITCALLID) || !column->addr.port) {
            snprintf(coltext, MAX_SETTING_LEN, "%s", column->alias);
        } else if (setting_enabled(SETTING_DISPLAY_ALIAS)) {
            if (strlen(column->addr.ip) > 15) {
                snprintf(coltext, MAX_SETTING_LEN, "..%.*s:%u",
                         MAX_SETTING_LEN - 9, column->alias + strlen(column->alias) - 13, column->addr.port);
            } else {
                snprintf(coltext, MAX_SETTING_LEN, "%.*s:%u",
                         MAX_SETTING_LEN - 7, column->alias, column->addr.port);
            }
        } else {
            if (strlen(column->addr.ip) > 15) {
                snprintf(coltext, MAX_SETTING_LEN, "..%.*s:%u",
                         MAX_SETTING_LEN - 9, column->addr.ip + strlen(column->addr.ip) - 13, column->addr.port);
            } else {
                snprintf(coltext, MAX_SETTING_LEN, "%.*s:%u",
                         MAX_SETTING_LEN - 7, column->addr.ip, column->addr.port);
            }
        }

        mvwprintw(ui->win, 2, 10 + 30 * column->colpos + (22 - strlen(coltext)) / 2, "%s", coltext);
        wattroff(ui->win, A_BOLD);
    }

    return 0;
}

void
call_flow_draw_arrows(ui_t *ui)
{
    call_flow_info_t *info;
    call_flow_arrow_t *arrow = NULL;
    int cline = 0;

    // Get panel information
    info = call_flow_info(ui);

    // Create pending SIP arrows
    sip_msg_t *msg = NULL;
    while ((msg = call_group_get_next_msg(info->group, msg))) {
        if (!call_flow_arrow_find(ui, msg)) {
            arrow = call_flow_arrow_create(ui, msg, CF_ARROW_SIP);
            vector_append(info->arrows, arrow);
        }
    }
    // Create pending RTP arrows
    rtp_stream_t *stream = NULL;
    while ((stream = call_group_get_next_stream(info->group, stream))) {
        if (!call_flow_arrow_find(ui, stream)) {
            arrow = call_flow_arrow_create(ui, stream, CF_ARROW_RTP);
            vector_append(info->arrows, arrow);
        }
    }

    // Copy displayed arrows
    // vector_destroy(info->darrows);
    //info->darrows = vector_copy_if(info->arrows, call_flow_arrow_filter);
    info->darrows = info->arrows;

    // If no active call, use the fist one (if exists)
    if (info->cur_arrow == -1 && vector_count(info->darrows)) {
        info->cur_arrow = info->first_arrow = 0;
    }

    // Draw arrows
    vector_iter_t it = vector_iterator(info->darrows);
    vector_iterator_set_current(&it, info->first_arrow - 1);
    vector_iterator_set_filter(&it, call_flow_arrow_filter);
    while ((arrow = vector_iterator_next(&it))) {
        // Stop if we have reached the bottom of the screen
        if (cline >= getmaxy(info->flow_win))
            break;
        // Draw arrow
        cline += call_flow_draw_arrow(ui, arrow, cline);
    }
}

int
call_flow_draw_arrow(ui_t *ui, call_flow_arrow_t *arrow, int line)
{
    if (arrow->type == CF_ARROW_SIP) {
        return call_flow_draw_message(ui, arrow, line);
    } else {
        return call_flow_draw_rtp_stream(ui, arrow, line);
    }
}

void
call_flow_draw_preview(ui_t *ui)
{
    call_flow_arrow_t *arrow = NULL;
    call_flow_info_t *info;

    // Check if not displaying raw has been requested
    if (setting_disabled(SETTING_CF_FORCERAW))
        return;

    // Get panel information
    info = call_flow_info(ui);

    // Draw current arrow preview
    if ((arrow = vector_item(info->darrows, info->cur_arrow))) {
        if (arrow->type == CF_ARROW_SIP) {
            call_flow_draw_raw(ui, arrow->item);
        } else {
            call_flow_draw_raw_rtcp(ui, arrow->item);
        }
    }
}

int
call_flow_draw_message(ui_t *ui, call_flow_arrow_t *arrow, int cline)
{
    call_flow_info_t *info;
    WINDOW *flow_win;
    sdp_media_t *media;
    const char *callid;
    char msg_method[SIP_ATTR_MAXLEN];
    char msg_time[80];
    address_t src;
    address_t dst;
    char method[METHOD_MAXLEN + 1];
    char delta[15] = {};
    int flowh, floww;
    char mediastr[40];
    sip_msg_t *msg = arrow->item;
    vector_iter_t medias;
    int color = 0;
    int msglen;
    int aline = cline + 1;

    // Initialize method
    memset(method, 0, sizeof(method));

    // Get panel information
    info = call_flow_info(ui);

    // Get the messages window
    flow_win = info->flow_win;
    getmaxyx(flow_win, flowh, floww);

    // Store arrow start line
    arrow->line = cline;

    // Calculate how many lines this message requires
    arrow->height = call_flow_arrow_height(ui, arrow);

    // Check this message fits on the panel
    if (cline > flowh + arrow->height)
        return 0;

    // For extended, use xcallid nstead
    callid = msg->call->callid;
    src = msg->packet->src;
    dst = msg->packet->dst;
    media = vector_first(msg->medias);
    msg_get_attribute(msg, SIP_ATTR_METHOD, msg_method);
    timeval_to_time(msg_get_time(msg), msg_time);

    // Get Message method (include extra info)
    snprintf(method, METHOD_MAXLEN, "%.*s", METHOD_MAXLEN-1, msg_method);

    // If message has sdp information
    if (msg_has_sdp(msg) && setting_has_value(SETTING_CF_SDP_INFO, "off")) {
        // Show sdp tag in title
        snprintf(method, METHOD_MAXLEN, "%.*s (SDP)", METHOD_MAXLEN-7, msg_method );
    }

    // If message has sdp information
    if (setting_has_value(SETTING_CF_SDP_INFO, "compressed")) {
        // Show sdp tag in title
        if (msg_has_sdp(msg)) {
            snprintf(method, METHOD_MAXLEN, "%.*s (SDP)", 12, msg_method);
        } else {
            snprintf(method, METHOD_MAXLEN, "%.*s", 17, msg_method);
        }
    }

    if (msg_has_sdp(msg) && setting_has_value(SETTING_CF_SDP_INFO, "first")) {
        snprintf(method, METHOD_MAXLEN, "%.3s (%s:%u)",
		 msg_method,
		 media->address.ip,
		 media->address.port);
    }

    if (msg_has_sdp(msg) && setting_has_value(SETTING_CF_SDP_INFO, "full")) {
        snprintf(method, METHOD_MAXLEN, "%.3s (%s)", msg_method, media->address.ip);
    }

    // Draw message type or status and line
    msglen = (strlen(method) > 24) ? 24 : strlen(method);

    // Get origin and destination column
    arrow->scolumn = call_flow_column_get(ui, callid, src);
    arrow->dcolumn = call_flow_column_get(ui, callid, dst);

    // Determine start and end position of the arrow line
    int startpos, endpos;
    if (arrow->scolumn == arrow->dcolumn) {
        arrow->dir = CF_ARROW_SPIRAL;
        startpos = 19 + 30 * arrow->dcolumn->colpos;
        endpos = 20 + 30 * arrow->scolumn->colpos;
    } else if (arrow->scolumn->colpos < arrow->dcolumn->colpos) {
        arrow->dir = CF_ARROW_RIGHT;
        startpos = 20 + 30 * arrow->scolumn->colpos;
        endpos = 20 + 30 * arrow->dcolumn->colpos;
    } else {
        arrow->dir = CF_ARROW_LEFT;
        startpos = 20 + 30 * arrow->dcolumn->colpos;
        endpos = 20 + 30 * arrow->scolumn->colpos;
    }
    int distance = abs(endpos - startpos) - 3;

    // Highlight current message
    if (arrow == vector_item(info->darrows, info->cur_arrow)) {
        if (setting_has_value(SETTING_CF_HIGHTLIGHT, "reverse")) {
            wattron(flow_win, A_REVERSE);
        }
        if (setting_has_value(SETTING_CF_HIGHTLIGHT, "bold")) {
            wattron(flow_win, A_BOLD);
        }
        if (setting_has_value(SETTING_CF_HIGHTLIGHT, "reversebold")) {
            wattron(flow_win, A_REVERSE);
            wattron(flow_win, A_BOLD);
        }
    }

    // Color the message {
    if (setting_has_value(SETTING_COLORMODE, "request")) {
        // Color by request / response
        color = (msg_is_request(msg)) ? CP_RED_ON_DEF : CP_GREEN_ON_DEF;
    } else if (setting_has_value(SETTING_COLORMODE, "callid")) {
        // Color by call-id
        color = call_group_color(info->group, msg->call);
    } else if (setting_has_value(SETTING_COLORMODE, "cseq")) {
        // Color by CSeq within the same call
        color = msg->cseq % 7 + 1;
    }

    // Print arrow in the same line than message
    if (setting_has_value(SETTING_CF_SDP_INFO, "compressed")) {
        aline = cline;
    }

    // Turn on the message color
    wattron(flow_win, COLOR_PAIR(color));

    // Clear the line
    mvwprintw(flow_win, cline, startpos + 2, "%*s", distance, "");

    // Draw method
    if (arrow->dir == CF_ARROW_SPIRAL) {
        mvwprintw(flow_win, cline, startpos + 5, "%.26s", method);
    } else {
        mvwprintw(flow_win, cline, startpos + distance / 2 - msglen / 2 + 2, "%.26s", method);
    }

    // Draw media information
    if (msg_has_sdp(msg) && setting_has_value(SETTING_CF_SDP_INFO, "full")) {
        medias = vector_iterator(msg->medias);
        while ((media = vector_iterator_next(&medias))) {
            sprintf(mediastr, "%s %d (%s)",
                    media->type,
                    media->address.port,
                    media_get_prefered_format(media));
            if (arrow->dir == CF_ARROW_SPIRAL) {
                mvwprintw(flow_win, cline + 1, startpos + 5, "%s", mediastr);
            } else {
                mvwprintw(flow_win, cline + 1, startpos + distance / 2 - strlen(mediastr) / 2 + 2, "%s", mediastr);
            }
            cline++;
            aline++;
        }
    }

    if (arrow->dir != CF_ARROW_SPIRAL) {
        if (arrow == call_flow_arrow_selected(ui)) {
            mvwhline(flow_win, aline, startpos + 2, '=', distance);
        } else {
            mvwhline(flow_win, aline, startpos + 2, ACS_HLINE, distance);
        }
    }

    // Write the arrow at the end of the message (two arrows if this is a retrans)
    if (arrow->dir == CF_ARROW_SPIRAL) {
        mvwaddch(flow_win, aline, startpos + 2, '<');
        if (msg->retrans) {
            mvwaddch(flow_win, aline, startpos + 3, '<');
            mvwaddch(flow_win, aline, startpos + 4, '<');
        }
        // If multiple lines are available, print a spiral icon
        if (aline != cline) {
            mvwaddch(flow_win, aline, startpos + 3, ACS_LRCORNER);
            mvwaddch(flow_win, aline - 1, startpos + 3, ACS_URCORNER);
            mvwaddch(flow_win, aline - 1, startpos + 2, ACS_HLINE);
        }
    } else if (arrow->dir == CF_ARROW_RIGHT) {
        mvwaddch(flow_win, aline, endpos - 2, '>');
        if (msg->retrans) {
            mvwaddch(flow_win, aline, endpos - 3, '>');
            mvwaddch(flow_win, aline, endpos - 4, '>');
        }
    } else {
        mvwaddch(flow_win, aline, startpos + 2, '<');
        if (msg->retrans) {
            mvwaddch(flow_win, aline, startpos + 3, '<');
            mvwaddch(flow_win, aline, startpos + 4, '<');
        }
    }

    if (setting_has_value(SETTING_CF_SDP_INFO, "compressed"))
        mvwprintw(flow_win, cline, startpos + distance / 2 - msglen / 2 + 2, " %.26s ", method);

    // Turn off colors
    wattroff(flow_win, COLOR_PAIR(CP_RED_ON_DEF));
    wattroff(flow_win, COLOR_PAIR(CP_GREEN_ON_DEF));
    wattroff(flow_win, COLOR_PAIR(CP_CYAN_ON_DEF));
    wattroff(flow_win, COLOR_PAIR(CP_YELLOW_ON_DEF));
    wattroff(flow_win, A_BOLD | A_REVERSE);

    // Print timestamp
    if (info->arrowtime) {
        if (arrow == call_flow_arrow_selected(ui))
            wattron(flow_win, COLOR_PAIR(CP_CYAN_ON_DEF));

        if (arrow == vector_item(info->darrows, info->cur_arrow)) {
            wattron(flow_win, A_BOLD);
            mvwprintw(flow_win, cline, 2, "%s", msg_time);
            wattroff(flow_win, A_BOLD);
        } else {
            mvwprintw(flow_win, cline, 2, "%s", msg_time);
        }

        // Print delta from selected message
        if (!setting_has_value(SETTING_CF_SDP_INFO, "compressed")) {
            if (info->selected == -1) {
                if (setting_enabled(SETTING_CF_DELTA)) {
                    struct timeval selts, curts;
                    selts = msg_get_time(call_group_get_prev_msg(info->group, msg));
                    curts = msg_get_time(msg);
                    timeval_to_delta(selts, curts, delta);
                }
            } else if (arrow == vector_item(info->darrows, info->cur_arrow)) {
                struct timeval selts, curts;
                selts = msg_get_time(call_flow_arrow_message(call_flow_arrow_selected(ui)));
                curts = msg_get_time(msg);
                timeval_to_delta(selts, curts, delta);
            }

            if (strlen(delta)) {
                wattron(flow_win, COLOR_PAIR(CP_CYAN_ON_DEF));
                mvwprintw(flow_win, cline - 1 , 2, "%15s", delta);
            }
            wattroff(flow_win, COLOR_PAIR(CP_CYAN_ON_DEF));
        }
    }
    wattroff(flow_win, COLOR_PAIR(CP_CYAN_ON_DEF));

    return arrow->height;
}


int
call_flow_draw_rtp_stream(ui_t *ui, call_flow_arrow_t *arrow, int cline)
{
    call_flow_info_t *info;
    WINDOW *win;
    char text[50], time[20];
    int height, width;
    rtp_stream_t *stream = arrow->item;
    sip_msg_t *msg;
    sip_call_t *call;
    call_flow_arrow_t *msgarrow;
    address_t addr;

    // Get panel information
    info = call_flow_info(ui);
    // Get the messages window
    win = info->flow_win;
    getmaxyx(win, height, width);

    // Store arrow start line
    arrow->line = cline;

    // Calculate how many lines this message requires
    arrow->height = call_flow_arrow_height(ui, arrow);

    // Check this media fits on the panel
    if (cline > height + arrow->height)
        return 0;

    // Get arrow text
    sprintf(text, "RTP (%s) %d", stream_get_format(stream), stream_get_count(stream));

    // Get message data
    call = stream->media->msg->call;

    /**
     * This logic will try to use the same columns for the stream representation
     * that are used in the SIP messages that configured the streams in their SDP
     * if they share the same IP addresses.
     */
    // Message with Stream destination configured in SDP content
    msg = stream->media->msg;

    // If message and stream share the same IP address
    if (address_equals(msg->packet->src, stream->dst)) {
        // Reuse the msg arrow columns as destination column
        if ((msgarrow = call_flow_arrow_find(ui, msg))) {
            // Get origin and destination column
            arrow->dcolumn = call_flow_column_get(ui, msg->call->callid, msg->packet->src);
        }
    }

    // fallback: Just use any column that have the destination IP printed
    if (!arrow->dcolumn) {
        // FIXME figure a better way to find ignoring port :(
        addr = stream->dst; addr.port = 0;
        arrow->dcolumn = call_flow_column_get(ui, 0, addr);
    }

    /**
     * For source address of the stream, first try to find a message that have
     * the stream source configured in their SDP as destination and then apply
     * the same previous logic address: if IP address matches, reuse message
     * column, othwerise any column with the source IP will be used.
     */
    // Message with Stream source configured in SDP content
    msg = call_msg_with_media(call, stream->src);

    // Try to find a message with configured SDP matching the source of this stream
    if (msg && address_equals(msg->packet->src, stream->src)) {
        // Reuse the msg arrow columns as destination column
        if ((msgarrow = call_flow_arrow_find(ui, msg))) {
            // Get origin and destination column
            arrow->scolumn = call_flow_column_get(ui, msg->call->callid, msg->packet->src);
        }
    }

    // Prefer message that configured this stream rather than any column
    if (!arrow->scolumn) {
        msg = stream->media->msg;
        // If message and stream share the same IP address
        if (address_equals(msg->packet->dst, stream->src)) {
            // Reuse the msg arrow columns as destination column
            if ((msgarrow = call_flow_arrow_find(ui, msg))) {
                arrow->scolumn = call_flow_column_get(ui, msg->call->callid, msg->packet->dst);
            }
        }
    }

    // fallback: Just use any column that have the soruce IP printed
    if (!arrow->scolumn) {
        // FIXME figure a better way to find ignoring port :(
        addr = stream->src; addr.port = 0;
        arrow->scolumn = call_flow_column_get(ui, 0, addr);
    }

    // Determine start and end position of the arrow line
    int startpos, endpos;
    if (arrow->scolumn->colpos < arrow->dcolumn->colpos) {
        arrow->dir= CF_ARROW_RIGHT;
        startpos = 20 + 30 * arrow->scolumn->colpos;
        endpos = 20 + 30 * arrow->dcolumn->colpos;
    } else {
        arrow->dir = CF_ARROW_LEFT;
        startpos = 20 + 30 * arrow->dcolumn->colpos;
        endpos = 20 + 30 * arrow->scolumn->colpos;
    }
    int distance = 0;

    if (startpos != endpos) {
        // In compressed mode, we display the src and dst port inside the arrow
        // so fixup the stard and end position
        if (!setting_has_value(SETTING_CF_SDP_INFO, "compressed")) {
            startpos += 5;
            endpos -= 5;
        }
        distance = abs(endpos - startpos) - 4 + 1;
    } else {
        // Fix port positions
        startpos -= 2;
        endpos += 2;
        distance = 1;

        // Fix arrow direction based on ports
        if (stream->src.port < stream->dst.port) {
            arrow->dir = CF_ARROW_RIGHT;
        } else {
            arrow->dir = CF_ARROW_LEFT;
        }
    }

    // Highlight current message
    if (arrow == vector_item(info->darrows, info->cur_arrow)) {
        if (setting_has_value(SETTING_CF_HIGHTLIGHT, "reverse")) {
            wattron(win, A_REVERSE);
        }
        if (setting_has_value(SETTING_CF_HIGHTLIGHT, "bold")) {
            wattron(win, A_BOLD);
        }
        if (setting_has_value(SETTING_CF_HIGHTLIGHT, "reversebold")) {
            wattron(win, A_REVERSE);
            wattron(win, A_BOLD);
        }
    }

    // Check if displayed stream is active
    int active = stream_is_active(stream);

    // Clear the line
    mvwprintw(win, cline, startpos + 2, "%*s", distance, "");
    // Draw RTP arrow text
    mvwprintw(win, cline, startpos + (distance) / 2 - strlen(text) / 2 + 2, "%s", text);

    if (!setting_has_value(SETTING_CF_SDP_INFO, "compressed"))
        cline++;

    // Draw line between columns
    if (active)
        mvwhline(win, cline, startpos + 2, '-', distance);
    else
        mvwhline(win, cline, startpos + 2, ACS_HLINE, distance);

    // Write the arrow at the end of the message (two arrows if this is a retrans)
    if (arrow->dir == CF_ARROW_RIGHT) {
        if (!setting_has_value(SETTING_CF_SDP_INFO, "compressed")) {
            mvwprintw(win, cline, startpos - 4, "%d", stream->src.port);
            mvwprintw(win, cline, endpos, "%d", stream->dst.port);
        }
        mvwaddch(win, cline, endpos - 2, '>');
        if (active) {
            arrow->rtp_count = stream_get_count(stream);
            arrow->rtp_ind_pos = (arrow->rtp_ind_pos + 1) % distance;
            mvwaddch(win, cline, startpos + arrow->rtp_ind_pos + 2, '>');
        }
    } else {
        if (!setting_has_value(SETTING_CF_SDP_INFO, "compressed")) {
            mvwprintw(win, cline, endpos, "%d", stream->src.port);
            mvwprintw(win, cline, startpos - 4, "%d", stream->dst.port);
        }
        mvwaddch(win, cline, startpos + 2, '<');
        if (active) {
            arrow->rtp_count = stream_get_count(stream);
            arrow->rtp_ind_pos = (arrow->rtp_ind_pos + 1) % distance;
            mvwaddch(win, cline, endpos - arrow->rtp_ind_pos - 2, '<');
        }
    }

    if (setting_has_value(SETTING_CF_SDP_INFO, "compressed"))
        mvwprintw(win, cline, startpos + (distance) / 2 - strlen(text) / 2 + 2, " %s ", text);

    wattroff(win, A_BOLD | A_REVERSE);

    // Print timestamp
    if (info->arrowtime) {
        timeval_to_time(stream->time, time);
        if (arrow == vector_item(info->darrows, info->cur_arrow)) {
            wattron(win, A_BOLD);
            mvwprintw(win, cline, 2, "%s", time);
            wattroff(win, A_BOLD);
        } else {
            mvwprintw(win, cline, 2, "%s", time);
        }

    }

    return arrow->height;
}

call_flow_arrow_t *
call_flow_arrow_create(ui_t *ui, void *item, int type)
{
    call_flow_arrow_t *arrow;

    if ((arrow = call_flow_arrow_find(ui, item)))
        return arrow;

    // Create a new arrow of the given type
    arrow = malloc(sizeof(call_flow_arrow_t));
    memset(arrow, 0, sizeof(call_flow_arrow_t));
    arrow->type = type;
    arrow->item = item;
    return arrow;
}

int
call_flow_arrow_height(ui_t *ui, const call_flow_arrow_t *arrow)
{
    if (arrow->type == CF_ARROW_SIP) {
        if (setting_enabled(SETTING_CF_ONLYMEDIA))
            return 0;
        if (setting_has_value(SETTING_CF_SDP_INFO, "compressed"))
            return 1;
        if (!msg_has_sdp(arrow->item))
            return 2;
        if (setting_has_value(SETTING_CF_SDP_INFO, "off"))
            return 2;
        if (setting_has_value(SETTING_CF_SDP_INFO, "first"))
            return 2;
        if (setting_has_value(SETTING_CF_SDP_INFO, "full"))
            return msg_media_count(arrow->item) + 2;
    } else if (arrow->type == CF_ARROW_RTP || arrow->type == CF_ARROW_RTCP) {
        if (setting_has_value(SETTING_CF_SDP_INFO, "compressed"))
            return 1;
        if (setting_disabled(SETTING_CF_MEDIA))
            return 0;
        return 2;
    }

    return 0;
}

call_flow_arrow_t *
call_flow_arrow_find(ui_t *ui, const void *data)
{
    call_flow_info_t *info;
    call_flow_arrow_t *arrow;
    vector_iter_t arrows;

    if (!data)
        return NULL;

    if (!(info = call_flow_info(ui)))
        return NULL;

    arrows = vector_iterator(info->arrows);
    while ((arrow = vector_iterator_next(&arrows)))
        if (arrow->item == data)
            return arrow;

    return arrow;
}

sip_msg_t *
call_flow_arrow_message(const  call_flow_arrow_t *arrow)
{
    if (!arrow)
        return NULL;

    if (arrow->type == CF_ARROW_SIP) {
        return arrow->item;
    }

    if (arrow->type == CF_ARROW_RTP) {
        rtp_stream_t *stream = arrow->item;
        return stream->media->msg;
    }

    return NULL;
}

int
call_flow_draw_raw(ui_t *ui, sip_msg_t *msg)
{
    call_flow_info_t *info;
    WINDOW *raw_win;
    vector_iter_t arrows;
    call_flow_arrow_t *arrow;
    int raw_width, raw_height;
    int min_raw_width, fixed_raw_width;

    // Get panel information
    if (!(info = call_flow_info(ui)))
        return 1;

    // Get min raw width
    min_raw_width = setting_get_intvalue(SETTING_CF_RAWMINWIDTH);
    fixed_raw_width = setting_get_intvalue(SETTING_CF_RAWFIXEDWIDTH);

    // Calculate the raw data width (width - used columns for flow - vertical lines)
    raw_width = ui->width - (30 * vector_count(info->columns)) - 2;

    // If last column has spirals, add an extra column with
    arrows = vector_iterator(info->arrows);
    while ((arrow = vector_iterator_next(&arrows))) {
        if (arrow->dir == CF_ARROW_SPIRAL
            && arrow->scolumn == vector_last(info->columns)) {
            raw_width -= 15;
            break;
        }
    }

    // We can define a mininum size for rawminwidth
    if (raw_width < min_raw_width) {
        raw_width = min_raw_width;
    }
    // We can configure an exact raw size
    if (fixed_raw_width > 0) {
        raw_width = fixed_raw_width;
    }

    // Height of raw window is always available size minus 6 lines for header/footer
    raw_height = ui->height - 3;

    // If we already have a raw window
    raw_win = info->raw_win;
    if (raw_win) {
        // Check it has the correct size
        if (getmaxx(raw_win) != raw_width) {
            // We need a new raw window
            delwin(raw_win);
            info->raw_win = raw_win = newwin(raw_height, raw_width, 0, 0);
        } else {
            // We have a valid raw win, clear its content
            werase(raw_win);
        }
    } else {
        // Create the raw window of required size
        info->raw_win = raw_win = newwin(raw_height, raw_width, 0, 0);
    }

    // Draw raw box li
    wattron(ui->win, COLOR_PAIR(CP_BLUE_ON_DEF));
    mvwvline(ui->win, 1, ui->width - raw_width - 2, ACS_VLINE, ui->height - 2);
    wattroff(ui->win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Print msg payload
    draw_message(info->raw_win, msg);

    // Copy the raw_win contents into the panel
    copywin(raw_win, ui->win, 0, 0, 1, ui->width - raw_width - 1, raw_height, ui->width - 2, 0);

    return 0;
}


int
call_flow_draw_raw_rtcp(ui_t *ui, rtp_stream_t *stream)
{
    /**
     * TODO This is too experimental to even display it
     */
    return 0;

    call_flow_info_t *info;
    WINDOW *raw_win;
    int raw_width, raw_height;
    int min_raw_width, fixed_raw_width;

    // Get panel information
    if (!(info = call_flow_info(ui)))
        return 1;

    // Get min raw width
    min_raw_width = setting_get_intvalue(SETTING_CF_RAWMINWIDTH);
    fixed_raw_width = setting_get_intvalue(SETTING_CF_RAWFIXEDWIDTH);

    // Calculate the raw data width (width - used columns for flow - vertical lines)
    raw_width = ui->width - (30 * vector_count(info->columns)) - 2;
    // We can define a mininum size for rawminwidth
    if (raw_width < min_raw_width) {
        raw_width = min_raw_width;
    }
    // We can configure an exact raw size
    if (fixed_raw_width > 0) {
        raw_width = fixed_raw_width;
    }

    // Height of raw window is always available size minus 6 lines for header/footer
    raw_height = ui->height - 3;

    // If we already have a raw window
    raw_win = info->raw_win;
    if (raw_win) {
        // Check it has the correct size
        if (getmaxx(raw_win) != raw_width) {
            // We need a new raw window
            delwin(raw_win);
            info->raw_win = raw_win = newwin(raw_height, raw_width, 0, 0);
        } else {
            // We have a valid raw win, clear its content
            werase(raw_win);
        }
    } else {
        // Create the raw window of required size
        info->raw_win = raw_win = newwin(raw_height, raw_width, 0, 0);
    }

    // Draw raw box lines
    wattron(ui->win, COLOR_PAIR(CP_BLUE_ON_DEF));
    mvwvline(ui->win, 1, ui->width - raw_width - 2, ACS_VLINE, ui->height - 2);
    wattroff(ui->win, COLOR_PAIR(CP_BLUE_ON_DEF));

    mvwprintw(raw_win, 0, 0, "============ RTCP Information ============");
    mvwprintw(raw_win, 2, 0, "Sender's packet count: %d", stream->rtcpinfo.spc);
    mvwprintw(raw_win, 3, 0, "Fraction Lost: %d / 256", stream->rtcpinfo.flost);
    mvwprintw(raw_win, 4, 0, "Fraction discarded: %d / 256", stream->rtcpinfo.fdiscard);
    mvwprintw(raw_win, 6, 0, "MOS - Listening Quality: %.1f", (float) stream->rtcpinfo.mosl / 10);
    mvwprintw(raw_win, 7, 0, "MOS - Conversational Quality: %.1f", (float) stream->rtcpinfo.mosc / 10);



    // Copy the raw_win contents into the panel
    copywin(raw_win, ui->win, 0, 0, 1, ui->width - raw_width - 1, raw_height, ui->width - 2, 0);

    return 0;
}

int
call_flow_handle_key(ui_t *ui, int key)
{
    int raw_width, height, width;
    call_flow_info_t *info = call_flow_info(ui);
    ui_t *next_ui;
    sip_call_t *call = NULL, *xcall = NULL;
    int rnpag_steps = setting_get_intvalue(SETTING_CF_SCROLLSTEP);
    int action = -1;

    // Sanity check, this should not happen
    if (!info)
        return KEY_NOT_HANDLED;

    getmaxyx(info->flow_win, height, width);

    // Check actions for this key
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch(action) {
            case ACTION_DOWN:
                call_flow_move(ui, info->cur_arrow + 1);
                break;
            case ACTION_UP:
                call_flow_move(ui, info->cur_arrow - 1);
                break;
            case ACTION_HNPAGE:
                rnpag_steps = rnpag_steps / 2;
                /* no break */
            case ACTION_NPAGE:
                call_flow_move(ui, info->cur_arrow + rnpag_steps);
                break;
            case ACTION_HPPAGE:
                rnpag_steps = rnpag_steps / 2;
                /* no break */
            case ACTION_PPAGE:
                // Prev page => N key up strokes
                call_flow_move(ui, info->cur_arrow - rnpag_steps);
                break;
            case ACTION_BEGIN:
                call_flow_move(ui, 0);
                break;
            case ACTION_END:
                call_flow_move(ui, vector_count(info->darrows));
                break;
            case ACTION_SHOW_FLOW_EX:
                werase(ui->win);
                if (call_group_count(info->group) == 1) {
                    call = vector_first(info->group->calls);
                    if (call->xcallid != NULL && strlen(call->xcallid)) {
                        if ((xcall = sip_find_by_callid(call->xcallid))) {
                            call_group_del(info->group, call);
                            call_group_add(info->group, xcall);
                            call_group_add_calls(info->group, xcall->xcalls);
                            info->group->callid = xcall->callid;
                        }
                    } else {
                        call_group_add_calls(info->group, call->xcalls);
                        info->group->callid = call->callid;
                    }
                } else {
                    call = vector_first(info->group->calls);
                    vector_clear(info->group->calls);
                    call_group_add(info->group, call);
                    info->group->callid = 0;
                }
                call_flow_set_group(info->group);
                break;
            case ACTION_SHOW_RAW:
                // KEY_R, display current call in raw mode
                ui_create_panel(PANEL_CALL_RAW);
                call_raw_set_group(info->group);
                break;
            case ACTION_DECREASE_RAW:
                raw_width = getmaxx(info->raw_win);
                if (raw_width - 2 > 1) {
                    setting_set_intvalue(SETTING_CF_RAWFIXEDWIDTH, raw_width - 2);
                }
                break;
            case ACTION_INCREASE_RAW:
                raw_width = getmaxx(info->raw_win);
                if (raw_width + 2 < COLS - 1) {
                    setting_set_intvalue(SETTING_CF_RAWFIXEDWIDTH, raw_width + 2);
                }
                break;
            case ACTION_RESET_RAW:
                setting_set_intvalue(SETTING_CF_RAWFIXEDWIDTH, -1);
                break;
            case ACTION_ONLY_SDP:
                // Toggle SDP mode
                info->group->sdp_only = !(info->group->sdp_only);
                // Disable sdp_only if there are not messages with sdp
                if (call_group_msg_count(info->group) == 0)
                    info->group->sdp_only = 0;
                // Reset screen
                call_flow_set_group(info->group);
                break;
            case ACTION_SDP_INFO:
                setting_toggle(SETTING_CF_SDP_INFO);
                break;
            case ACTION_ONLY_MEDIA:
                setting_toggle(SETTING_CF_ONLYMEDIA);
                call_flow_set_group(info->group);
                break;
            case ACTION_TOGGLE_MEDIA:
                setting_toggle(SETTING_CF_MEDIA);
                // Force reload arrows
                call_flow_set_group(info->group);
                break;
            case ACTION_TOGGLE_RAW:
                setting_toggle(SETTING_CF_FORCERAW);
                break;
            case ACTION_COMPRESS:
                setting_toggle(SETTING_CF_SPLITCALLID);
                // Force columns reload
                call_flow_set_group(info->group);
                break;
            case ACTION_SAVE:
                if (capture_sources_count() > 1) {
                    dialog_run("Saving is not possible when multiple input sources are specified.");
                    break;
                }
                next_ui = ui_create_panel(PANEL_SAVE);
                save_set_group(next_ui, info->group);
                save_set_msg(next_ui,
                    call_flow_arrow_message(vector_item(info->darrows, info->cur_arrow)));
                break;
            case ACTION_TOGGLE_TIME:
                info->arrowtime = (info->arrowtime) ? false : true;
                break;
            case ACTION_SELECT:
                if (info->selected == -1) {
                    info->selected = info->cur_arrow;
                } else {
                    if (info->selected == info->cur_arrow) {
                        info->selected = -1;
                    } else {
                        // Show diff panel
                        next_ui = ui_create_panel(PANEL_MSG_DIFF);
                        msg_diff_set_msgs(next_ui,
                                          call_flow_arrow_message(vector_item(info->darrows, info->selected)),
                                          call_flow_arrow_message(vector_item(info->darrows, info->cur_arrow)));
                    }
                }
                break;
            case ACTION_CLEAR:
                info->selected = -1;
                break;
            case ACTION_CONFIRM:
                // KEY_ENTER, display current message in raw mode
                ui_create_panel(PANEL_CALL_RAW);
                call_raw_set_group(info->group);
                call_raw_set_msg(call_flow_arrow_message(vector_item(info->darrows, info->cur_arrow)));
                break;
            case ACTION_CLEAR_CALLS:
            case ACTION_CLEAR_CALLS_SOFT:
                // Propagate the key to the previous panel
                return KEY_PROPAGATED;

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

int
call_flow_help(ui_t *ui)
{
    WINDOW *help_win;
    int height, width;

    // Create a new panel and show centered
    height = 28;
    width = 65;
    help_win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);

    // Set the window title
    mvwprintw(help_win, 1, 18, "Call Flow Help");

    // Write border and boxes around the window
    wattron(help_win, COLOR_PAIR(CP_BLUE_ON_DEF));
    box(help_win, 0, 0);
    mvwhline(help_win, 2, 1, ACS_HLINE, 63);
    mvwhline(help_win, 7, 1, ACS_HLINE, 63);
    mvwhline(help_win, height - 3, 1, ACS_HLINE, 63);
    mvwaddch(help_win, 2, 0, ACS_LTEE);
    mvwaddch(help_win, 7, 0, ACS_LTEE);
    mvwaddch(help_win, height - 3, 0, ACS_LTEE);
    mvwaddch(help_win, 2, 64, ACS_RTEE);
    mvwaddch(help_win, 7, 64, ACS_RTEE);
    mvwaddch(help_win, height - 3, 64, ACS_RTEE);

    // Set the window footer (nice blue?)
    mvwprintw(help_win, height - 2, 20, "Press any key to continue");

    // Some brief explanation abotu what window shows
    wattron(help_win, COLOR_PAIR(CP_CYAN_ON_DEF));
    mvwprintw(help_win, 3, 2, "This window shows the messages from a call and its relative");
    mvwprintw(help_win, 4, 2, "ordered by sent or received time.");
    mvwprintw(help_win, 5, 2, "This panel is mosly used when capturing at proxy systems that");
    mvwprintw(help_win, 6, 2, "manages incoming and outgoing request between calls.");
    wattroff(help_win, COLOR_PAIR(CP_CYAN_ON_DEF));

    // A list of available keys in this window
    mvwprintw(help_win, 8, 2, "Available keys:");
    mvwprintw(help_win, 9, 2, "Esc/Q       Go back to Call list window");
    mvwprintw(help_win, 10, 2, "F5/Ctrl-L   Leave screen and clear call list");
    mvwprintw(help_win, 11, 2, "Enter       Show current message Raw");
    mvwprintw(help_win, 12, 2, "F1/h        Show this screen");
    mvwprintw(help_win, 13, 2, "F2/d        Toggle SDP Address:Port info");
    mvwprintw(help_win, 14, 2, "F3/m        Toggle RTP arrows display");
    mvwprintw(help_win, 15, 2, "F4/X        Show call-flow with X-CID/X-Call-ID dialog");
    mvwprintw(help_win, 16, 2, "F5/s        Toggle compressed view (One address <=> one column");
    mvwprintw(help_win, 17, 2, "F6/R        Show original call messages in raw mode");
    mvwprintw(help_win, 18, 2, "F7/c        Cycle between available color modes");
    mvwprintw(help_win, 19, 2, "F8/C        Turn on/off message syntax highlighting");
    mvwprintw(help_win, 20, 2, "F9/l        Turn on/off resolved addresses");
    mvwprintw(help_win, 21, 2, "9/0         Increase/Decrease raw preview size");
    mvwprintw(help_win, 22, 2, "t           Toggle raw preview display");
    mvwprintw(help_win, 23, 2, "T           Restore raw preview size");
    mvwprintw(help_win, 24, 2, "D           Only show SDP messages");

    // Press any key to close
    wgetch(help_win);

    return 0;
}

int
call_flow_set_group(sip_call_group_t *group)
{
    ui_t *ui;
    call_flow_info_t *info;

    if (!(ui = ui_find_by_type(PANEL_CALL_FLOW)))
        return -1;

    if (!(info = call_flow_info(ui)))
        return -1;

    vector_clear(info->columns);
    vector_clear(info->arrows);

    info->group = group;
    info->cur_arrow = info->selected = -1;

    return 0;
}

void
call_flow_column_add(ui_t *ui, const char *callid, address_t addr)
{
    call_flow_info_t *info;
    call_flow_column_t *column;
    vector_iter_t columns;

    if (!(info = call_flow_info(ui)))
        return;

    if (call_flow_column_get(ui, callid, addr))
        return;

    // Try to fill the second Call-Id of the column
    columns = vector_iterator(info->columns);
    while ((column = vector_iterator_next(&columns))) {
        if (addressport_equals(column->addr, addr)) {
            if (column->colpos != 0 && vector_count(column->callids) < info->maxcallids) {
                vector_append(column->callids, (void*)callid);
                return;
            }
        }
    }

    // Create a new column
    column = malloc(sizeof(call_flow_column_t));
    memset(column, 0, sizeof(call_flow_column_t));
    column->callids = vector_create(1, 1);
    vector_append(column->callids, (void*)callid);
    column->addr = addr;
    if (setting_enabled(SETTING_ALIAS_PORT)) {
        strcpy(column->alias, get_alias_value_vs_port(addr.ip, addr.port));
    } else {
        strcpy(column->alias, get_alias_value(addr.ip));
    }
    column->colpos = vector_count(info->columns);
    vector_append(info->columns, column);
}

call_flow_column_t *
call_flow_column_get(ui_t *ui, const char *callid, address_t addr)
{
    call_flow_info_t *info;
    call_flow_column_t *column;
    vector_iter_t columns;
    int match_port;
    const char *alias;

    if (!(info = call_flow_info(ui)))
        return NULL;

    // Look for address or address:port ?
    match_port = addr.port != 0;

    // Get alias value for given address
    if (setting_enabled(SETTING_ALIAS_PORT) && match_port) {
        alias = get_alias_value_vs_port(addr.ip, addr.port);
    } else {
        alias = get_alias_value(addr.ip);
    }

    columns = vector_iterator(info->columns);
    while ((column = vector_iterator_next(&columns))) {
        // In compressed mode, we search using alias instead of address
        if (setting_enabled(SETTING_CF_SPLITCALLID)) {
            if (!strcmp(column->alias, alias)) {
                return column;
            }
        } else {
            // Check if this column matches requested address
            if (match_port) {
                if (addressport_equals(column->addr, addr)) {
                    if (vector_index(column->callids, (void*)callid) >= 0) {
                        return column;
                    }
                }
            } else {
                // Dont check port
                if (address_equals(column->addr, addr)) {
                    return column;
                }
            }
        }
    }
    return NULL;
}

void
call_flow_move(ui_t *ui, int arrowindex)
{
    call_flow_info_t *info;
    call_flow_arrow_t *arrow;
    int flowh;
    int curh = 0;

    // Get panel info
    if (!(info = call_flow_info(ui)))
        return;

    // Already in this position?
    if (info->cur_arrow == arrowindex)
        return;

    // Get flow subwindow height (for scrolling)
    flowh  = getmaxy(info->flow_win);

    // Moving down or up?
    bool move_down = (info->cur_arrow < arrowindex);

    vector_iter_t it = vector_iterator(info->darrows);
    vector_iterator_set_current(&it, info->cur_arrow);
    vector_iterator_set_filter(&it, call_flow_arrow_filter);

    if (move_down) {
        while ((arrow = vector_iterator_next(&it))) {
            // Get next selected arrow
            info->cur_arrow = vector_iterator_current(&it);

            // We have reached our destination
            if (info->cur_arrow >= arrowindex) {
                break;
            }
        }
    } else {
        while ((arrow = vector_iterator_prev(&it))) {
            // Get previous selected arrow
            info->cur_arrow = vector_iterator_current(&it);

            // We have reached our destination
            if (info->cur_arrow <= arrowindex) {
                break;
            }
        }
    }

    // Update the first displayed arrow
    if (info->cur_arrow <= info->first_arrow) {
        info->first_arrow = info->cur_arrow;
    } else {
        // Draw the scrollbar
        vector_iterator_set_current(&it, info->first_arrow - 1);
        while ((arrow = vector_iterator_next(&it))) {
            // Increase current arrow height position
            curh += call_flow_arrow_height(ui, arrow);
            // If we have reached current arrow
            if (vector_iterator_current(&it) == info->cur_arrow) {
                if (curh > flowh) {
                    // Go to the next first arrow and check if current arrow
                    // is still out of bottom bounds
                    info->first_arrow++;
                    vector_iterator_set_current(&it, info->first_arrow - 1);
                    curh = 0;
                } else {
                    break;
                }
            }
        }
    }
}

call_flow_arrow_t *
call_flow_arrow_selected(ui_t *ui)
{
    // Get panel info
    call_flow_info_t *info = call_flow_info(ui);
    // No selected call
    if (info->selected == -1)
        return NULL;

    return vector_item(info->darrows, info->selected);

}

struct timeval
call_flow_arrow_time(call_flow_arrow_t *arrow)
{
    struct timeval ts = { 0 };
    sip_msg_t *msg;
    rtp_stream_t *stream;

    if (!arrow)
        return ts;

    if (arrow->type == CF_ARROW_SIP) {
        msg = (sip_msg_t *) arrow->item;
        ts = packet_time(msg->packet);
    } else if (arrow->type == CF_ARROW_RTP) {
        stream = (rtp_stream_t *) arrow->item;
        ts = stream->time;
    }
    return ts;

}

void
call_flow_arrow_sorter(vector_t *vector, void *item)
{
    struct timeval curts, prevts;
    int count = vector_count(vector);
    int i;

    // First item is alway sorted
    if (vector_count(vector) == 1)
        return;

    curts = call_flow_arrow_time(item);

    for (i = count - 2 ; i >= 0; i--) {
        // Get previous arrow
        prevts = call_flow_arrow_time(vector_item(vector, i));
        // Check if the item is already in a sorted position
        if (timeval_is_older(curts, prevts)) {
            vector_insert(vector, item, i + 1);
            return;
        }
    }

    // Put this item at the begining of the vector
    vector_insert(vector, item, 0);
}

int
call_flow_arrow_filter(void *item)
{
    call_flow_arrow_t *arrow = (call_flow_arrow_t *) item;

    // SIP arrows are never filtered
    if (arrow->type == CF_ARROW_SIP && setting_disabled(SETTING_CF_ONLYMEDIA))
        return 1;

    // RTP arrows are only displayed when requested
    if (arrow->type == CF_ARROW_RTP) {
        // Display all streams
        if (setting_enabled(SETTING_CF_MEDIA))
            return 1;
        // Otherwise only show active streams
        if (setting_has_value(SETTING_CF_MEDIA, SETTING_ACTIVE))
            return stream_is_active(arrow->item);
    }

    // Rest of the arrows are never displayed
    return 0;
}
