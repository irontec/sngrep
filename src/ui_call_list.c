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
#include <regex.h>
#include <ctype.h>
#include "option.h"
#include "filter.h"
#include "capture.h"
#include "ui_call_list.h"
#include "ui_call_flow.h"
#include "ui_call_raw.h"
#include "ui_save_pcap.h"

/**
 * Ui Structure definition for Call List panel
 */
ui_t ui_call_list = {
    .type = PANEL_CALL_LIST,
    .panel = NULL,
    .create = call_list_create,
    .draw = call_list_draw,
    .resize = call_list_resize,
    .handle_key = call_list_handle_key,
    .help = call_list_help,
    .destroy = call_list_destroy,

};

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
            title = sip_attr_get_title(attrid);
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
    call_list_form_activate(panel, 0);

    // Calculate available printable area
    info->list_win = subwin(win, height - 5, width, 4, 0);
    info->group = call_group_create();

    // Set defualt filter text if configured
    if (get_option_value("cl.filter")) {
        set_field_buffer(info->fields[FLD_LIST_FILTER], 0, get_option_value("cl.filter"));
        call_list_form_activate(panel, 0);
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

int
call_list_resize(PANEL *panel)
{
    int maxx, maxy;

    // Get panel info
    call_list_info_t *info = (call_list_info_t*) panel_userptr(panel);
    // Get current screen dimensions
    getmaxyx(stdscr, maxy, maxx);

    // Change the main window size
    wresize(panel_window(panel), maxy, maxx);
    // Calculate available printable area
    wresize(info->list_win, maxy - 5, maxx - 4);
    // Force list redraw
    call_list_clear(panel);

    return 0;
}

void
call_list_draw_header(PANEL *panel)
{
    const char *infile, *coldesc;
    int height, width, colpos, collen, i;

    // Get panel info
    call_list_info_t *info = (call_list_info_t*) panel_userptr(panel);

    // Let's draw the fixed elements of the screen
    WINDOW *win = panel_window(panel);
    getmaxyx(win, height, width);

    // Draw panel title
    draw_title(panel, "sngrep - SIP messages flow viewer");

    // Draw a Panel header lines
    clear_line(win, 1);
    if ((infile = capture_get_infile()))
        mvwprintw(win, 1, width - strlen(infile) - 11, "Filename: %s", infile);
    mvwprintw(win, 2, 2, "Display Filter: ");
    mvwprintw(win, 1, 2, "Current Mode: %s", capture_get_status_desc());

    // Reverse colors on monochrome terminals
    if (!has_colors())
        wattron(win, A_REVERSE);

    // Draw columns titles
    wattron(win, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));
    mvwprintw(win, 3, 0, "%*s", width, "");
    for (colpos = 6, i = 0; i < info->columncnt; i++) {
        // Get current column width
        collen = info->columns[i].width;
        // Get current column title
        coldesc = sip_attr_get_title(info->columns[i].id);

        // Check if the column will fit in the remaining space of the screen
        if (colpos + strlen(coldesc) >= width)
            break;
        mvwprintw(win, 3, colpos, "%.*s", collen, coldesc);
        colpos += collen + 1;
    }
    wattroff(win, A_BOLD | A_REVERSE | COLOR_PAIR(CP_DEF_ON_CYAN));

    // Get filter call counters
    filter_stats(&info->callcnt, &info->dispcallcnt);

    // Print calls count (also filtered)
    mvwprintw(win, 1, 35, "%*s", 35, "");
    if (info->callcnt != info->dispcallcnt) {
        mvwprintw(win, 1, 35, "Dialogs: %d (%d displayed)", info->callcnt, info->dispcallcnt);
    } else {
        mvwprintw(win, 1, 35, "Dialogs: %d", info->callcnt);
    }
}

