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
#ifdef USE_EEP
#include "capture_eep.h"
#endif
#include "ui_manager.h"
#include "ui_call_list.h"
#include "ui_call_flow.h"
#include "ui_call_raw.h"
#include "ui_filter.h"
#include "ui_save.h"
#include "sip.h"

/**
 * Ui Structure definition for Call List panel
 */
ui_t ui_call_list = {
    .type = PANEL_CALL_LIST,
    .create = call_list_create,
    .destroy = call_list_destroy,
    .redraw = call_list_redraw,
    .draw = call_list_draw,
    .resize = call_list_resize,
    .handle_key = call_list_handle_key,
    .help = call_list_help,
};

void
call_list_create(ui_t *ui)
{
    int i, attrid, collen;
    call_list_info_t *info;
    char option[80];
    const char *field, *title;

    // Create a new panel that fill all the screen
    ui_panel_create(ui, LINES, COLS);

    // Initialize Call List specific data
    info = malloc(sizeof(call_list_info_t));
    memset(info, 0, sizeof(call_list_info_t));
    set_panel_userptr(ui->panel, (void*) info);

    // Add configured columns
    for (i = 0; i < SIP_ATTR_COUNT; i++) {
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
            call_list_add_column(ui, attrid, field, title, collen);
        }
    }

    // Initialize the fields
    info->fields[FLD_LIST_FILTER] = new_field(1, ui->width - 19, 3, 18, 0, 0);
    info->fields[FLD_LIST_COUNT] = NULL;

    // Create the form and post it
    info->form = new_form(info->fields);
    set_form_sub(info->form, ui->win);

    // Form starts inactive
    call_list_form_activate(ui, 0);
    info->menu_active = 0;

    // Calculate available printable area
    info->list_win = subwin(ui->win, ui->height - 6, ui->width, 5, 0);
    info->scroll = ui_set_scrollbar(info->list_win, SB_VERTICAL, SB_LEFT);

    // Group of selected calls
    info->group = call_group_create();

    // Get current call list
    info->cur_call = -1;

    // Set autoscroll default status
    info->autoscroll = setting_enabled(SETTING_CL_AUTOSCROLL);

    // Apply initial configured filters
    filter_method_from_setting(setting_get_value(SETTING_FILTER_METHODS));
    filter_payload_from_setting(setting_get_value(SETTING_FILTER_PAYLOAD));
}

void
call_list_destroy(ui_t *ui)
{
    call_list_info_t *info;

    // Free its status data
    if ((info = call_list_info(ui))) {
        // Deallocate forms data
        if (info->form) {
            unpost_form(info->form);
            free_form(info->form);
            free_field(info->fields[FLD_LIST_FILTER]);
        }

        // Deallocate group data
        call_group_destroy(info->group);
        vector_destroy(info->dcalls);

        // Deallocate panel windows
        delwin(info->list_win);
        sng_free(info);
    }

    ui_panel_destroy(ui);
}

call_list_info_t *
call_list_info(ui_t *ui)
{
    return (call_list_info_t*) panel_userptr(ui->panel);
}

bool
call_list_redraw(ui_t *ui)
{
    return sip_calls_has_changed();
}

int
call_list_resize(ui_t *ui)
{
    int maxx, maxy;

    // Get panel info
    call_list_info_t *info = call_list_info(ui);
    // Get current screen dimensions
    getmaxyx(stdscr, maxy, maxx);

    // Change the main window size
    wresize(ui->win, maxy, maxx);

    // Store new size
    ui->width = maxx;
    ui->height = maxy;

    // Calculate available printable area
    wresize(info->list_win, maxy - 6, maxx); //-4
    // Force list redraw
    call_list_clear(ui);

    return 0;
}

