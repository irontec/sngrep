/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2026 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2026 Irontec SL. All rights reserved.
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
#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <stdlib.h>
#include <locale.h>
#include "setting.h"
#include "ui_manager.h"
#include "capture.h"
#include "ui_call_list.h"
#include "ui_call_flow.h"
#include "ui_call_raw.h"
#include "ui_filter.h"
#include "ui_msg_diff.h"
#include "ui_column_select.h"
#include "ui_save.h"
#include "ui_settings.h"

/**
 * @brief Available panel windows list
 *
 * This list contains the available list of windows
 * and pointer to their main functions.

 */
static ui_t *panel_pool[] = {
    &ui_call_list,
    &ui_call_flow,
    &ui_call_raw,
    &ui_filter,
    &ui_save,
    &ui_msg_diff,
    &ui_column_select,
    &ui_settings,
    &ui_stats
};

/**
 * @brief Color theme definition
 *
 * Panels draw with color pairs named after the 8 basic colors. A theme
 * replaces those basic colors with its own palette: accent[] is used when
 * the color is the foreground, bar[] when it is the background. Themes keep
 * the terminal default background, so they work with any terminal scheme.
 */
struct color_theme {
    const char *name;
    short bar_fg;           // Foreground over bars and selections
    short accent[8];        // black, red, green, yellow, blue, magenta, cyan, white
    short bar[8];
};

static const struct color_theme themes[] = {
    { "modern", 255,
        { 235, 204, 114, 221, 75, 176, 80, 255 },
        { 235, 131, 65, 136, 61, 96, 31, 250 } },
    { "professional", 255,
        { 236, 167, 108, 179, 110, 139, 109, 255 },
        { 236, 88, 22, 94, 240, 53, 67, 252 } },
    { "simple", 234,
        { 234, 203, 150, 223, 153, 182, 152, 255 },
        { 234, 203, 150, 223, 245, 182, 248, 252 } },
};

void
ncurses_init_colors()
{
    const struct color_theme *theme = NULL;
    short fg, bg, bar_fg;
    short accent[8], bar[8];
    int i;

    // ponytail: themes need a 256 color terminal, others keep the classic colors
    if (COLORS >= 256) {
        for (i = 0; i < (int) (sizeof(themes) / sizeof(themes[0])); i++) {
            if (setting_has_value(SETTING_THEME, themes[i].name))
                theme = &themes[i];
        }
    }

    for (i = 0; i < 8; i++) {
        accent[i] = theme ? theme->accent[i] : i;
        bar[i] = theme ? theme->bar[i] : i;
    }

    // Check if user wants a black background
    fg = bg = COLOR_DEFAULT;
    if (!theme && setting_has_value(SETTING_BACKGROUND, "dark")) {
        fg = COLOR_WHITE;
        bg = COLOR_BLACK;
    }
    assume_default_colors(fg, bg);
    bar_fg = theme ? theme->bar_fg : fg;

    // Initialize colorpairs
    init_pair(CP_CYAN_ON_DEF, accent[COLOR_CYAN], bg);
    init_pair(CP_YELLOW_ON_DEF, accent[COLOR_YELLOW], bg);
    init_pair(CP_MAGENTA_ON_DEF, accent[COLOR_MAGENTA], bg);
    init_pair(CP_GREEN_ON_DEF, accent[COLOR_GREEN], bg);
    init_pair(CP_RED_ON_DEF, accent[COLOR_RED], bg);
    init_pair(CP_BLUE_ON_DEF, accent[COLOR_BLUE], bg);
    init_pair(CP_WHITE_ON_DEF, accent[COLOR_WHITE], bg);
    init_pair(CP_DEF_ON_CYAN, bar_fg, bar[COLOR_CYAN]);
    init_pair(CP_DEF_ON_BLUE, bar_fg, bar[COLOR_BLUE]);
    init_pair(CP_WHITE_ON_BLUE, accent[COLOR_WHITE], bar[COLOR_BLUE]);
    init_pair(CP_BLACK_ON_CYAN, accent[COLOR_BLACK], bar[COLOR_CYAN]);
    init_pair(CP_WHITE_ON_CYAN, accent[COLOR_WHITE], bar[COLOR_CYAN]);
    init_pair(CP_YELLOW_ON_CYAN, accent[COLOR_YELLOW], bar[COLOR_CYAN]);
    init_pair(CP_BLUE_ON_CYAN, accent[COLOR_BLUE], bar[COLOR_CYAN]);
    init_pair(CP_BLUE_ON_WHITE, accent[COLOR_BLUE], bar[COLOR_WHITE]);
    init_pair(CP_CYAN_ON_WHITE, accent[COLOR_CYAN], bar[COLOR_WHITE]);
    init_pair(CP_CYAN_ON_BLACK, accent[COLOR_CYAN], bar[COLOR_BLACK]);
}