void
call_list_draw_footer(PANEL *panel)
{
    const char *keybindings[] = {
        key_action_key_str(ACTION_PREV_SCREEN), "Quit",
        key_action_key_str(ACTION_SHOW_FLOW), "Show",
        key_action_key_str(ACTION_SELECT), "Select",
        key_action_key_str(ACTION_SHOW_HELP), "Help",
        key_action_key_str(ACTION_SAVE), "Save",
        key_action_key_str(ACTION_DISP_FILTER), "Search",
        key_action_key_str(ACTION_SHOW_FLOW_EX), "Extended",
        key_action_key_str(ACTION_CLEAR_CALLS), "Clear",
        key_action_key_str(ACTION_SHOW_RAW), "Raw",
        key_action_key_str(ACTION_SHOW_FILTERS), "Filter",
        key_action_key_str(ACTION_SHOW_COLUMNS), "Columns"
    };

    draw_keybindings(panel, keybindings, 22);
}

void
call_list_draw_list(PANEL *panel)
{

    int height, width, cline = 0;
    struct sip_call *call;
    char linetext[256];
    WINDOW *win;

    // Get panel info
    call_list_info_t *info = (call_list_info_t*) panel_userptr(panel);

    // No calls, we've finished drawing the list
    if (info->dispcallcnt == 0)
        return;

    // Get window of call list panel
    win = info->list_win;
    getmaxyx(win, height, width);

    // If no active call, use the fist one (if exists)
    if (!info->first_call && call_get_next_filtered(NULL)) {
        info->cur_call = info->first_call = call_get_next_filtered(NULL);
        info->cur_line = info->first_line = 1;
    }

    // Fill the call list
    for (call = info->first_call; call; call = call_get_next_filtered(call)) {
        // Stop if we have reached the bottom of the list
        if (cline == height)
            break;

        // We only print calls with messages (In fact, all call should have msgs)
        if (!call_msg_count(call))
            continue;

        // Show bold selected rows
        if (call_group_exists(info->group, call))
            wattron(win, A_BOLD | COLOR_PAIR(CP_DEFAULT));

        // Highlight active call
        if (call == info->cur_call) {
            // Reverse colors on monochrome terminals
            if (!has_colors())
                wattron(win, A_REVERSE);
            wattron(win, COLOR_PAIR(CP_DEF_ON_BLUE));
        }

        // Set current line background
        clear_line(win, cline);
        // Set current line selection box
        mvwprintw(win, cline, 2, call_group_exists(info->group, call) ? "[*]" : "[ ]");

        // Print call line
        memset(linetext, 0, sizeof(linetext));
        mvwprintw(win, cline, 6, "%-*s", width - 6, call_list_line_text(panel, call, linetext));
        cline++;

        wattroff(win, COLOR_PAIR(CP_DEFAULT));
        wattroff(win, COLOR_PAIR(CP_DEF_ON_BLUE));
        wattroff(win, A_BOLD | A_REVERSE);
    }

    // Draw scrollbar to the right
    draw_vscrollbar(win, info->first_line, info->dispcallcnt, 1);
    wnoutrefresh(info->list_win);
}

int
call_list_draw(PANEL *panel)
{
    int cury, curx;

    // Store cursor position
    getyx(panel_window(panel), cury, curx);

    // Draw the header
    call_list_draw_header(panel);
    // Draw the footer
    call_list_draw_footer(panel);
    // Draw the list content
    call_list_draw_list(panel);

    // Restore cursor position
    wmove(panel_window(panel), cury, curx);

    return 0;
}