void
call_list_draw_header(ui_t *ui)
{
    const char *infile, *coldesc;
    int colpos, collen, i;
    char sortind;
    const char *countlb;
    const char *device, *filterexpr, *filterbpf;

    // Get panel info
    call_list_info_t *info = call_list_info(ui);

    // Draw panel title
    ui_set_title(ui, "sngrep - SIP messages flow viewer");

    // Draw a Panel header lines
    ui_clear_line(ui, 1);

    // Print Open filename in Offline mode
    if (!capture_is_online() && (infile = capture_input_file()))
        mvwprintw(ui->win, 1, 77, "Filename: %s", infile);

    mvwprintw(ui->win, 1, 2, "Current Mode: ");
    if (capture_is_online()) {
        wattron(ui->win, COLOR_PAIR(CP_GREEN_ON_DEF));
    } else {
        wattron(ui->win, COLOR_PAIR(CP_RED_ON_DEF));
    }
    wprintw(ui->win, "%s ", capture_status_desc());

    // Get online mode capture device
    if ((device = capture_device()))
        wprintw(ui->win, "[%s]", device);

#ifdef USE_EEP
    const char *eep_port;
    if ((eep_port = capture_eep_send_port())) {
        wprintw(ui->win, "[H:%s]", eep_port);
    }
    if ((eep_port = capture_eep_listen_port())) {
        wprintw(ui->win, "[L:%s]", eep_port);
    }
#endif

    wattroff(ui->win, COLOR_PAIR(CP_GREEN_ON_DEF));
    wattroff(ui->win, COLOR_PAIR(CP_RED_ON_DEF));

    // Label for Display filter
    mvwprintw(ui->win, 3, 2, "Display Filter: ");

    mvwprintw(ui->win, 2, 2, "Match Expression: ");

    wattron(ui->win, COLOR_PAIR(CP_YELLOW_ON_DEF));
    if ((filterexpr = sip_get_match_expression()))
        wprintw(ui->win, "%s", filterexpr);
    wattroff(ui->win, COLOR_PAIR(CP_YELLOW_ON_DEF));

    mvwprintw(ui->win, 2, 45, "BPF Filter: ");
    wattron(ui->win, COLOR_PAIR(CP_YELLOW_ON_DEF));
    if ((filterbpf = capture_get_bpf_filter()))
        wprintw(ui->win, "%s", filterbpf);
    wattroff(ui->win, COLOR_PAIR(CP_YELLOW_ON_DEF));

    // Reverse colors on monochrome terminals
    if (!has_colors())
        wattron(ui->win, A_REVERSE);

    // Get configured sorting options
    sip_sort_t sort = sip_sort_options();

    // Draw columns titles
    wattron(ui->win, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));
    ui_clear_line(ui, 4);

    // Draw separator line
    if (info->menu_active) {
        colpos = 18;
        mvwprintw(ui->win, 4, 0, "Sort by");
        wattron(ui->win, A_BOLD | COLOR_PAIR(CP_CYAN_ON_BLACK));
        mvwprintw(ui->win, 4, 11, "%*s", 1, "");
        wattron(ui->win, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));
    } else {
        colpos = 6;
    }

    for (i = 0; i < info->columncnt; i++) {
        // Get current column width
        collen = info->columns[i].width;
        // Get current column title
        coldesc = sip_attr_get_title(info->columns[i].id);

        // Check if the column will fit in the remaining space of the screen
        if (colpos + strlen(coldesc) >= ui->width)
            break;

        // Print sort column indicator
        if (info->columns[i].id == sort.by) {
            wattron(ui->win, A_BOLD | COLOR_PAIR(CP_YELLOW_ON_CYAN));
            sortind = (sort.asc) ? '^' : 'v';
            mvwprintw(ui->win, 4, colpos, "%c%.*s", sortind, collen, coldesc);
            wattron(ui->win, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));
        } else {
            mvwprintw(ui->win, 4, colpos, "%.*s", collen, coldesc);
        }
        colpos += collen + 1;
    }
    // Print Autoscroll indicator
    if (info->autoscroll)
        mvwprintw(ui->win, 4, 0, "A");
    wattroff(ui->win, A_BOLD | A_REVERSE | COLOR_PAIR(CP_DEF_ON_CYAN));

    // Print Dialogs or Calls in label depending on calls filter
    if (setting_enabled(SETTING_SIP_CALLS)) {
        countlb = "Calls";
    } else {
        countlb = "Dialogs";
    }

    // Print calls count (also filtered)
    sip_stats_t stats = sip_calls_stats();
    mvwprintw(ui->win, 1, 45, "%*s", 30, "");
    if (stats.total != stats.displayed) {
        mvwprintw(ui->win, 1, 45, "%s: %d (%d displayed)", countlb, stats.total, stats.displayed);
    } else {
        mvwprintw(ui->win, 1, 45, "%s: %d", countlb, stats.total);
    }

}