int
ncurses_init()
{
    const char *term;

    // Set Locale
    setlocale(LC_CTYPE, "");

    // Initialize curses
    if (!initscr()) {
        fprintf(stderr, "Unable to initialize ncurses mode.\n");
        return -1;
    }

    // Enable Colors
    start_color();
    cbreak();

    // Dont write user input on screen
    noecho();
    // Hide the cursor
    curs_set(0);
    // Only delay ESC Sequences 25 ms (we dont want Escape sequences)
    set_escdelay(25);

    // Redefine some keys
    term = getenv("TERM");
    if (term
        && (!strcmp(term, "xterm") || !strcmp(term, "xterm-color") || !strcmp(term, "vt220"))) {
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

    ncurses_init_colors();

    return 0;
}

void
ncurses_deinit()
{
    // Clear screen before leaving
    refresh();
    // End ncurses mode
    endwin();
}

ui_t *
ui_create_panel(enum panel_types type)
{
    // Find the panel of given type and create it
    return ui_create(ui_find_by_type(type));
}

ui_t *
ui_find_by_panel(PANEL *panel)
{
    int i;
    // Return ui pointer if found
    for (i = 0; i < PANEL_COUNT; i++) {
        if (panel_pool[i]->panel == panel)
            return panel_pool[i];
    }
    return NULL;
}

ui_t *
ui_find_by_type(enum panel_types type)
{
    int i;
    // Return ui pointer if found
    for (i = 0; i < PANEL_COUNT; i++) {
        if (panel_pool[i]->type == type)
            return panel_pool[i];
    }
    return NULL;
}

int
ui_wait_for_input()
{
    ui_t *ui;
    WINDOW *win;
    PANEL *panel;

    // While there are still panels
    while ((panel = panel_below(NULL))) {

        if (was_sigterm_received())
            return 0;

        // Get panel interface structure
        ui = ui_find_by_panel(panel);

        // Set character input timeout 200 ms
        halfdelay(REFRESHTHSECS);

        // Avoid parsing any packet while UI is being drawn
        capture_lock();
        // Query the interface if it needs to be redrawn
        if (ui_draw_redraw(ui)) {
            // Redraw this panel
            if (ui_draw_panel(ui) != 0) {
                capture_unlock();
                return -1;
            }
        }
        capture_unlock();

        // Update panel stack
        update_panels();
        doupdate();

        // Get topmost panel
        panel = panel_below(NULL);

        // Enable key input on current panel
        win = panel_window(panel);
        keypad(win, TRUE);

        // Enable mouse events if current panel supports them
        ui_mouse_setup(ui);

        // Get pressed key
        int c = wgetch(win);

        // Mouse events become keys when possible (wheel, footer hints)
        if (c == KEY_MOUSE)
            c = ui_mouse_translate(ui);

        // Timeout, no key pressed
        if (c == ERR)
            continue;

        capture_lock();
        // Handle received key
        int hld = KEY_NOT_HANDLED;
        while (hld != KEY_HANDLED) {
            // Mouse click handlers are distinct from keyboard handlers.
            if (c == KEY_MOUSE)
                hld = ui_handle_mouse(ui, c);
            else
                hld = ui_handle_key(ui, c);

            if (hld == KEY_HANDLED) {
                // Panel handled this key
                continue;
            } else if (hld == KEY_PROPAGATED) {
                // Destroy current panel
                ui_destroy(ui);
                // Try to handle this key with the previous panel
                ui = ui_find_by_panel(panel_below(NULL));
            } else {
                // Key not handled by UI nor propagated. Use default handler
                hld = ui_default_handle_key(ui, c);
            }
        }
        capture_unlock();
    }

    return 0;
}

int
ui_default_handle_key(ui_t *ui, int key)
{
    int action = -1;

    // Check actions for this key
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch (action) {
            case ACTION_RESIZE_SCREEN:
                ui_resize_panels();
                break;
            case ACTION_TOGGLE_SYNTAX:
                setting_toggle(SETTING_SYNTAX);
                break;
            case ACTION_TOGGLE_HINT:
                setting_toggle(SETTING_ALTKEY_HINT);
                break;
            case ACTION_CYCLE_COLOR:
                setting_toggle(SETTING_COLORMODE);
                break;
            case ACTION_SHOW_ALIAS:
                setting_toggle(SETTING_DISPLAY_ALIAS);
                break;
            case ACTION_SHOW_SETTINGS:
                ui_create_panel(PANEL_SETTINGS);
                break;
            case ACTION_TOGGLE_PAUSE:
                // Pause/Resume capture
                capture_set_paused(!capture_paused());
                break;
            case ACTION_SHOW_HELP:
                ui_help(ui);
                break;
            case ACTION_PREV_SCREEN:
                ui_destroy(ui);
                break;
            default:
                // Parse next action
                continue;
        }
        // Default handler has handled the key
        break;
    }

    // Consider the key handled at this point
    return KEY_HANDLED;
}

