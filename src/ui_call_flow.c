/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013,2014 Ivan Alonso (Kaian)
 ** Copyright (C) 2013,2014 Irontec SL. All rights reserved.
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
 *
 * @todo Code help screen. Please.
 *
 */
#include <stdlib.h>
#include <string.h>
#include "capture.h"
#include "ui_call_flow.h"
#include "ui_call_raw.h"
#include "ui_msg_diff.h"
#include "option.h"

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
 * | Usefull hotkeys                                        |
 * +--------------------------------------------------------+
 *
 * Some values we have in mind, stored in the info structure of the panel:
 *
 *  - raw_width: this represents the raw message preview width if visible.
 */

/**
 * Ui Structure definition for Call Flow panel
 */
ui_t ui_call_flow = {
    .type = PANEL_CALL_FLOW,
    .panel = NULL,
    .create = call_flow_create,
    .draw = call_flow_draw,
    .handle_key = call_flow_handle_key,
    .help = call_flow_help
};

PANEL *
call_flow_create()
{
    PANEL *panel;
    WINDOW *win;
    int height, width;
    call_flow_info_t *info;

    // Create a new panel to fill all the screen
    panel = new_panel(newwin(LINES, COLS, 0, 0));

    // Initialize Call List specific data
    info = malloc(sizeof(call_flow_info_t));
    memset(info, 0, sizeof(call_flow_info_t));

    // Store it into panel userptr
    set_panel_userptr(panel, (void*) info);

    // Let's draw the fixed elements of the screen
    win = panel_window(panel);
    getmaxyx(win, height, width);

    // Calculate available printable area for messages
    info->flow_win = subwin(win, height - 2 - 2 - 2, width - 2, 4, 0); // Header - Footer - Address
    info->raw_width = 0; // calculated with the available space after drawing columns

    return panel;
}

call_flow_info_t *
call_flow_info(PANEL *panel)
{
    return (call_flow_info_t*) panel_userptr(panel);
}

void
call_flow_destroy(PANEL *panel)
{
    call_flow_info_t *info;
    // Hide the panel
    hide_panel(panel);
    // Free the panel information
    if ((info = call_flow_info(panel))) {
        // Deallocate group memory
        free(info);
    }
    // Delete panel window
    delwin(panel_window(panel));
    // Delete panel
    del_panel(panel);
}

int
call_flow_draw(PANEL *panel)
{
    call_flow_info_t *info;
    sip_msg_t *msg;
    WINDOW *win;
    int height, width, cline = 0;
    char title[256];

    // Get panel information
    info = call_flow_info(panel);

    // Get window of main panel
    win = panel_window(panel);
    getmaxyx(win, height, width);
    werase(win);

    // Set title
    if (info->group->callcnt == 1) {
        sprintf(title, "Call flow for %s",
                call_get_attribute(*info->group->calls, SIP_ATTR_CALLID));
    } else {
        sprintf(title, "Call flow for %d dialogs", info->group->callcnt);
    }

    // Print color mode in title
    if (is_option_enabled("color")) {
        if (is_option_enabled("color.request"))
            strcat(title, " (Color by Request/Response)");
        if (is_option_enabled("color.callid"))
            strcat(title, " (Color by Call-Id)");
        if (is_option_enabled("color.cseq"))
            strcat(title, " (Color by CSeq)");
    }

    // Draw panel title
    draw_title(panel, title);

    // Show some keybinding
    call_flow_draw_footer(panel);

    // Redraw columns
    call_flow_draw_columns(panel);

    // Let's start from the first displayed message (not the first in the call group)
    for (msg = info->first_msg; msg; msg = call_group_get_next_msg(info->group, msg)) {
        // Draw messages until the Message height has been filled
        if (call_flow_draw_message(panel, msg, cline) != 0)
            break;
        // One message fills 2 lines
        cline += 2;
    }

    // If there are only three columns, then draw the raw message on this panel
    if (is_option_enabled("cf.forceraw")) {
        call_flow_draw_raw(panel, info->cur_msg);
    }

    // Draw the scrollbar
    draw_vscrollbar(info->flow_win, call_group_msg_number(info->group, info->first_msg) * 2,
                    call_group_msg_count(info->group) * 2, 1);

    // Redraw flow win
    wnoutrefresh(info->flow_win);

    return 0;

}