void
call_list_draw_footer(ui_t *ui)
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
        key_action_key_str(ACTION_SHOW_FILTERS), "Filter",
        key_action_key_str(ACTION_SHOW_SETTINGS), "Settings",
        key_action_key_str(ACTION_CLEAR_CALLS_SOFT), "Clear with Filter",
        key_action_key_str(ACTION_SHOW_COLUMNS), "Columns"
    };

    ui_draw_bindings(ui, keybindings, 23);
}

void
call_list_draw_list(ui_t *ui)
{
    WINDOW *list_win;
    int listh, listw, cline = 0;
    struct sip_call *call = NULL;
    int i, collen;
    char coltext[SIP_ATTR_MAXLEN];
    int colid;
    int colpos;
    int color;

    // Get panel info
    call_list_info_t *info = call_list_info(ui);

    // Get window of call list panel
    list_win = info->list_win;
    getmaxyx(list_win, listh, listw);

    // Store selected call
    if (info->cur_call >= 0)
        call = vector_item(info->dcalls, info->cur_call);

    // Get the list of calls that are goint to be displayed
    vector_destroy(info->dcalls);
    info->dcalls = vector_copy_if(sip_calls_vector(), filter_check_call);

    // If no active call, use the fist one (if exists)
    if (info->cur_call == -1 && vector_count(info->dcalls)) {
        info->cur_call = info->scroll.pos = 0;
    }

    // If autoscroll is enabled, select the last dialog
    if (info->autoscroll)  {
        sip_sort_t sort = sip_sort_options();
        if (sort.asc) {
            call_list_move(ui, vector_count(info->dcalls) - 1);
        } else {
            call_list_move(ui, 0);
        }
    } else if (call) {
        call_list_move(ui, vector_index(info->dcalls, call));
    }

    // Clear call list before redrawing
    werase(list_win);

    // Set the iterator position to the first call
    vector_iter_t it = vector_iterator(info->dcalls);
    vector_iterator_set_current(&it, info->scroll.pos - 1);

    // Fill the call list
    while ((call = vector_iterator_next(&it))) {
        // Stop if we have reached the bottom of the list
        if (cline == listh)
            break;

        // We only print calls with messages (In fact, all call should have msgs)
        if (!call_msg_count(call))
            continue;

        // Show bold selected rows
        if (call_group_exists(info->group, call))
            wattron(list_win, A_BOLD | COLOR_PAIR(CP_DEFAULT));

        // Highlight active call
        if (info->cur_call == vector_iterator_current(&it)) {
            wattron(list_win, COLOR_PAIR(CP_WHITE_ON_BLUE));
            // Reverse colors on monochrome terminals
            if (!has_colors())
                wattron(list_win, A_REVERSE);
        }
        // Set current line background
        mvwprintw(list_win, cline, 0, "%*s", listw, "");
        // Set current line selection box
        mvwprintw(list_win, cline, 2, call_group_exists(info->group, call) ? "[*]" : "[ ]");

        // Print requested columns
        colpos = 6;
        for (i = 0; i < info->columncnt; i++) {
            // Get current column id
            colid = info->columns[i].id;
            // Get current column width
            collen = info->columns[i].width;
            // Check if next column fits on window width
            if (colpos + collen >= listw)
                break;

            // Initialize column text
            memset(coltext, 0, sizeof(coltext));

            // Get call attribute for current column
            if (!call_get_attribute(call, colid, coltext)) {
                colpos += collen + 1;
                continue;
            }

            // Enable attribute color (if not current one)
            color = 0;
            if (info->cur_call != vector_iterator_current(&it)) {
                if ((color = sip_attr_get_color(colid, coltext)) > 0) {
                    wattron(list_win, color);
                }
            }

            // Add the column text to the existing columns
            mvwprintw(list_win, cline, colpos, "%.*s", collen, coltext);
            colpos += collen + 1;

            // Disable attribute color
            if (color > 0)
                wattroff(list_win, color);
        }
        cline++;

        wattroff(list_win, COLOR_PAIR(CP_DEFAULT));
        wattroff(list_win, COLOR_PAIR(CP_DEF_ON_BLUE));
        wattroff(list_win, A_BOLD | A_REVERSE);
    }

    // Draw scrollbar to the right
    info->scroll.max = vector_count(info->dcalls);
    ui_scrollbar_draw(info->scroll);

    // Refresh the list
    if (!info->menu_active)
        wnoutrefresh(info->list_win);
}

