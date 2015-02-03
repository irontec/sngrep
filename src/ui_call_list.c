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
 * @file ui_call_list.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_call_list.h
 *
 */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "option.h"
#include "ui_call_list.h"
#include "ui_call_flow.h"
#include "ui_call_raw.h"
#include "ui_save_pcap.h"

PANEL *
call_list_create()
{
    PANEL *panel;
    WINDOW *win;
    int height, width, i, attrid, collen;
    call_list_info_t *info;
    char option[80];
    const char *field, *title;

    // Create a new panel that fill all the screen
    panel = new_panel(newwin(LINES, COLS, 0, 0));
    // Initialize Call List specific data
    info = malloc(sizeof(call_list_info_t));
    memset(info, 0, sizeof(call_list_info_t));
    // Store it into panel userptr
    set_panel_userptr(panel, (void*) info);

    // Add configured columns
    for (i = 0; i < SIP_ATTR_SENTINEL; i++) {
        // Get column attribute name from options
        sprintf(option, "cl.column%d", i);
        if ((field = get_option_value(option))) {
            if ((attrid = sip_attr_from_name(field)) == -1)
                continue;
            // Get column width from options
            sprintf(option, "cl.column%d.width", i);
            if ((collen = get_option_int_value(option)) == -1)
                collen = sip_attr_get_width(attrid);
            // Get column title
            title = sip_attr_get_description(attrid);
            // Add column to the list
            call_list_add_column(panel, attrid, field, title, collen);
        }
    }

    // Let's draw the fixed elements of the screen
    win = panel_window(panel);
    getmaxyx(win, height, width);

    // Initialize the fields
    info->fields[FLD_LIST_FILTER] = new_field(1, width - 19, 2, 18, 0, 0);
    info->fields[FLD_LIST_COUNT] = NULL;

    // Create the form and post it
    info->form = new_form(info->fields);
    set_form_sub(info->form, win);
    // Form starts inactive
    call_list_form_activate(panel, false);

    // Calculate available printable area
    info->list_win = subwin(win, height - 5, width, 4, 0);
    info->group = call_group_create();

    // Draw a Panel header lines
    if (get_option_value("capture.infile"))
        mvwprintw(win, 1, width - strlen(get_option_value("capture.infile")) - 11, "Filename: %s",
                  get_option_value("capture.infile"));
    mvwprintw(win, 2, 2, "Display Filter: ");

    // Draw the footer of the window
    call_list_draw_footer(panel);

    wattron(win, COLOR_PAIR(CP_DEF_ON_CYAN));
    // Draw panel title
    mvwprintw(win, 0, 0, "%*s", width, "");
    mvwprintw(win, 0, (width - 45) / 2, "sngrep - SIP messages flow viewer");
    wattroff(win, COLOR_PAIR(CP_DEF_ON_CYAN));

    // Set defualt filter text if configured
    if (get_option_value("cl.filter")) {
        set_field_buffer(info->fields[FLD_LIST_FILTER], 0, get_option_value("cl.filter"));
        call_list_form_activate(panel, false);
    }

    // Return the created panel
    return panel;
}

void
call_list_destroy(PANEL *panel)
{
    call_list_info_t *info;

    // Hide the panel
    hide_panel(panel);

    // Free its status data
    if ((info = (call_list_info_t*) panel_userptr(panel))) {

        // Deallocate forms data
        if (info->form) {
            unpost_form(info->form);
            free_form(info->form);
            free_field(info->fields[FLD_LIST_FILTER]);
        }

        // Deallocate group data
        if (info->group)
            call_group_destroy(info->group);
        free(info);
    }

    // Finally free the panel memory
    del_panel(panel);
}

void
call_list_draw_footer(PANEL *panel)
{
    const char *keybindings[] =
        {
          "Esc",
          "Quit",
          "Enter",
          "Show",
          "Space",
          "Select",
          "F1",
          "Help",
          "F2",
          "Save",
          "F3",
          "Search",
          "F4",
          "Extended",
          "F5",
          "Clear",
          "F6",
          "Raw",
          "F7",
          "Filter",
          "F10",
          "Columns" };

    draw_keybindings(panel, keybindings, 22);
}

