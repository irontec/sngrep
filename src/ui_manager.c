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
 * @file ui_manager.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_manager.h
 *
 */
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "option.h"
#include "ui_manager.h"
#include "ui_call_list.h"
#include "ui_call_flow.h"
#include "ui_call_raw.h"
#include "ui_filter.h"
#include "ui_save_pcap.h"
#include "ui_save_raw.h"
#include "ui_msg_diff.h"
#include "ui_column_select.h"

/**
 * @brief Warranty thread-safe ui refresh
 *
 * This lock should be used whenever the ui is replaced or updated
 * preventing the ui swap during the screen refresh
 */
pthread_mutex_t refresh_lock;

/**
 * @brief Available panel windows list
 *
 * This list contein the available list of windows
 * and pointer to their main functions.
 *
 * XXX If the panel count increase a lot, it will be required to
 *     load panels as modules and provide a way to register
 *     themselfs into the panel pool dynamically.
 */
static ui_t panel_pool[] =
    {
          {
            .type = MAIN_PANEL,
            .panel = NULL,
            .create = call_list_create,
            .redraw_required = call_list_redraw_required,
            .draw = call_list_draw,
            .handle_key = call_list_handle_key,
            .help = call_list_help,
            .destroy = call_list_destroy, },
          {
            .type = DETAILS_PANEL,
            .panel = NULL,
            .create = call_flow_create,
            .redraw_required = call_flow_redraw_required,
            .draw = call_flow_draw,
            .handle_key = call_flow_handle_key,
            .help = call_flow_help },
          {
            .type = RAW_PANEL,
            .panel = NULL,
            .create = call_raw_create,
            .redraw_required = call_raw_redraw_required,
            .draw = call_raw_draw,
            .handle_key = call_raw_handle_key },
          {
            .type = FILTER_PANEL,
            .panel = NULL,
            .create = filter_create,
            .handle_key = filter_handle_key,
            .destroy = filter_destroy },
          {
            .type = SAVE_PANEL,
            .panel = NULL,
            .create = save_create,
            .draw = save_draw,
            .handle_key = save_handle_key,
            .destroy = save_destroy },
          {
            .type = SAVE_RAW_PANEL,
            .panel = NULL,
            .create = save_raw_create,
            .handle_key = save_raw_handle_key,
            .destroy = save_raw_destroy },
          {
            .type = MSG_DIFF_PANEL,
            .panel = NULL,
            .create = msg_diff_create,
            .handle_key = msg_diff_handle_key,
            .destroy = msg_diff_destroy,
            .draw = msg_diff_draw,
            .help = msg_diff_help },
          {
            .type = COLUMN_SELECT_PANEL,
            .panel = NULL,
            .create = column_select_create,
            .handle_key = column_select_handle_key,
            .destroy = column_select_destroy } };