int
call_list_draw(ui_t *ui)
{
    int cury, curx;

    // Store cursor position
    getyx(ui->win, cury, curx);

    // Draw the header
    call_list_draw_header(ui);
    // Draw the footer
    call_list_draw_footer(ui);
    // Draw the list content
    call_list_draw_list(ui);

    // Restore cursor position
    wmove(ui->win, cury, curx);

    return 0;
}

void
call_list_form_activate(ui_t *ui, int active)
{
    call_list_info_t *info = call_list_info(ui);

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
call_list_line_text(ui_t *ui, sip_call_t *call, char *text)
{
    int i, collen;
    char call_attr[SIP_ATTR_MAXLEN];
    char coltext[SIP_ATTR_MAXLEN];
    int colid;

    // Get panel info
    call_list_info_t *info = call_list_info(ui);

    // Print requested columns
    for (i = 0; i < info->columncnt; i++) {

        // Get current column id
        colid = info->columns[i].id;

        // Get current column width
        collen = info->columns[i].width;

        // Check if next column fits on window width
        if (strlen(text) + collen >= ui->width)
            collen = ui->width - strlen(text);

        // If no space left on the screen stop processing columns
        if (collen <= 0)
            break;

        // Initialize column text
        memset(coltext, 0, sizeof(coltext));
        memset(call_attr, 0, sizeof(call_attr));

        // Get call attribute for current column
        if (call_get_attribute(call, colid, call_attr)) {
            sprintf(coltext, "%.*s", collen, call_attr);
        }
        // Add the column text to the existing columns
        sprintf(text + strlen(text), "%-*s ", collen, coltext);
    }

    return text;
}

int
call_list_handle_key(ui_t *ui, int key)
{
    int listh, listw,rnpag_steps = setting_get_intvalue(SETTING_CL_SCROLLSTEP);
    call_list_info_t *info;
    ui_t *next_ui;
    sip_call_group_t *group;
    int action = -1;
    sip_call_t *call, *xcall;
    sip_sort_t sort;

    // Sanity check, this should not happen
    if (!(info  = call_list_info(ui)))
        return -1;

    // Handle form key
    if (info->form_active)
        return call_list_handle_form_key(ui, key);

    if (info->menu_active)
        return call_list_handle_menu_key(ui, key);

    // Get window of call list panel
    WINDOW *list_win = info->list_win;
    getmaxyx(list_win, listh, listw);

    // Check actions for this key
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch (action) {
            case ACTION_DOWN:
                call_list_move(ui, info->cur_call + 1);
                break;
            case ACTION_UP:
                call_list_move(ui, info->cur_call - 1);
                break;
            case ACTION_HNPAGE:
                rnpag_steps = rnpag_steps / 2;
                /* no break */
            case ACTION_NPAGE:
                // Next page => N key down strokes
                call_list_move(ui, info->cur_call + rnpag_steps);
                break;
            case ACTION_HPPAGE:
                rnpag_steps = rnpag_steps / 2;
                /* no break */
            case ACTION_PPAGE:
                // Prev page => N key up strokes
                call_list_move(ui, info->cur_call - rnpag_steps);
                break;
            case ACTION_BEGIN:
                // Move to first list entry
                call_list_move(ui, 0);
                break;
            case ACTION_END:
                call_list_move(ui, vector_count(info->dcalls));
                break;
            case ACTION_DISP_FILTER:
                // Activate Form
                call_list_form_activate(ui, 1);
                break;
            case ACTION_SHOW_FLOW:
            case ACTION_SHOW_FLOW_EX:
            case ACTION_SHOW_RAW:
                // Check we have calls in the list
                if (info->cur_call == -1)
                    break;
                // Create a new group of calls
                group = call_group_clone(info->group);

                // If not selected call, show current call flow
                if (call_group_count(info->group) == 0)
                    call_group_add(group, vector_item(info->dcalls, info->cur_call));

                // Add xcall to the group
                if (action == ACTION_SHOW_FLOW_EX) {
                    call = vector_item(info->dcalls, info->cur_call);
                    if (call->xcallid != NULL && strlen(call->xcallid)) {
                        if ((xcall = sip_find_by_callid(call->xcallid))) {
                            call_group_del(group, call);
                            call_group_add(group, xcall);
                            call_group_add_calls(group, xcall->xcalls);
                            group->callid = xcall->callid;
                        }
                    } else {
                        call_group_add_calls(group, call->xcalls);
                        group->callid = call->callid;
                    }
                }

                if (action == ACTION_SHOW_RAW) {
                    // Create a Call Flow panel
                    ui_create_panel(PANEL_CALL_RAW);
                    call_raw_set_group(group);
                } else {
                    // Display current call flow (normal or extended)
                    ui_create_panel(PANEL_CALL_FLOW);
                    call_flow_set_group(group);
                }
                break;
            case ACTION_SHOW_FILTERS:
                ui_create_panel(PANEL_FILTER);
                break;
            case ACTION_SHOW_COLUMNS:
                ui_create_panel(PANEL_COLUMN_SELECT);
                break;
            case ACTION_SHOW_STATS:
                ui_create_panel(PANEL_STATS);
                break;
            case ACTION_SAVE:
                if (capture_sources_count() > 1) {
                    dialog_run("Saving is not possible when multiple input sources are specified.");
                    break;
                }
                next_ui = ui_create_panel(PANEL_SAVE);
                save_set_group(next_ui, info->group);
                break;
            case ACTION_CLEAR:
                // Clear group calls
                vector_clear(info->group->calls);
                break;
            case ACTION_CLEAR_CALLS:
                // Remove all stored calls
                sip_calls_clear();
                // Clear List
                call_list_clear(ui);
                break;
            case ACTION_CLEAR_CALLS_SOFT:
                // Remove stored calls, keeping the currently displayed calls
                sip_calls_clear_soft();
                // Clear List
                call_list_clear(ui);
                break;
            case ACTION_AUTOSCROLL:
                info->autoscroll = (info->autoscroll) ? 0 : 1;
                break;
            case ACTION_SHOW_SETTINGS:
                ui_create_panel(PANEL_SETTINGS);
                break;
            case ACTION_SELECT:
                call = vector_item(info->dcalls, info->cur_call);
                if (call_group_exists(info->group, call)) {
                    call_group_del(info->group, call);
                } else {
                    call_group_add(info->group, call);
                }
                break;
            case ACTION_SORT_SWAP:
                // Change sort order
                sort = sip_sort_options();
                sort.asc = (sort.asc) ? false : true;
                sip_set_sort_options(sort);
                break;
            case ACTION_SORT_NEXT:
            case ACTION_SORT_PREV:
                call_list_select_sort_attribute(ui);
                break;
            case ACTION_PREV_SCREEN:
                // Handle quit from this screen unless requested
                if (setting_enabled(SETTING_EXITPROMPT)) {
                    if (dialog_confirm("Confirm exit", "Are you sure you want to quit?", "Yes,No") == 0) {
                        ui_destroy(ui);
                    }
                } else {
                    ui_destroy(ui);
                }
                return KEY_HANDLED;
                break;
            default:
                // Parse next action
                continue;
        }

        // This panel has handled the key successfully
        break;
    }

    // Disable autoscroll on some key pressed
    switch(action) {
        case ACTION_DOWN:
        case ACTION_UP:
        case ACTION_HNPAGE:
        case ACTION_HPPAGE:
        case ACTION_NPAGE:
        case ACTION_PPAGE:
        case ACTION_BEGIN:
        case ACTION_END:
        case ACTION_DISP_FILTER:
            info->autoscroll = 0;
            break;
    }


    // Return if this panel has handled or not the key
    return (action == ERR) ? KEY_NOT_HANDLED : KEY_HANDLED;
}