int
call_list_redraw_required(PANEL *panel, sip_msg_t *msg)
{
    //@todo alway redraw this screen on new messages
    return 0;
}

int
call_list_draw(PANEL *panel)
{
    int height, width, cline = 0, i, colpos, collen;
    struct sip_call *call;
    int callcnt;
    const char *ouraddr;
    const char *coldesc;
    char linetext[256];

    // Get panel info
    call_list_info_t *info = (call_list_info_t*) panel_userptr(panel);
    if (!info)
        return -1;

    // Get window of call list panel
    WINDOW *win = panel_window(panel);
    getmaxyx(win, height, width);

    // Update current mode information
    if (!info->form_active) {
        mvwprintw(win, 1, 2, "Current Mode: %s %9s", get_option_value("sngrep.mode"),
                  (is_option_enabled("sip.capture") ? "" : "(Stopped)"));
    }

    // Draw columns titles
    wattron(win, COLOR_PAIR(CP_DEF_ON_CYAN));
    mvwprintw(win, 3, 0, "%*s", width, "");
    for (colpos = 6, i = 0; i < info->columncnt; i++) {
        // Get current column width
        collen = info->columns[i].width;
        // Get current column title
        coldesc = sip_attr_get_description(info->columns[i].id);

        // Check if the column will fit in the remaining space of the screen
        if (colpos + strlen(coldesc) >= width)
            break;
        mvwprintw(win, 3, colpos, "%.*s", collen, coldesc);
        colpos += collen + 1;
    }
    wattroff(win, COLOR_PAIR(CP_DEF_ON_CYAN));

    // Get window of call list panel
    win = info->list_win;
    getmaxyx(win, height, width);

    // Get available calls counter (we'll use it here a couple of times)
    callcnt = call_list_count(panel);

    // No calls, we've finished drawing
    if (callcnt == 0)
        return 0;

    // If no active call, use the fist one (if exists)
    if (!info->first_call && call_list_get_next(panel, NULL)) {
        info->cur_call = info->first_call = call_list_get_next(panel, NULL);
        info->cur_line = info->first_line = 1;
    }

    // Fill the call list
    for (call = info->first_call; call; call = call_list_get_next(panel, call)) {
        // Stop if we have reached the bottom of the list
        if (cline == height)
            break;

        // We only print calls with messages (In fact, all call should have msgs)
        if (!call_msg_count(call))
            continue;

        // TODO Useless feature :|
        if ((ouraddr = get_option_value("address"))) {
            if (!strcasecmp(ouraddr, call_get_attribute(call, SIP_ATTR_SRC))) {
                wattron(win, COLOR_PAIR(CP_RED_ON_DEF));
            } else if (!strcasecmp(ouraddr, call_get_attribute(call, SIP_ATTR_DST))) {
                wattron(win, COLOR_PAIR(CP_GREEN_ON_DEF));
            }
        }

        // Show bold selected rows
        if (call_group_exists(info->group, call)) {
            wattron(win, A_BOLD);
            wattron(win, COLOR_PAIR(CP_DEFAULT));
        }

        // Highlight active call
        if (call == info->cur_call) {
            wattron(win, COLOR_PAIR(CP_DEF_ON_BLUE));
        }

        // Set current line background
        mvwprintw(win, cline, 0, "%*s", width, "");
        // Set current line selection box
        mvwprintw(win, cline, 2, call_group_exists(info->group, call) ? "[*]" : "[ ]");

        // Print call line if no filter is active or it matchs the filter
        memset(linetext, 0, sizeof(linetext));
        mvwprintw(win, cline, 6, "%s", call_list_line_text(panel, call, linetext));
        cline++;

        wattroff(win, COLOR_PAIR(CP_DEFAULT));
        wattroff(win, COLOR_PAIR(CP_BLUE_ON_DEF));
        wattroff(win, A_BOLD);
    }

    // Draw scrollbar to the right
    draw_vscrollbar(win, info->first_line, callcnt, true);
    wnoutrefresh(info->list_win);

    return 0;
}