int
init_interface()
{
    int bg, fg;
    const char *term;

    // Initialize curses
    if (!initscr()) {
        fprintf(stderr, "Unable to initialize ncurses mode.\n");
        return -1;
    }

    // Check if user wants a black background
    if (is_option_value("background", "dark")) {
        assume_default_colors(COLOR_WHITE, COLOR_BLACK);
    } else {
        use_default_colors();
    }
    // Enable Colors
    start_color();
    cbreak();

    // Dont write user input on screen
    noecho();
    // Hide the cursor
    curs_set(0);
    // Only delay ESC Sequences 25 ms (we dont want Escape sequences)
    ESCDELAY = 25;

    // Redefine some keys
    term = getenv("TERM");
    if (term && (!strcmp(term, "xterm") || !strcmp(term, "xterm-color") || !strcmp(term, "vt220"))) {
       define_key("\033[H", KEY_HOME);
       define_key("\033[F", KEY_END);
       define_key("\033OP", KEY_F(1));
       define_key("\033OQ", KEY_F(2));
       define_key("\033OR", KEY_F(3));
       define_key("\033OS", KEY_F(4));
       define_key("\033[11~", KEY_F(1));
       define_key("\033[12~", KEY_F(2));
       define_key("\033[13~", KEY_F(3));
       define_key("\033[14~", KEY_F(4));
       define_key("\033[17;2~", KEY_F(18));
    }

    if (is_option_value("background", "dark")) {
        fg = COLOR_WHITE;
        bg = COLOR_BLACK;
    } else {
        fg = COLOR_DEFAULT;
        bg = COLOR_DEFAULT;
    }

    // Initialize colorpairs
    init_pair(CP_CYAN_ON_DEF, COLOR_CYAN, bg);
    init_pair(CP_YELLOW_ON_DEF, COLOR_YELLOW, bg);
    init_pair(CP_MAGENTA_ON_DEF, COLOR_MAGENTA, bg);
    init_pair(CP_GREEN_ON_DEF, COLOR_GREEN, bg);
    init_pair(CP_RED_ON_DEF, COLOR_RED, bg);
    init_pair(CP_BLUE_ON_DEF, COLOR_BLUE, bg);
    init_pair(CP_WHITE_ON_DEF, COLOR_WHITE, bg);
    init_pair(CP_DEF_ON_CYAN, fg, COLOR_CYAN);
    init_pair(CP_DEF_ON_BLUE, fg, COLOR_BLUE);
    init_pair(CP_BLACK_ON_CYAN, COLOR_BLACK, COLOR_CYAN);
    init_pair(CP_WHITE_ON_CYAN, COLOR_WHITE, COLOR_CYAN);
    init_pair(CP_BLUE_ON_CYAN, COLOR_BLUE, COLOR_CYAN);
    init_pair(CP_BLUE_ON_WHITE, COLOR_BLUE, COLOR_WHITE);
    init_pair(CP_CYAN_ON_WHITE, COLOR_CYAN, COLOR_WHITE);
    init_pair(CP_CYAN_ON_BLACK, COLOR_CYAN, COLOR_BLACK);

    return 0;
}

int
deinit_interface()
{
    // Clear screen before leaving
    refresh();
    // End ncurses mode
    return endwin();
}

ui_t *
ui_create(ui_t *ui)
{
    // Creating a panel changes the panel stack, so we must be sure
    // that during this process, noone requests stack status
    // p.e. topmost panel
    pthread_mutex_lock(&refresh_lock);
    // If ui has no panel
    if (!ui_get_panel(ui)) {
        // Create the new panel for this ui
        if (ui->create) {
            ui->panel = ui->create();
        }
    }
    pthread_mutex_unlock(&refresh_lock);
    // And return it
    return ui;
}

void
ui_destroy(ui_t *ui)
{
    PANEL *panel;
    // If thre is no ui panel, we're done
    if (!(panel = ui_get_panel(ui)))
        return;

    // If panel has a destructor function use it
    if (ui->destroy)
        ui->destroy(panel);

    // Initialize panel pointer
    ui->panel = NULL;
}

PANEL *
ui_get_panel(ui_t *ui)
{
    // Return panel pointer of ui struct
    return (ui) ? ui->panel : NULL;
}

int
ui_redraw_required(ui_t *ui, sip_msg_t *msg)
{
    // By default ui does not require redrawing
    int ret = -1;

    //! Sanity check, this should not happen
    if (!ui)
        return ret;

    // Ensure no panel content is being updated while we check redraw
    pthread_mutex_lock(&ui->lock);
    // Request the panel to draw on the scren
    if (ui->redraw_required) {
        ret = ui->redraw_required(ui_get_panel(ui), msg);
    }
    pthread_mutex_unlock(&ui->lock);
    // If no redraw capabilities, never redraw
    return ret;
}