void
call_list_form_activate(PANEL *panel, int active)
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

        // Get current column id
        colid = info->columns[i].id;

        // Get current column width
        collen = info->columns[i].width;

        // Check if next column fits on window width
        if (strlen(text) + collen >= width)
            collen = width - strlen(text);

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
    int action = -1;

    // Sanity check, this should not happen
    if (!info)
        return -1;

    // Handle form key
    if (info->form_active)
        return call_list_handle_form_key(panel, key);

    // Get window of call list panel
    WINDOW *win = info->list_win;
    getmaxyx(win, height, width);

    // Check actions for this key
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch (action) {
            case ACTION_DOWN:
                // Check if there is a call below us
                if (!info->cur_call || !call_get_next_filtered(info->cur_call))
                    break;
                info->cur_call = call_get_next_filtered(info->cur_call);
                info->cur_line++;
                // If we are out of the bottom of the displayed list
                // refresh it starting in the next call
                if (info->cur_line > height) {
                    info->first_call = call_get_next_filtered(info->first_call);
                    info->first_line++;
                    info->cur_line = height;
                }
                break;
            case ACTION_UP:
                // Check if there is a call above us
                if (!info->cur_call || !call_get_prev_filtered(info->cur_call))
                    break;
                info->cur_call = call_get_prev_filtered(info->cur_call);
                info->cur_line--;
                // If we are out of the top of the displayed list
                // refresh it starting in the previous (in fact current) call
                if (info->cur_line <= 0) {
                    info->first_call = info->cur_call;
                    info->first_line--;
                    info->cur_line = 1;
                }
                break;
            case ACTION_HNPAGE:
                rnpag_steps = rnpag_steps / 2;
                /* no break */
            case ACTION_NPAGE:
                // Next page => N key down strokes
                for (i = 0; i < rnpag_steps; i++)
                    call_list_handle_key(panel, KEY_DOWN);
                break;
            case ACTION_HPPAGE:
                rnpag_steps = rnpag_steps / 2;
                /* no break */
            case ACTION_PPAGE:
                // Prev page => N key up strokes
                for (i = 0; i < rnpag_steps; i++)
                    call_list_handle_key(panel, KEY_UP);
                break;
            case ACTION_DISP_FILTER:
                // Activate Form
                call_list_form_activate(panel, 1);
                break;
            case ACTION_SHOW_FLOW:
                // Display current call flow
                if (!info->cur_call)
                    break;
                next_panel = ui_create(ui_find_by_type(PANEL_CALL_FLOW));
                if (info->group->callcnt) {
                    group = info->group;
                } else {
                    if (!info->cur_call)
                        break;
                    group = call_group_create();
                    call_group_add(group, info->cur_call);
                }
                call_flow_set_group(group);
                wait_for_input(next_panel);
                break;
            case ACTION_SHOW_FLOW_EX:
                // Display current call flow (extended)
                next_panel = ui_create(ui_find_by_type(PANEL_CALL_FLOW));
                if (info->group->callcnt) {
                    group = info->group;
                } else {
                    if (!info->cur_call)
                        break;
                    group = call_group_create();
                    call_group_add(group, info->cur_call);
                    call_group_add(group, call_get_xcall(info->cur_call));
                }
                call_flow_set_group(group);
                wait_for_input(next_panel);
                break;
            case ACTION_SHOW_RAW:
                // KEY_R , Display current call flow (extended)
                next_panel = ui_create(ui_find_by_type(PANEL_CALL_RAW));
                if (info->group->callcnt) {
                    group = info->group;
                } else {
                    if (!info->cur_call)
                        break;
                    group = call_group_create();
                    call_group_add(group, info->cur_call);
                }
                call_raw_set_group(group);
                wait_for_input(next_panel);
                break;
            case ACTION_SHOW_FILTERS:
                // KEY_F, Display filter panel
                next_panel = ui_create(ui_find_by_type(PANEL_FILTER));
                wait_for_input(next_panel);
                call_list_clear(panel);
                break;
            case ACTION_SHOW_COLUMNS:
                // Display column selection panel
                next_panel = ui_create(ui_find_by_type(PANEL_COLUMN_SELECT));
                wait_for_input(next_panel);
                call_list_clear(panel);
                break;
            case ACTION_SAVE:
                if (!is_option_disabled("sngrep.tmpfile")) {
                    // KEY_S, Display save panel
                    next_panel = ui_create(ui_find_by_type(PANEL_SAVE));
                    save_set_group(ui_get_panel(next_panel), info->group);
                    wait_for_input(next_panel);
                }
                break;
            case ACTION_DISP_INVITE:
                // Set Display filter text
                set_field_buffer(info->fields[FLD_LIST_FILTER], 0, "invite");
                filter_set(FILTER_CALL_LIST, "invite");
                call_list_clear(panel);
                filter_reset_calls();
                break;
            case ACTION_CLEAR_CALLS:
                // Remove all stored calls
                sip_calls_clear();
                // Clear List
                call_list_clear(panel);
                break;
            case ACTION_SELECT:
                if (!info->cur_call)
                    break;
                if (call_group_exists(info->group, info->cur_call)) {
                    call_group_del(info->group, info->cur_call);
                } else {
                    call_group_add(info->group, info->cur_call);
                }
                break;
            case ACTION_PREV_SCREEN:
                // Handle quit from this screen unless requested
                if (!is_option_enabled("cl.noexitprompt")) {
                    return call_list_exit_confirm(panel);
                }
                break;
            default:
                // Parse next action
                continue;
        }

        // This panel has handled the key successfully
        break;
    }

    // Return if this panel has handled or not the key
    return (action == ERR) ? key : 0;
}