int
call_list_handle_form_key(ui_t *ui, int key)
{
    char *dfilter;
    int action = -1;

    // Get panel information
    call_list_info_t *info = call_list_info(ui);

    // Check actions for this key
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch (action) {
            case ACTION_PRINTABLE:
                // If this is a normal character on input field, print it
                form_driver(info->form, key);
                break;
            case ACTION_PREV_SCREEN:
            case ACTION_NEXT_FIELD:
            case ACTION_CONFIRM:
            case ACTION_SELECT:
            case ACTION_UP:
            case ACTION_DOWN:
                // Activate list
                call_list_form_activate(ui, 0);
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
                break;
            default:
                // Parse next action
                continue;
        }

        // We've handled this key, stop checking actions
        break;
    }

    // Filter has changed, re-apply filter to displayed calls
    if (action == ACTION_PRINTABLE || action == ACTION_BACKSPACE ||
            action == ACTION_DELETE || action == ACTION_CLEAR) {
        // Updated displayed results
         call_list_clear(ui);
         // Reset filters on each key stroke
         filter_reset_calls();
    }

    // Validate all input data
    form_driver(info->form, REQ_VALIDATION);

    // Store dfilter input
    int field_len = strlen(field_buffer(info->fields[FLD_LIST_FILTER], 0));
    dfilter = malloc(field_len + 1);
    memset(dfilter, 0, field_len + 1);
    strncpy(dfilter, field_buffer(info->fields[FLD_LIST_FILTER], 0), field_len);
    // Trim any trailing spaces
    strtrim(dfilter);

    // Set display filter
    filter_set(FILTER_CALL_LIST, strlen(dfilter) ? dfilter : NULL);
    free(dfilter);

    // Return if this panel has handled or not the key
    return (action == ERR) ? KEY_NOT_HANDLED : KEY_HANDLED;
}