int
ui_draw_panel(ui_t *ui)
{
    int ret = 0;

    //! Sanity check, this should not happen
    if (!ui)
        return -1;

    // Make this panel the topmost panel
    top_panel(ui_get_panel(ui));

    // Ensure no other thread access panel content while drawing
    pthread_mutex_lock(&ui->lock);

    // Request the panel to draw on the scren
    if (ui->draw) {
        ret = ui->draw(ui_get_panel(ui));
    }

    // Update panel stack
    update_panels();
    doupdate();
    pthread_mutex_unlock(&ui->lock);

    return ret;
}

void
ui_help(ui_t *ui)
{
    // If current ui has help function
    if (ui->help) {
        ui->help(ui_get_panel(ui));
    }
}

int
ui_handle_key(ui_t *ui, int key)
{
    int ret = 0;
    // Avoid processing user action while screen is being updated
    // from capture thread
    pthread_mutex_lock(&ui->lock);
    if (ui->handle_key)
        ret = ui->handle_key(ui_get_panel(ui), key);
    pthread_mutex_unlock(&ui->lock);
    return ret;
}

ui_t *
ui_find_by_panel(PANEL *panel)
{
    int i;
    // Panel pool is fixed size
    int panelcnt = sizeof(panel_pool) / sizeof(ui_t);
    // Return ui pointer if found
    for (i = 0; i < panelcnt; i++) {
        if (panel_pool[i].panel == panel)
            return &panel_pool[i];
    }
    return NULL;
}

ui_t *
ui_find_by_type(int type)
{
    int i;
    // Panel pool is fixed size
    int panelcnt = sizeof(panel_pool) / sizeof(ui_t);
    // Return ui pointer if found
    for (i = 0; i < panelcnt; i++) {
        if (panel_pool[i].type == type)
            return &panel_pool[i];
    }
    return NULL;
}

int
wait_for_input(ui_t *ui)
{
    WINDOW *win;

    // Keep getting keys until panel is destroyed
    while (ui_get_panel(ui)) {
        // Redraw this panel
        if (ui_draw_panel(ui) != 0)
            return -1;

        // Enable key input on current panel
        win = panel_window(ui_get_panel(ui));
        keypad(win, TRUE);

        // Get pressed key
        int c = wgetch(win);

        // Check if current panel has custom bindings for that key
        if ((c = ui_handle_key(ui, c)) == 0) {
            // Key has been handled by panel
            continue;
        }

        // Key not handled by UI, try default handler
        default_handle_key(ui, c);
    }

    return -1;
}

int
default_handle_key(ui_t *ui, int key)
{
    // Otherwise, use standard keybindings
    switch (key) {
    case 'C':
    case KEY_F(8):
        toggle_option("syntax");
        break;
    case 'c':
    case KEY_F(7):
        if (is_option_enabled("color.request")) {
            toggle_option("color.request");
            toggle_option("color.callid");
        } else if (is_option_enabled("color.callid")) {
            toggle_option("color.callid");
            toggle_option("color.cseq");
        } else if (is_option_enabled("color.cseq")) {
            toggle_option("color.cseq");
            toggle_option("color.request");
        }
        break;
    case 'l':
        toggle_option("sngrep.displayhost");
        break;
    case 'p':
        // Toggle capture option
        toggle_option("sip.capture");
        break;
    case KEY_F(1):
    case 'h':
    case '?':
        ui_help(ui);
        break;
    case 'q':
    case 'Q':
    case 27: /* KEY_ESC */
        ui_destroy(ui);
        break;
    }

    return key;
}

void
ui_new_msg_refresh(sip_msg_t *msg)
{
    ui_t * ui;
    PANEL * panel;

    // Ensure panel stack won't change while updating.
    // This can only happen when a new panel has been created due to
    // some user iteraction
    pthread_mutex_lock(&refresh_lock);
    // Get the topmost panel
    if ((panel = panel_below(NULL))) {
        // If topmost panel does not has an UI, it can not be refreshed
        if ((ui = ui_find_by_panel(panel))) {
            // If ui needs to be update, redraw it
            if (ui_redraw_required(ui, msg) == 0) {
                ui_draw_panel(ui);
            }
        }
    }
    pthread_mutex_unlock(&refresh_lock);
}