void
call_flow_draw_footer(PANEL *panel)
{
    const char *keybindings[] = {
        key_action_key_str(ACTION_PREV_SCREEN), "Calls List",
        key_action_key_str(ACTION_CONFIRM), "Raw Message",
        key_action_key_str(ACTION_SELECT), "Compare",
        key_action_key_str(ACTION_SHOW_HELP), "Help",
        key_action_key_str(ACTION_SDP_INFO), "SDP mode",
        key_action_key_str(ACTION_TOGGLE_RAW), "Toggle Raw",
        key_action_key_str(ACTION_SHOW_FLOW_EX), "Extended",
        key_action_key_str(ACTION_COMPRESS), "Compressed",
        key_action_key_str(ACTION_SHOW_RAW), "Raw",
        key_action_key_str(ACTION_CYCLE_COLOR), "Colour by",
        key_action_key_str(ACTION_INCREASE_RAW), "Increase Raw"
    };

    draw_keybindings(panel, keybindings, 22);
}

int
call_flow_draw_columns(PANEL *panel)
{
    call_flow_info_t *info;
    call_flow_column_t *column;
    WINDOW *win;
    sip_msg_t *msg;
    int flow_height, flow_width;
    const char *coltext;
    char address[50], *end;

    // Get panel information
    info = call_flow_info(panel);
    // Get window of main panel
    win = panel_window(panel);
    getmaxyx(info->flow_win, flow_height, flow_width);

    // Load columns
    for (msg = call_group_get_next_msg(info->group, NULL); msg;
         msg = call_group_get_next_msg(info->group, msg)) {
        call_flow_column_add(panel, CALLID(msg), SRC(msg));
        call_flow_column_add(panel, CALLID(msg), DST(msg));
    }

    // Draw vertical columns lines
    for (column = info->columns; column; column = column->next) {
        mvwvline(info->flow_win, 0, 20 + 30 * column->colpos, ACS_VLINE, flow_height);
        mvwhline(win, 3, 10 + 30 * column->colpos, ACS_HLINE, 20);
        mvwaddch(win, 3, 20 + 30 * column->colpos, ACS_TTEE);

        // Set bold to this address if it's local
        strcpy(address, column->addr);
        if ((end = strchr(address, ':')))
            *end = '\0';
        if (is_local_address_str(address) && is_option_enabled("cf.localhighlight"))
            wattron(win, A_BOLD);

        coltext = sip_address_port_format(column->addr);
        mvwprintw(win, 2, 10 + 30 * column->colpos + (22 - strlen(coltext)) / 2, "%s", coltext);
        wattroff(win, A_BOLD);
    }

    return 0;
}