void
call_list_form_activate(PANEL *panel, bool active)
{
    call_list_info_t *info = (call_list_info_t*) panel_userptr(panel);

    // Store form state
    info->form_active = active;

    if (active) {
        set_current_field(info->form, info->fields[FLD_LIST_FILTER]);
        // Show cursor
        curs_set(1);
        // Change current field background
        set_field_back(info->fields[FLD_LIST_FILTER], A_REVERSE);
    } else {
        set_current_field(info->form, NULL);
        // Hide cursor
        curs_set(0);
        // Change current field background
        set_field_back(info->fields[FLD_LIST_FILTER], A_NORMAL);
    }
    post_form(info->form);
    form_driver(info->form, REQ_END_LINE);
}

const char *
call_list_line_text(PANEL *panel, sip_call_t *call, char *text)
{
    int i, collen;
    const char *call_attr;
    char coltext[256];
    int colid;
    int width;

    WINDOW *win = panel_window(panel);

    // Get window width
    width = getmaxx(win);

    // Get panel info
    call_list_info_t *info = (call_list_info_t*) panel_userptr(panel);

    // Print requested columns
    for (i = 0; i < info->columncnt; i++) {

        // Swappable columns
        switch (info->columns[i].id) {
        case SIP_ATTR_SRC:
        case SIP_ATTR_SRC_HOST:
            colid = (is_option_enabled("sngrep.displayhost")) ? SIP_ATTR_SRC_HOST : SIP_ATTR_SRC;
            break;
        case SIP_ATTR_DST:
        case SIP_ATTR_DST_HOST:
            colid = (is_option_enabled("sngrep.displayhost")) ? SIP_ATTR_DST_HOST : SIP_ATTR_DST;
            break;
        default:
            colid = info->columns[i].id;
        }

        // Get current column width
        collen = info->columns[i].width;

        // Check if next column fits on window width
        if (strlen(text) + collen >= width)
            collen -= width - strlen(text);

        // If no space left on the screen stop processing columns
        if (collen <= 0)
            break;

        // Initialize column text
        memset(coltext, 0, sizeof(coltext));
        // Get call attribute for current column
        if ((call_attr = call_get_attribute(call, colid))) {
            sprintf(coltext, "%.*s", collen, call_attr);
        }
        // Add the column text to the existing columns
        sprintf(text + strlen(text), "%-*s ", collen, coltext);
    }

    return text;
}