void
title_foot_box(WINDOW *win)
{
    int height, width;

    // Get window size
    getmaxyx(win, height, width);
    box(win, 0, 0);
    mvwaddch(win, 2, 0, ACS_LTEE);
    mvwhline(win, 2, 1, ACS_HLINE, width - 2);
    mvwaddch(win, 2, width - 1, ACS_RTEE);
    mvwaddch(win, height - 3, 0, ACS_LTEE);
    mvwhline(win, height - 3, 1, ACS_HLINE, width - 2);
    mvwaddch(win, height - 3, width - 1, ACS_RTEE);

}

void
draw_keybindings(PANEL *panel, const char *keybindings[], int count)
{
    int height, width, key, xpos = 0;

    // Get window available space
    WINDOW *win = panel_window(panel);
    getmaxyx(win, height, width);

    // Reverse colors on monochrome terminals
    if (!has_colors())
        wattron(win, A_REVERSE);

    // Write a line all the footer width
    wattron(win, COLOR_PAIR(CP_DEF_ON_CYAN));
    mvwprintw(win, height - 1, 0, "%*s", width, "");

    // Draw keys and their actions
    for (key = 0; key < count; key += 2) {
        wattron(win, A_BOLD | COLOR_PAIR(CP_WHITE_ON_CYAN));
        mvwprintw(win, height - 1, xpos, "%-*s", strlen(keybindings[key]) + 1, keybindings[key]);
        xpos += strlen(keybindings[key]) + 1;
        wattroff(win, A_BOLD | COLOR_PAIR(CP_WHITE_ON_CYAN));
        wattron(win, COLOR_PAIR(CP_BLACK_ON_CYAN));
        mvwprintw(win, height - 1, xpos, "%-*s", strlen(keybindings[key + 1]) + 1,
                  keybindings[key + 1]);
        wattroff(win, COLOR_PAIR(CP_BLACK_ON_CYAN));
        xpos += strlen(keybindings[key + 1]) + 3;
    }

    // Disable reverse mode in all cases
    wattroff(win, A_REVERSE);
}

void
draw_title(PANEL *panel, const char *title)
{
    int height, width, key, xpos = 0;

    // Get window available space
    WINDOW *win = panel_window(panel);
    getmaxyx(win, height, width);

    // Reverse colors on monochrome terminals
    if (!has_colors())
        wattron(win, A_REVERSE);

    // Center the title on the window
    wattron(win, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));
    mvwprintw(win, 0, 0, "%*s", width, "");
    mvwprintw(win, 0, (width - strlen(title)) / 2, "%s", title);
    wattroff(win, A_BOLD | A_REVERSE | COLOR_PAIR(CP_DEF_ON_CYAN));

}

void
draw_vscrollbar(WINDOW *win, int value, int max, bool left)
{
    int height, width, cline, scrollen, scrollypos, scrollxpos;

    // Get window available space
    getmaxyx(win, height, width);

    // If no even a screen has been filled, don't draw it
    if (max < height)
        return;

    // Display the scrollbar left or right
    scrollxpos = (left) ? 0 : width - 1;

    // Initialize scrollbar line
    mvwvline(win, 0, scrollxpos, ACS_VLINE, height);

    // How long the scroll will be
    scrollen = (height * 1.0f / max * height) + 0.5;

    // Where will the scroll start
    scrollypos = height * (value * 1.0f / max);

    // Draw the N blocks of the scrollbar
    for (cline = 0; cline < scrollen; cline++)
        mvwaddch(win, cline + scrollypos, scrollxpos, ACS_CKBOARD);

}

int
draw_message(WINDOW *win, sip_msg_t *msg)
{
    return draw_message_pos(win, msg, 0);
}