int
call_flow_draw_message(PANEL *panel, sip_msg_t *msg, int cline)
{
    call_flow_info_t *info;
    WINDOW *win;
    const char *msg_time;
    const char *msg_callid;
    const char *msg_method;
    const char *msg_from;
    const char *msg_to;
    const char *msg_src;
    const char *msg_dst;
    char method[80];
    int height, width;
    int rtp_count, sdp_port, sdp_local;

    // Get panel information
    info = call_flow_info(panel);
    // Get the messages window
    win = info->flow_win;
    getmaxyx(win, height, width);

    // Get the current line in the win
    if (cline > height + 2)
        return 1;

    // Get message attributes
    msg_time = msg_get_attribute(msg, SIP_ATTR_TIME);
    msg_callid = msg_get_attribute(msg, SIP_ATTR_CALLID);
    msg_method = msg_get_attribute(msg, SIP_ATTR_METHOD);
    msg_from = msg_get_attribute(msg, SIP_ATTR_SIPFROM);
    msg_to = msg_get_attribute(msg, SIP_ATTR_SIPTO);
    msg_src = msg_get_attribute(msg, SIP_ATTR_SRC);
    msg_dst = msg_get_attribute(msg, SIP_ATTR_DST);

    // Print timestamp
    mvwprintw(win, cline, 2, "%s", msg_time);

    // Get Message method (include extra info)
    memset(method, 0, sizeof(method));
    if (msg_method)
        sprintf(method, "%s", msg_method);

    // If message has sdp information
    if (msg->sdp) {
        // Change method text if sdpinfo mode is requested
        if (is_option_enabled("cf.sdpinfo")) {
            // Show message sdp in title
            memset(method, 0, sizeof(method));
            strncpy(method, msg_method, 3);
            sprintf(method + strlen(method), " (%s:%s)",
                    msg_get_attribute(msg, SIP_ATTR_SDP_ADDRESS),
                    msg_get_attribute(msg, SIP_ATTR_SDP_PORT));
        } else {
            // Show sdp tag in tittle
            strcat(method, " (SDP)");
        }
    }

    // Draw message type or status and line
    int msglen = strlen(method);
    if (msglen > 24)
        msglen = 24;

    // Get origin and destination column
    call_flow_column_t *column1 = call_flow_column_get(panel, msg_callid, msg_src);
    call_flow_column_t *column2 = call_flow_column_get(panel, msg_callid, msg_dst);

    call_flow_column_t *tmp;
    if (column1->colpos > column2->colpos) {
        tmp = column1;
        column1 = column2;
        column2 = tmp;
    }

    int startpos = 20 + 30 * column1->colpos;
    int endpos = 20 + 30 * column2->colpos;
    int distance = abs(endpos - startpos) - 3;

    // Highlight current message
    if (msg == info->cur_msg) {
        if (is_option_value("cf.highlight", "reverse")) {
            wattron(win, A_REVERSE);
        }
        if (is_option_value("cf.highlight", "bold")) {
            wattron(win, A_BOLD);
        }
        if (is_option_value("cf.highlight", "reversebold")) {
            wattron(win, A_REVERSE);
            wattron(win, A_BOLD);
        }
    }

    // Color the message {
    if (is_option_enabled("color.request")) {
        // Determine arrow color
        if (msg_is_request(msg)) {
            msg->color = CP_RED_ON_DEF;
        } else {
            msg->color = CP_GREEN_ON_DEF;
        }
    } else if (is_option_enabled("color.callid")) {
        // Color by call-id
        msg->color = call_group_color(info->group, msg->call);
    } else if (is_option_enabled("color.cseq")) {
        // Color by CSeq within the same call
        msg->color = msg->cseq % 7 + 1;
    }

    // Turn on the message color
    wattron(win, COLOR_PAIR(msg->color));

    mvwprintw(win, cline, startpos + 2, "%*s", distance, "");
    mvwprintw(win, cline, startpos + distance / 2 - msglen / 2 + 2, "%.26s", method);
    if (msg == info->selected) {
        mvwhline(win, cline + 1, startpos + 2, '=', distance);
    } else {
        mvwhline(win, cline + 1, startpos + 2, ACS_HLINE, distance);
    }


    // If message has SDP check if packet count in SDP Port has changed
    if (info->show_rtp && msg->sdp) {
        // Check if SDP address is local
        sdp_local = is_local_address_str(msg_get_attribute(msg, SIP_ATTR_SDP_ADDRESS));
        // Check if packet count has changed in SDP port
        sdp_port = atoi(msg_get_attribute(msg, SIP_ATTR_SDP_PORT));
        // Retrieve packet count for SDP port
        rtp_count = capture_packet_count_port(sdp_local, sdp_port);

        if (msg->rtp_count != rtp_count) {
            msg->rtp_count = rtp_count;
            msg->rtp_pos = (msg->rtp_pos + 1) % distance;

            if (!strcasecmp(msg_src, column1->addr)) {
                mvwaddch(win, cline + 1, startpos + distance - msg->rtp_pos, '<');
            } else {
                mvwaddch(win, cline + 1, startpos + 2 + msg->rtp_pos, '>');
            }
        }
    }

    // Write the arrow at the end of the message (two arros if this is a retrans)
    if (!strcasecmp(msg_src, column1->addr)) {
        mvwaddch(win, cline + 1, endpos - 2, '>');
        if (msg_is_retrans(msg)) {
            mvwaddch(win, cline + 1, endpos - 3, '>');
            mvwaddch(win, cline + 1, endpos - 4, '>');
        }
    } else {
        mvwaddch(win, cline + 1, startpos + 2, '<');
        if (msg_is_retrans(msg)) {
            mvwaddch(win, cline + 1, startpos + 3, '<');
            mvwaddch(win, cline + 1, startpos + 4, '<');
        }
    }

    // Turn off colors
    wattroff(win, COLOR_PAIR(CP_RED_ON_DEF));
    wattroff(win, COLOR_PAIR(CP_GREEN_ON_DEF));
    wattroff(win, COLOR_PAIR(CP_CYAN_ON_DEF));
    wattroff(win, COLOR_PAIR(CP_YELLOW_ON_DEF));
    wattroff(win, A_BOLD);
    wattroff(win, A_REVERSE);

    return 0;
}