void
ui_resize_panels()
{
    PANEL *panel = NULL;

    // While there are still panels
    while ((panel = panel_below(panel))) {
        // Invoke resize for this panel
        ui_resize_panel(ui_find_by_panel(panel));
    }
}

void
title_foot_box(PANEL *panel)
{
    int height, width;
    WINDOW *win = panel_window(panel);

    // Sanity check
    if (!win)
        return;

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

int
draw_message(WINDOW *win, sip_msg_t *msg)
{
    return draw_message_pos(win, msg, 0);
}

int
draw_message_pos(WINDOW *win, sip_msg_t *msg, int starting)
{
    int height, width, line, column, i;
    const char *cur_line, *payload, *method = NULL;
    int syntax = setting_enabled(SETTING_SYNTAX);
    const char *nonascii = setting_get_value(SETTING_CR_NON_ASCII);
    int payload_len;

    // Default text format
    int attrs = A_NORMAL | COLOR_PAIR(CP_DEFAULT);
    if (syntax)
        wattrset(win, attrs);

    // Get window of main panel
    getmaxyx(win, height, width);

    // Get message method (if request)
    if (msg_is_request(msg)) {
        method = sip_method_str(msg->reqresp);
    }

    // Get packet payload
    cur_line = payload = (const char *) msg_get_payload(msg);
    payload_len=packet_payloadlen(msg->packet);

    // Print msg payload
    line = starting;
    column = 0;
    for (i = 0; i < payload_len; i++) {
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
                if (method && i == strlen(method) + 1) {
                    attrs = A_BOLD | COLOR_PAIR(CP_CYAN_ON_DEF);
                }
            } else {

                // Header syntax
                if (strchr(cur_line, ':') && payload + i < strchr(cur_line, ':'))
                    attrs = A_NORMAL | COLOR_PAIR(CP_GREEN_ON_DEF);

                // Call-ID Header syntax
                if (!strncasecmp(cur_line, "Call-ID:", 8) && column > 8)
                    attrs = A_BOLD | COLOR_PAIR(CP_MAGENTA_ON_DEF);

                // CSeq Heaedr syntax
                if (!strncasecmp(cur_line, "CSeq:", 5) && column > 5 && !isdigit(payload[i]))
                    attrs = A_NORMAL | COLOR_PAIR(CP_YELLOW_ON_DEF);

                // tag and branch syntax
                if (i > 0 && payload[i - 1] == ';') {
                    // Highlight branch if requested
                    if (setting_enabled(SETTING_SYNTAX_BRANCH)) {
                        if (!strncasecmp(payload + i, "branch", 6)) {
                            attrs = A_BOLD | COLOR_PAIR(CP_CYAN_ON_DEF);
                        }
                    }
                    // Highlight tag if requested
                    if (setting_enabled(SETTING_SYNTAX_TAG)) {
                        if (!strncasecmp(payload + i, "tag", 3)) {
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
            if (strcspn(payload + i, " \n;<>") == 0) {
                wattroff(win, attrs);
                attrs = A_NORMAL | COLOR_PAIR(CP_DEFAULT);
            }

            // Syntax hightlight text!
            wattron(win, attrs);
        }

        // Dont print this characters
        if (payload[i] == '\r')
            continue;

        // Store where the line begins
        if (payload[i] == '\n')
            cur_line =payload + i + 1;

        // Move to the next line if line is filled or a we reach a line break
        if (column > width - 1 || payload[i] == '\n') {
            line++;
            column = 0;
        }

        // No need to print new line characters
        if (payload[i] == '\n')
            continue;

        // Put next character in position
        if (isascii(payload[i])) {
            mvwaddch(win, line, column++, payload[i]);
        } else {
            mvwaddch(win, line, column++, *nonascii);
        }

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

int
dialog_run(const char *fmt, ...)
{
    char textva[2048];
    va_list ap;
    int height, width;
    WINDOW *win;
    char *word;
    int col = 2;
    int line = 2;

    // Get the message from the format string
    va_start(ap, fmt);
    vsprintf(textva, fmt, ap);
    va_end(ap);

    // Determine dialog dimensions
    height = 6 + (strlen(textva) / 50);
    width = strlen(textva);

    // Check we don't have a too big or small window
    if (width > DIALOG_MAX_WIDTH)
        width = DIALOG_MAX_WIDTH;
    if (width < DIALOG_MIN_WIDTH)
        width = DIALOG_MIN_WIDTH;

    // Create the window
    win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);
    box(win, 0, 0);

    // Write the message into the screen
    for (word = strtok(textva, " "); word; word = strtok(NULL, " ")) {
        if (col + strlen(word) > width - 2) {
            line++;
            col = 2;
        }
        mvwprintw(win, line, col, "%s", word);
        col += strlen(word) + 1;
    }

    // Write Accept button
    wattron(win, A_REVERSE);
    mvwprintw(win, height - 2, width/2 - 5, "[ Accept ]");

    curs_set(0);
    // Disable input timeout
    nocbreak();
    cbreak();

    // Wait for input
    wgetch(win);

    delwin(win);
    return 1;

}

WINDOW *
dialog_progress_run(const char *fmt, ...)
{
    char textva[2048];
    va_list ap;
    int height, width;
    WINDOW *win;
    char *word;
    int col = 2;
    int line = 2;

    // Get the message from the format string
    va_start(ap, fmt);
    vsprintf(textva, fmt, ap);
    va_end(ap);

    // Determine dialog dimensions
    height = 6 + (strlen(textva) / 50);
    width = strlen(textva);

    // Check we don't want a too big or small window
    if (width > DIALOG_MAX_WIDTH)
        width = DIALOG_MAX_WIDTH;
    if (width < DIALOG_MIN_WIDTH)
        width = DIALOG_MIN_WIDTH;

    // Create the window
    win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);
    box(win, 0, 0);

    // Write the message into the screen
    for (word = strtok(textva, " "); word; word = strtok(NULL, " ")) {
        if (col + strlen(word) > width - 2) {
            line++;
            col = 2;
        }
        mvwprintw(win, line, col, "%s", word);
        col += strlen(word) + 1;
    }

    curs_set(0);
    wrefresh(win);
    // Disable input timeout
    nocbreak();
    cbreak();

    return win;

}

void
dialog_progress_set_value(WINDOW *win, int perc)
{
    int width;

    width = getmaxx(win);
    mvwhline(win, 4, 4, '-', width - 10);
    mvwaddch(win, 4, 3, '[');
    mvwaddch(win, 4, width - 7, ']');
    mvwprintw(win, 4, width - 5, "%d%%", perc);

    if (perc > 0 && perc <= 100)
        mvwhline(win, 4, 4, ACS_CKBOARD, (width - 10) * ((float)perc/100));

    wrefresh(win);
}

void
dialog_progress_destroy(WINDOW *win)
{
    delwin(win);
}

int
dialog_confirm(const char *title, const char *text, const char *options)
{
    WINDOW *dialog_win;
    int key, i, curs, newl, height, width;
    char *str, *tofree, *option, *word;
    int selected = 0;
    int optioncnt = 1;
    int col = 2;
    int line = 3;
    char opts[4][10];
    int action = -1;

    // Initialize
    memset(opts, 0, 4 * 10);

    // Check how many options exists
    for (i=0; options[i]; i++) {
        if (options[i] == ',')
            optioncnt++;
    }

    // We only support 4 options
    if (optioncnt > 4)
        return -1;

    // Calculate proper width taking into acount longest data
    width = strlen(options) + 6 * optioncnt;
    if (strlen(title) + 4 > width)
        width = strlen(title) + 4;
    if (strlen(text) > width && strlen(text) < 50)
        width = strlen(text);

    // Check we don't want a too big or small window
    if (width > DIALOG_MAX_WIDTH)
        width = DIALOG_MAX_WIDTH;
    if (width < DIALOG_MIN_WIDTH)
        width = DIALOG_MIN_WIDTH;

    // Determine dialog dimensions
    height = 7; // Minimum for header and button lines
    height += (strlen(text) / width);   // Space for the text.
    // Add one extra line for each newline in the text
    for (i=0; text[i]; i++) {
        if (text[i] == '\n')
            height++;
    }

    // Parse each line of payload looking for sdp information
    tofree = str = strdup((char*)options);
    i = 0;
    while (i < (sizeof(opts) / sizeof(opts[0])) && (option = strsep(&str, ",")) != NULL) {
        sng_strlcpy(opts[i], option, sizeof(opts[i]));
        i++;
    }
    free(tofree);

    // Create a new panel and show centered
    dialog_win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);
    keypad(dialog_win, TRUE);
    curs = curs_set(0);

    // Set the window title
    mvwprintw(dialog_win, 1, (width - strlen(title)) / 2, "%s", title);

    // Write border and boxes around the window
    wattron(dialog_win, COLOR_PAIR(CP_BLUE_ON_DEF));
    box(dialog_win, 0, 0);

    mvwhline(dialog_win, 2, 1, ACS_HLINE, width);
    mvwaddch(dialog_win, 2, 0, ACS_LTEE);
    mvwaddch(dialog_win, 2, width - 1, ACS_RTEE);

    mvwhline(dialog_win, height - 3, 1, ACS_HLINE, width);
    mvwaddch(dialog_win, height - 3, 0, ACS_LTEE);
    mvwaddch(dialog_win, height - 3, width - 1, ACS_RTEE);

    // Exit confirmation message message
    wattron(dialog_win, COLOR_PAIR(CP_CYAN_ON_DEF));
    // Write the message into the screen
    tofree = str = strdup((char*)text);
    newl = 0;
    while ((word = strsep(&str, " ")) != NULL) {
        if (word[strlen(word)-1] == '\n') {
            word[strlen(word)-1] = '\0';
            newl = 1;
        }

        if (col + strlen(word) > width - 2) {
            line++;
            col = 2;
        }
        mvwprintw(dialog_win, line, col, "%s", word);
        col += strlen(word) + 1;
        if (newl) {
            line++;
            col = 2;
            newl = 0;
        }
    }
    free(tofree);
    wattroff(dialog_win, COLOR_PAIR(CP_CYAN_ON_DEF));

    for (;;) {
        // A list of available keys in this window
        for (i = 0; i < optioncnt; i++ ) {
            if (i == selected) wattron(dialog_win, A_REVERSE);
            mvwprintw(dialog_win, height - 2, 10 + 10 * i, "[  %s  ]", opts[i]);
            wattroff(dialog_win, A_REVERSE);
        }

        // Get pressed key
        key = wgetch(dialog_win);

        // Clicking an option chooses it
        if (key == KEY_MOUSE && ui_mouse_read()) {
            int y, x;
            if (ui_mouse_pos(dialog_win, &y, &x) && y == height - 2) {
                for (i = 0; i < optioncnt; i++) {
                    if (x >= 10 + 10 * i && x < 10 + 10 * i + (int) strlen(opts[i]) + 6) {
                        selected = i;
                        goto done;
                    }
                }
            }
            continue;
        }

        // Check actions for this key
        while ((action = key_find_action(key, action)) != ERR) {
            // Check if we handle this action
            switch (action) {
                case ACTION_RIGHT:
                    selected++;
                    break;
                case ACTION_LEFT:
                case ACTION_NEXT_FIELD:
                    selected--;
                    break;
                case ACTION_SELECT:
                case ACTION_CONFIRM:
                    goto done;
                case ACTION_PREV_SCREEN:
                    selected = -1;
                    goto done;
                default:
                    // Parse next action
                    continue;
            }
            // key handled successfully
            break;
        }

        // Cycle through ooptions
        if (selected > optioncnt - 1)
            selected = 0;
        if (selected < 0)
            selected = optioncnt - 1;
    }

done:
    delwin(dialog_win);
    curs_set(curs);
    return selected;
}