int
call_list_handle_key(PANEL *panel, int key)
{
    int i, height, width, rnpag_steps = get_option_int_value("cl.scrollstep");
    call_list_info_t *info = (call_list_info_t*) panel_userptr(panel);
    ui_t *next_panel;
    sip_call_group_t *group;

    // Sanity check, this should not happen
    if (!info)
        return -1;

    // Handle form key
    if (info->form_active)
        return call_list_handle_form_key(panel, key);

    // Get window of call list panel
    WINDOW *win = info->list_win;
    getmaxyx(win, height, width);

    switch (key) {
    case '/':
    case 9 /*KEY_TAB*/:
    case KEY_F(3):
        // Activate Form
        call_list_form_activate(panel, true);
        break;
    case KEY_DOWN:
        // Check if there is a call below us
        if (!info->cur_call || !call_list_get_next(panel, info->cur_call))
            break;
        info->cur_call = call_list_get_next(panel, info->cur_call);
        info->cur_line++;
        // If we are out of the bottom of the displayed list
        // refresh it starting in the next call
        if (info->cur_line > height) {
            info->first_call = call_list_get_next(panel, info->first_call);
            info->first_line++;
            info->cur_line = height;
        }
        break;
    case KEY_UP:
        // Check if there is a call above us
        if (!info->cur_call || !call_list_get_prev(panel, info->cur_call))
            break;
        info->cur_call = call_list_get_prev(panel, info->cur_call);
        info->cur_line--;
        // If we are out of the top of the displayed list
        // refresh it starting in the previous (in fact current) call
        if (info->cur_line <= 0) {
            info->first_call = info->cur_call;
            info->first_line--;
            info->cur_line = 1;
        }
        break;
    case KEY_NPAGE:
        // Next page => N key down strokes
        for (i = 0; i < rnpag_steps; i++)
            call_list_handle_key(panel, KEY_DOWN);
        break;
    case KEY_PPAGE:
        // Prev page => N key up strokes
        for (i = 0; i < rnpag_steps; i++)
            call_list_handle_key(panel, KEY_UP);
        break;
    case 10:
        if (!info->cur_call)
            return -1;
        // KEY_ENTER , Display current call flow
        next_panel = ui_create(ui_find_by_type(DETAILS_PANEL));
        if (info->group->callcnt) {
            group = info->group;
        } else {
            if (!info->cur_call)
                return -1;
            group = call_group_create();
            call_group_add(group, info->cur_call);
        }
        call_flow_set_group(group);
        wait_for_input(next_panel);
        break;
    case 'x':
    case KEY_F(4):
        // KEY_X , Display current call flow (extended)
        next_panel = ui_create(ui_find_by_type(DETAILS_PANEL));
        if (info->group->callcnt) {
            group = info->group;
        } else {
            if (!info->cur_call)
                return -1;
            group = call_group_create();
            call_group_add(group, info->cur_call);
            call_group_add(group, call_get_xcall(info->cur_call));
        }
        call_flow_set_group(group);
        wait_for_input(next_panel);
        break;
    case 'r':
    case 'R':
    case KEY_F(6):
        // KEY_R , Display current call flow (extended)
        next_panel = ui_create(ui_find_by_type(RAW_PANEL));
        if (info->group->callcnt) {
            group = info->group;
        } else {
            if (!info->cur_call)
                return -1;
            group = call_group_create();
            call_group_add(group, info->cur_call);
        }
        call_raw_set_group(group);
        wait_for_input(next_panel);
        break;
    case 'f':
    case 'F':
    case KEY_F(7):
        // KEY_F, Display filter panel
        next_panel = ui_create(ui_find_by_type(FILTER_PANEL));
        wait_for_input(next_panel);
        call_list_filter_update(panel);
        break;
    case 't':
    case 'T':
    case KEY_F(10):
        // Display column selection panel
        next_panel = ui_create(ui_find_by_type(COLUMN_SELECT_PANEL));
        wait_for_input(next_panel);
        call_list_filter_update(panel);
        break;
    case 's':
    case 'S':
    case KEY_F(2):
        if (!is_option_disabled("sngrep.tmpfile")) {
            // KEY_S, Display save panel
            next_panel = ui_create(ui_find_by_type(SAVE_PANEL));
            save_set_group(ui_get_panel(next_panel), info->group);
            wait_for_input(next_panel);
        }
        break;
    case 'i':
    case 'I':
        // Set Display filter text
        set_field_buffer(info->fields[FLD_LIST_FILTER], 0, "invite");
        call_list_filter_update(panel);
        break;
    case KEY_F(5):
        // Remove all stored calls
        sip_calls_clear();
        // Clear List
        call_list_clear(panel);
        break;
    case ' ':
        if (!info->cur_call)
            return -1;
        if (call_group_exists(info->group, info->cur_call)) {
            call_group_del(info->group, info->cur_call);
        } else {
            call_group_add(info->group, info->cur_call);
        }
        break;
    case 'q':
    case 27: /* KEY_ESC */
        // Handle quit from this screen unless requested
        if (!is_option_enabled("cl.noexitprompt")) {
            return call_list_exit_confirm(panel);
        }
        break;
    default:
        return key;
    }

    return 0;
}

int
call_list_handle_form_key(PANEL *panel, int key)
{
    int field_idx;

    // Get panel information
    call_list_info_t *info = (call_list_info_t*) panel_userptr(panel);

    // Get current field id
    field_idx = field_index(current_field(info->form));

    switch (key) {
    case 27: /* KEY_ESC */
    case 9 /* KEY_TAB */:
    case 10 /* KEY_ENTER */:
    case KEY_DOWN:
    case KEY_UP:
        // Activate list
        call_list_form_activate(panel, false);
        break;
    case KEY_RIGHT:
        form_driver(info->form, REQ_RIGHT_CHAR);
        break;
    case KEY_LEFT:
        form_driver(info->form, REQ_LEFT_CHAR);
        break;
    case KEY_HOME:
        form_driver(info->form, REQ_BEG_LINE);
        break;
    case KEY_END:
        form_driver(info->form, REQ_END_LINE);
        break;
    case KEY_DC:
        form_driver(info->form, REQ_DEL_CHAR);
        break;
    case 8:
    case 127:
    case KEY_BACKSPACE:
        form_driver(info->form, REQ_DEL_PREV);
        // Updated displayed results
        call_list_filter_update(panel);
        break;
    default:
        // If this is a normal character on input field, print it
        form_driver(info->form, key);
        // Updated displayed results
        call_list_filter_update(panel);
        break;
    }

    // Validate all input data
    form_driver(info->form, REQ_VALIDATION);

    return 0;
}