int
call_list_handle_menu_key(ui_t *ui, int key)
{
    MENU *menu;
    int i;
    int action = -1;
    sip_sort_t sort;
    enum sip_attr_id id;

    // Get panel information
    call_list_info_t *info = call_list_info(ui);

    menu = info->menu;

      // Check actions for this key
      while ((action = key_find_action(key, action)) != ERR) {
          // Check if we handle this action
          switch (action) {
              case ACTION_DOWN:
                  menu_driver(menu, REQ_DOWN_ITEM);
                  break;
              case ACTION_UP:
                  menu_driver(menu, REQ_UP_ITEM);
                  break;
              case ACTION_NPAGE:
                  menu_driver(menu, REQ_SCR_DPAGE);
                  break;
              case ACTION_PPAGE:
                  menu_driver(menu, REQ_SCR_UPAGE);
                  break;
              case ACTION_CONFIRM:
              case ACTION_SELECT:
                  // Change sort attribute
                  sort = sip_sort_options();
                  id = sip_attr_from_name(item_name(current_item(info->menu)));
                  if (sort.by == id) {
                      sort.asc = (sort.asc) ? false : true;
                  } else {
                      sort.by = id;
                  }
                  sip_set_sort_options(sort);
                  /* no break */
              case ACTION_PREV_SCREEN:
                  // Desactive sorting menu
                  info->menu_active = 0;

                  // Remove menu
                  unpost_menu(info->menu);
                  free_menu(info->menu);

                  // Remove items
                  for (i = 0; i < SIP_ATTR_COUNT; i++) {
                      if (!info->items[i])
                          break;
                      free_item(info->items[i]);
                  }

                  // Restore list position
                  mvderwin(info->list_win, 5, 0);
                  // Restore list window size
                  wresize(info->list_win, ui->height - 6, ui->width);
                  break;
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
call_list_help(ui_t *ui)
{
    WINDOW *help_win;
    int height, width;

    // Create a new panel and show centered
    height = 28;
    width = 65;
    help_win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);

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
    mvwprintw(help_win, 17, 2, "F5/Ctrl-L   Clear call list (can not be undone!)");
    mvwprintw(help_win, 18, 2, "F6/R        Show selected call messages in raw mode");
    mvwprintw(help_win, 19, 2, "F7/F        Show filter options");
    mvwprintw(help_win, 20, 2, "F8/o        Show Settings");
    mvwprintw(help_win, 21, 2, "F10/t       Select displayed columns");
    mvwprintw(help_win, 22, 2, "i/I         Set display filter to invite");
    mvwprintw(help_win, 23, 2, "p           Stop/Resume packet capture");

    // Press any key to close
    wgetch(help_win);
    delwin(help_win);

    return 0;
}

int
call_list_add_column(ui_t *ui, enum sip_attr_id id, const char* attr,
                     const char *title, int width)
{
    call_list_info_t *info;

    if (!(info = call_list_info(ui)))
        return 1;

    info->columns[info->columncnt].id = id;
    info->columns[info->columncnt].attr = attr;
    info->columns[info->columncnt].title = title;
    info->columns[info->columncnt].width = width;
    info->columncnt++;
    return 0;
}

void
call_list_clear(ui_t *ui)
{
    call_list_info_t *info;

    // Get panel info
    if (!(info = call_list_info(ui)))
        return;

    // Initialize structures
    info->scroll.pos = info->cur_call = -1;
    vector_clear(info->group->calls);

    // Clear Displayed lines
    werase(info->list_win);
}

void
call_list_move(ui_t *ui, int line)
{
    call_list_info_t *info;

    // Get panel info
    if (!(info = call_list_info(ui)))
        return;

    // Already in this position?
    if (info->cur_call == line)
        return;

    // Moving down or up?
    bool move_down = (info->cur_call < line);

    vector_iter_t it = vector_iterator(info->dcalls);
    vector_iterator_set_current(&it, info->cur_call);

    if (move_down) {
        while (info->cur_call < line) {
            // Check if there is a call below us
            if (!vector_iterator_next(&it))
               break;

            // Increase current call position
            info->cur_call++;

            // If we are out of the bottom of the displayed list
            // refresh it starting in the next call
            if (info->cur_call - info->scroll.pos == getmaxy(info->list_win)) {
               info->scroll.pos++;
            }
        }
    } else {
        while (info->cur_call > line) {
            // Check if there is a call above us
            if (!vector_iterator_prev(&it))
              break;
            // If we are out of the top of the displayed list
            // refresh it starting in the previous (in fact current) call
            if (info->cur_call ==  info->scroll.pos) {
              info->scroll.pos--;
            }
            // Move current call position
            info->cur_call--;
        }
    }
}

void
call_list_select_sort_attribute(ui_t *ui)
{
    call_list_info_t *info;
    int i;

    // Get panel info
    if (!(info = call_list_info(ui)))
        return;

    // Activete sorting menu
    info->menu_active = 1;

    wresize(info->list_win, ui->height - 6, ui->width - 12);
    mvderwin(info->list_win, 5, 12);

    // Create menu entries
    for (i = 0; i < info->columncnt; i++) {
        info->items[i] = new_item(sip_attr_get_name(info->columns[i].id), 0);
    }
    info->items[info->columncnt] = NULL;
    // Create the columns menu and post it
    info->menu = new_menu(info->items);

    // Set main window and sub window
    set_menu_win(info->menu, ui->win);
    set_menu_sub(info->menu, derwin(ui->win, 20, 15, 5, 0));
    werase(menu_win(info->menu));
    set_menu_format(info->menu, ui->height, 1);
    set_menu_mark(info->menu, "");
    set_menu_fore(info->menu, COLOR_PAIR(CP_DEF_ON_BLUE));
    menu_opts_off(info->menu, O_ONEVALUE);
    post_menu(info->menu);
}