int
call_flow_draw_raw(PANEL *panel, sip_msg_t *msg)
{
    call_flow_info_t *info;
    WINDOW *win, *raw_win;
    int raw_width, raw_height, height, width;

    // Get panel information
    info = call_flow_info(panel);

    // Get window of main panel
    win = panel_window(panel);
    getmaxyx(win, height, width);

    // Calculate the raw data width (width - used columns for flow - vertical lines)
    raw_width = width - (31 + 30 * info->columns->colpos) - 2;
    // We can define a mininum size for rawminwidth
    if (raw_width < get_option_int_value("cf.rawminwidth")) {
        raw_width = get_option_int_value("cf.rawminwidth");
    }
    // We can configure an exact raw size
    if (get_option_int_value("cf.rawfixedwidth") != -1) {
        raw_width = get_option_int_value("cf.rawfixedwidth");
    }

    // Height of raw window is always available size minus 6 lines for header/footer
    raw_height = height - 3;

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
    wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    mvwvline(win, 1, width - raw_width - 2, ACS_VLINE, height - 2);
    wattroff(win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Print msg payload
    draw_message(info->raw_win, msg);

    // Copy the raw_win contents into the panel
    copywin(raw_win, win, 0, 0, 1, width - raw_width - 1, raw_height, width - 2, 0);

    return 0;
}

int
call_flow_handle_key(PANEL *panel, int key)
{
    int i, raw_width, height, width;
    call_flow_info_t *info = call_flow_info(panel);
    sip_msg_t *next = NULL, *prev = NULL;
    ui_t *next_panel;
    sip_call_group_t *group;
    int rnpag_steps = get_option_int_value("cf.scrollstep");
    int action = -1;

    // Sanity check, this should not happen
    if (!info)
        return -1;

    getmaxyx(info->flow_win, height, width);

    // Check actions for this key
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch(action) {
            case ACTION_DOWN:
                // Check if there is a call below us
                if (!(next = call_group_get_next_msg(info->group, info->cur_msg)))
                    break;
                info->cur_msg = next;
                info->cur_line += 2;
                // If we are out of the bottom of the displayed list
                // refresh it starting in the next call
                if (info->cur_line >= height) {
                    info->first_msg = call_group_get_next_msg(info->group, info->first_msg);
                    info->cur_line -= 2;
                }
                break;
            case ACTION_UP:
                // Loop through messages until we found current message
                // storing the previous one
                while ((next = call_group_get_next_msg(info->group, next))) {
                    if (next == info->cur_msg)
                        break;
                    prev = next;
                }
                // We're at the first message already
                if (!prev)
                    break;
                info->cur_msg = prev;
                info->cur_line -= 2;
                if (info->cur_line <= 0) {
                    info->first_msg = info->cur_msg;
                    info->cur_line += 2;
                }
                break;
            case ACTION_HNPAGE:
                rnpag_steps = rnpag_steps / 2;
                /* no break */
            case ACTION_NPAGE:
                // Next page => N key down strokes
                for (i = 0; i < rnpag_steps; i++)
                    call_flow_handle_key(panel, KEY_DOWN);
                break;
            case ACTION_HPPAGE:
                rnpag_steps = rnpag_steps / 2;
                /* no break */
            case ACTION_PPAGE:
                // Prev page => N key up strokes
                for (i = 0; i < rnpag_steps; i++)
                    call_flow_handle_key(panel, KEY_UP);
                break;
            case ACTION_TOGGLE_RTP:
                 info->show_rtp = (info->show_rtp)?0:1;
                 break;
            case ACTION_SHOW_FLOW_EX:
                werase(panel_window(panel));
                // KEY_X , Display current call flow
                if (info->group->callcnt == 1) {
                    group = call_group_create();
                    call_group_add(group, info->group->calls[0]);
                    call_group_add(group, call_get_xcall(info->group->calls[0]));
                    call_flow_set_group(group);
                } else {
                    group = call_group_create();
                    call_group_add(group, info->group->calls[0]);
                    call_flow_set_group(group);
                }
                break;
            case ACTION_SHOW_RAW:
                // KEY_R, display current call in raw mode
                next_panel = ui_create(ui_find_by_type(PANEL_CALL_RAW));
                // TODO
                call_raw_set_group(info->group);
                wait_for_input(next_panel);
                break;
            case ACTION_INCREASE_RAW:
                raw_width = getmaxx(info->raw_win);
                if (raw_width - 2 > 1) {
                    set_option_int_value("cf.rawfixedwidth", raw_width - 2);
                }
                break;
            case ACTION_DECREASE_RAW:
                raw_width = getmaxx(info->raw_win);
                if (raw_width + 2 < COLS - 1) {
                    set_option_int_value("cf.rawfixedwidth", raw_width + 2);
                }
                break;
            case ACTION_RESET_RAW:
                set_option_int_value("cf.rawfixedwidth", -1);
                break;
            case ACTION_ONLY_SDP:
                // Toggle SDP mode
                info->group->sdp_only = !(info->group->sdp_only);
                call_flow_set_group(info->group);
                break;
            case ACTION_SDP_INFO:
                set_option_value("cf.sdpinfo", is_option_enabled("cf.sdpinfo") ? "off" : "on");
                break;
            case ACTION_TOGGLE_RAW:
                set_option_value("cf.forceraw", is_option_enabled("cf.forceraw") ? "off" : "on");
                break;
            case ACTION_COMPRESS:
                set_option_value("cf.splitcallid", is_option_enabled("cf.splitcallid") ? "off" : "on");
                // Force columns reload
                info->columns = NULL;
                break;
            case ACTION_SELECT:
                if (!info->selected) {
                    info->selected = info->cur_msg;
                } else {
                    if (info->selected == info->cur_msg) {
                        info->selected = NULL;
                    } else {
                        // Show diff panel
                        next_panel = ui_create(ui_find_by_type(PANEL_MSG_DIFF));
                        msg_diff_set_msgs(ui_get_panel(next_panel), info->selected, info->cur_msg);
                        wait_for_input(next_panel);
                    }
                }
                break;
            case ACTION_CONFIRM:
                // KEY_ENTER, display current message in raw mode
                next_panel = ui_create(ui_find_by_type(PANEL_CALL_RAW));
                // TODO
                call_raw_set_group(info->group);
                call_raw_set_msg(info->cur_msg);
                wait_for_input(next_panel);
                break;
            default:
                // Parse next action
                continue;
        }

        // We've handled this key, stop checking actions
        break;
    }

    // Return if this panel has handled or not the key
    return (action == ERR) ? key : 0;
}