int
call_list_help(PANEL *panel)
{
    WINDOW *help_win;
    PANEL *help_panel;
    int height, width;

    // Create a new panel and show centered
    height = 27;
    width = 65;
    help_win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);
    help_panel = new_panel(help_win);

    // Set the window title
    mvwprintw(help_win, 1, 25, "Call List Help");

    // Write border and boxes around the window
    wattron(help_win, COLOR_PAIR(CP_BLUE_ON_DEF));
    box(help_win, 0, 0);
    mvwhline(help_win, 2, 1, ACS_HLINE, width - 2);
    mvwhline(help_win, 7, 1, ACS_HLINE, width - 2);
    mvwhline(help_win, height - 3, 1, ACS_HLINE, width - 2);
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
    mvwprintw(help_win, 3, 2, "This windows show the list of parsed calls from a pcap file ");
    mvwprintw(help_win, 4, 2, "(Offline) or a live capture with libpcap functions (Online).");
    mvwprintw(help_win, 5, 2, "You can configure the columns shown in this screen and some");
    mvwprintw(help_win, 6, 2, "static filters using sngreprc resource file.");
    wattroff(help_win, COLOR_PAIR(CP_CYAN_ON_DEF));

    // A list of available keys in this window
    mvwprintw(help_win, 8, 2, "Available keys:");
    mvwprintw(help_win, 10, 2, "Esc/Q       Exit sngrep.");
    mvwprintw(help_win, 11, 2, "Enter       Show selected calls message flow");
    mvwprintw(help_win, 12, 2, "Space       Select call");
    mvwprintw(help_win, 13, 2, "F1/h        Show this screen");
    mvwprintw(help_win, 14, 2, "F2/S        Save captured packages to a file");
    mvwprintw(help_win, 15, 2, "F3//        Display filtering (match string case insensitive)");
    mvwprintw(help_win, 16, 2, "F4/X        Show selected call-flow (Extended) if available");
    mvwprintw(help_win, 17, 2, "F5          Clear call list (can not be undone!)");
    mvwprintw(help_win, 18, 2, "F6/R        Show selected call messages in raw mode");
    mvwprintw(help_win, 19, 2, "F7/F        Show filter options");
    mvwprintw(help_win, 20, 2, "F8/c        Turn on/off window colours");
    mvwprintw(help_win, 21, 2, "F9/l        Turn on/off resolved addresses");
    mvwprintw(help_win, 22, 2, "F10/t       Select displayed columns");
    mvwprintw(help_win, 23, 2, "i/I         Set display filter to invite");

    // Press any key to close
    wgetch(help_win);
    update_panels();
    doupdate();

    return 0;
}