int
draw_message_pos(WINDOW *win, sip_msg_t *msg, int starting)
{
    int height, width, line, column, i;
    char *cur_line = msg->payload;
    int syntax = is_option_enabled("syntax");

    // Default text format
    int attrs = A_NORMAL | COLOR_PAIR(CP_DEFAULT);
    if (syntax)
        wattrset(win, attrs);

    // Get window of main panel
    getmaxyx(win, height, width);

    // Print msg payload
    line = starting;
    column = 0;
    for (i = 0; i < strlen(msg->payload); i++) {
        // If syntax highlighting is enabled
        if (syntax) {
            // First line highlight
            if (line == starting) {
                // Request syntax
                if (i == 0 && strncmp(cur_line, "SIP/2.0", 7))
                    attrs = A_BOLD | COLOR_PAIR(CP_YELLOW_ON_DEF);

                // Response syntax
                if (i == 8 && !strncmp(cur_line, "SIP/2.0", 7))
                    attrs = A_BOLD | COLOR_PAIR(CP_RED_ON_DEF);

                // SIP URI syntax
                if (!strncasecmp(msg->payload + i, "sip:", 4)) {
                    attrs = A_BOLD | COLOR_PAIR(CP_CYAN_ON_DEF);
                }
            } else {

                // Header syntax
                if (strchr(cur_line, ':') && msg->payload + i < strchr(cur_line, ':'))
                    attrs = A_NORMAL | COLOR_PAIR(CP_GREEN_ON_DEF);

                // Call-ID Header syntax
                if (!strncasecmp(cur_line, "Call-ID:", 8) && column > 8)
                    attrs = A_BOLD | COLOR_PAIR(CP_MAGENTA_ON_DEF);

                // CSeq Heaedr syntax
                if (!strncasecmp(cur_line, "CSeq:", 5) && column > 5 && !isdigit(msg->payload[i]))
                    attrs = A_NORMAL | COLOR_PAIR(CP_YELLOW_ON_DEF);

                // tag and branch syntax
                if (i > 0 && msg->payload[i - 1] == ';') {
                    // Highlight branch if requested
                    if (is_option_enabled("syntax.branch")) {
                        if (!strncasecmp(msg->payload + i, "branch", 6)) {
                            attrs = A_BOLD | COLOR_PAIR(CP_CYAN_ON_DEF);
                        }
                    }
                    // Highlight tag if requested
                    if (is_option_enabled("syntax.tag")) {
                        if (!strncasecmp(msg->payload + i, "tag", 3)) {
                            if (!strncasecmp(cur_line, "From:", 5)) {
                                attrs = A_BOLD | COLOR_PAIR(CP_DEFAULT);
                            } else {
                                attrs = A_BOLD | COLOR_PAIR(CP_GREEN_ON_DEF);
                            }
                        }
                    }
                }

                // SDP syntax
                if (strcspn(cur_line, "=") == 1)
                    attrs = A_NORMAL | COLOR_PAIR(CP_DEFAULT);
            }

            // Remove previous syntax
            if (strcspn(msg->payload + i, " \n;<>") == 0) {
                wattroff(win, attrs);
                attrs = A_NORMAL | COLOR_PAIR(CP_DEFAULT);
            }

            // Syntax hightlight text!
            wattron(win, attrs);
        }

        // Dont print this characters
        if (msg->payload[i] == '\r')
            continue;

        // Store where the line begins
        if (msg->payload[i] == '\n')
            cur_line = msg->payload + i + 1;

        // Move to the next line if line is filled or a we reach a line break
        if (column > width || msg->payload[i] == '\n') {
            line++;
            column = 0;
            continue;
        }

        // Put next character in position
        mvwaddch(win, line, column++, msg->payload[i]);

        // Stop if we've reached the bottom of the window
        if (line == height)
            break;
    }

    // Disable syntax when leaving
    if (syntax)
        wattroff(win, attrs);

    // Redraw raw win
    wnoutrefresh(win);

    return line - starting;
}