int
call_flow_help(PANEL *panel)
{
    WINDOW *help_win;
    PANEL *help_panel;
    int height, width;

    // Create a new panel and show centered
    height = 26;
    width = 65;
    help_win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);
    help_panel = new_panel(help_win);

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
    mvwprintw(help_win, 10, 2, "Enter       Show current message Raw");
    mvwprintw(help_win, 11, 2, "F1/h        Show this screen");
    mvwprintw(help_win, 12, 2, "F2/d        Toggle SDP Address:Port info");
    mvwprintw(help_win, 13, 2, "F3/t        Toggle raw preview display");
    mvwprintw(help_win, 14, 2, "F4/X        Show call-flow with X-CID/X-Call-ID dialog");
    mvwprintw(help_win, 15, 2, "F5/S        Toggle compressed view (One address <=> one column");
    mvwprintw(help_win, 16, 2, "F6/R        Show original call messages in raw mode");
    mvwprintw(help_win, 17, 2, "F7/c        Cycle between available color modes");
    mvwprintw(help_win, 18, 2, "F8/C        Turn on/off message syntax highlighting");
    mvwprintw(help_win, 19, 2, "F9/l        Turn on/off resolved addresses");
    mvwprintw(help_win, 20, 2, "9/0         Increase/Decrease raw preview size");
    mvwprintw(help_win, 21, 2, "T           Restore raw preview size");
    mvwprintw(help_win, 22, 2, "D           Only show SDP messages");

    // Press any key to close
    wgetch(help_win);
    update_panels();
    doupdate();

    return 0;
}