int
call_list_handle_form_key(PANEL *panel, int key)
{
    int field_idx;
    char dfilter[256];
    int action = -1;

    // Get panel information
    call_list_info_t *info = (call_list_info_t*) panel_userptr(panel);

    // Get current field id
    field_idx = field_index(current_field(info->form));

    // Check actions for this key
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch (action) {
            case ACTION_PRINTABLE:
                // If this is a normal character on input field, print it
                form_driver(info->form, key);
                // Updated displayed results
                call_list_clear(panel);
                // Reset filters on each key stroke
                filter_reset_calls();
                break;
            case ACTION_PREV_SCREEN:
            case ACTION_NEXT_FIELD:
            case ACTION_CONFIRM:
            case ACTION_SELECT:
            case ACTION_UP:
            case ACTION_DOWN:
                // Activate list
                call_list_form_activate(panel, 0);
                break;
            case ACTION_RIGHT:
                form_driver(info->form, REQ_RIGHT_CHAR);
                break;
            case ACTION_LEFT:
                form_driver(info->form, REQ_LEFT_CHAR);
                break;
            case ACTION_BEGIN:
                form_driver(info->form, REQ_BEG_LINE);
                break;
            case ACTION_END:
                form_driver(info->form, REQ_END_LINE);
                break;
            case ACTION_CLEAR:
                form_driver(info->form, REQ_BEG_LINE);
                form_driver(info->form, REQ_CLR_EOL);
                break;
            case ACTION_DELETE:
                form_driver(info->form, REQ_DEL_CHAR);
                break;
            case ACTION_BACKSPACE:
                form_driver(info->form, REQ_DEL_PREV);
                // Updated displayed results
                call_list_clear(panel);
                // Reset filters on each key stroke
                filter_reset_calls();
                break;
            default:
                // Parse next action
                continue;
        }

        // We've handled this key, stop checking actions
        break;
    }

    // Validate all input data
    form_driver(info->form, REQ_VALIDATION);

    // Store dfilter input
    // We trim spaces with sscanf because and empty field is stored as space characters
    memset(dfilter, 0, sizeof(dfilter));
    sscanf(field_buffer(info->fields[FLD_LIST_FILTER], 0), "%[^ ]", dfilter);

    // Set display filter
    filter_set(FILTER_CALL_LIST, strlen(dfilter) ? dfilter : NULL);

    // Return if this panel has handled or not the key
    return (action == ERR) ? key : 0;
}

int
call_list_help(PANEL *panel)
{
    WINDOW *help_win;
    PANEL *help_panel;
    int height, width;

    // Create a new panel and show centered
    height = 28;
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
    mvwprintw(help_win, 24, 2, "p           Stop/Resume packet capture");

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