int
call_list_exit_confirm(PANEL *panel)
{
    WINDOW *exit_win;
    PANEL *exit_panel;
    int c;
    // Initial exit status
    int exit = get_option_int_value("cl.defexitbutton");

    // Create a new panel and show centered
    exit_win = newwin(8, 40, (LINES - 8) / 2, (COLS - 40) / 2);
    exit_panel = new_panel(exit_win);
    keypad(exit_win, TRUE);

    // Set the window title
    mvwprintw(exit_win, 1, 13, "Confirm exit");

    // Write border and boxes around the window
    wattron(exit_win, COLOR_PAIR(CP_BLUE_ON_DEF));
    box(exit_win, 0, 0);
    mvwhline(exit_win, 2, 1, ACS_HLINE, 40);
    mvwhline(exit_win, 5, 1, ACS_HLINE, 40);
    mvwaddch(exit_win, 2, 0, ACS_LTEE);
    mvwaddch(exit_win, 5, 0, ACS_LTEE);
    mvwaddch(exit_win, 2, 39, ACS_RTEE);
    mvwaddch(exit_win, 5, 39, ACS_RTEE);

    // Exit confirmation message message
    wattron(exit_win, COLOR_PAIR(CP_CYAN_ON_DEF));
    mvwprintw(exit_win, 3, 2, "Are you sure you want to quit?");
    wattroff(exit_win, COLOR_PAIR(CP_CYAN_ON_DEF));

    for (;;) {
        // A list of available keys in this window
        if (exit)
            wattron(exit_win, A_REVERSE);
        mvwprintw(exit_win, 6, 10, "[  Yes  ]");
        wattroff(exit_win, A_REVERSE);
        if (!exit)
            wattron(exit_win, A_REVERSE);
        mvwprintw(exit_win, 6, 20, "[  No   ]");
        wattroff(exit_win, A_REVERSE);

        update_panels();
        doupdate();

        c = wgetch(exit_win);
        switch (c) {
        case KEY_RIGHT:
            exit = 0;
            break;
        case KEY_LEFT:
            exit = 1;
            break;
        case 9:
            exit = (exit) ? 0 : 1;
            break;
        case 10:
            // If we return ESC, we let ui_manager to handle this
            // key and exit sngrep gracefully
            return (exit) ? 27 : 0;
        }
    }

    return 0;
}

int
call_list_add_column(PANEL *panel, enum sip_attr_id id, const char* attr, const char *title,
                     int width)
{
    call_list_info_t *info = (call_list_info_t*) panel_userptr(panel);
    if (!info)
        return 1;

    info->columns[info->columncnt].id = id;
    info->columns[info->columncnt].attr = attr;
    info->columns[info->columncnt].title = title;
    info->columns[info->columncnt].width = width;
    info->columncnt++;
    return 0;
}

void
call_list_filter_update(PANEL *panel)
{
    // TODO
    // Clear list
    call_list_clear(panel);
}

void
call_list_clear(PANEL *panel)
{
    // Get panel info
    call_list_info_t *info = (call_list_info_t*) panel_userptr(panel);
    if (!info)
        return;

    // Initialize structures
    info->first_call = info->cur_call = NULL;
    info->first_line = info->cur_line = 0;
    info->group->callcnt = 0;

    // Clear Displayed lines
    werase(info->list_win);
    wnoutrefresh(info->list_win);
}

int
call_list_count(PANEL *panel)
{
    int count = 0;
    sip_call_t *call = NULL;
    while ((call = call_list_get_next(panel, call)))
        count++;
    return count;
}

sip_call_t *
call_list_get_next(PANEL *panel, sip_call_t *cur)
{
    sip_call_t *next = cur;
    while ((next = call_get_next(next))) {
        if (call_list_match_dfilter(panel, next))
            return next;
    }
    return NULL;
}

sip_call_t *
call_list_get_prev(PANEL *panel, sip_call_t *cur)
{
    sip_call_t *prev = cur;
    while ((prev = call_get_prev(prev))) {
        if (call_list_match_dfilter(panel, prev))
            return prev;
    }
    return NULL;
}

bool
call_list_match_dfilter(PANEL *panel, sip_call_t *call)
{
    char *upper;
    // Get panel information
    call_list_info_t *info = (call_list_info_t*) panel_userptr(panel);

    // Get Display filter
    char dfilter[128], linetext[512];

    // We trim spaces with sscanf because and empty field is stored as space characters
    memset(dfilter, 0, sizeof(dfilter));
    sscanf(field_buffer(info->fields[FLD_LIST_FILTER], 0), "%[^ ]", dfilter);

    if (strlen(dfilter) == 0)
        return true;

    memset(linetext, 0, sizeof(linetext));
    call_list_line_text(panel, call, linetext);

    // Upercase both strings
    for (upper = dfilter; *upper != '\0'; upper++)
        *upper = toupper((unsigned char) *upper);
    for (upper = linetext; *upper != '\0'; upper++)
        *upper = toupper((unsigned char) *upper);

    // Check if line contains the filter text
    return strstr(linetext, dfilter) != NULL;
}