int
call_flow_set_group(sip_call_group_t *group)
{
    PANEL *panel;
    call_flow_info_t *info;

    if (!(panel = ui_get_panel(ui_find_by_type(PANEL_CALL_FLOW))))
        return -1;

    if (!(info = call_flow_info(panel)))
        return -1;

    info->group = group;
    info->cur_msg = info->first_msg = call_group_get_next_msg(group, NULL);
    info->cur_line = 1;
    info->columns = NULL;

    return 0;
}

void
call_flow_column_add(PANEL *panel, const char *callid, const char *addr)
{
    call_flow_info_t *info;
    call_flow_column_t *column;
    int colpos = 0;

    if (!(info = call_flow_info(panel)))
        return;

    if (call_flow_column_get(panel, callid, addr))
        return;

    column = info->columns;
    while (column) {
        if (!strcasecmp(addr, column->addr) && column->colpos != 0 && !column->callid2) {
            column->callid2 = callid;
            return;
        }
        column = column->next;
    }

    if (info->columns)
        colpos = info->columns->colpos + 1;

    column = malloc(sizeof(call_flow_column_t));
    memset(column, 0, sizeof(call_flow_column_t));
    column->callid = callid;
    column->addr = addr;
    column->colpos = colpos;
    column->next = info->columns;
    info->columns = column;
}

call_flow_column_t *
call_flow_column_get(PANEL *panel, const char *callid, const char *addr)
{
    call_flow_info_t *info;
    call_flow_column_t *columns;

    if (!(info = call_flow_info(panel)))
        return NULL;

    columns = info->columns;
    while (columns) {
        if (!strcasecmp(addr, columns->addr)) {
            if (is_option_enabled("cf.splitcallid"))
                return columns;
            if (columns->callid && !strcasecmp(callid, columns->callid))
                return columns;
            if (columns->callid2 && !strcasecmp(callid, columns->callid2))
                return columns;
        }
        columns = columns->next;
    }
    return NULL;
}

