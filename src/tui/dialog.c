/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
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
 * @file dialog.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Function for handing modal dialogs
 *
 */

#include "config.h"
#include <string.h>
#include "tui/theme.h"
#include "tui/keybinding.h"
#include "tui/dialog.h"

int
dialog_run(const char *fmt, ...)
{
    char textva[2048];
    va_list ap;
    WINDOW *win;
    int col = 2;
    int line = 2;

    // Get the message from the format string
    va_start(ap, fmt);
    vsprintf(textva, fmt, ap);
    va_end(ap);

    // Determine dialog dimensions
    g_auto(GStrv) dialog_lines = g_strsplit(textva, "\n", 0);
    if (g_strv_length(dialog_lines) == 0) {
        return 1;
    }

    // Calculate height based on text lines and each line length
    gint height = 4 + g_strv_length(dialog_lines);
    gint width = 0;
    for (guint i = 0; i < g_strv_length(dialog_lines); i++) {
        gint line_length = strlen(dialog_lines[i]);
        height += line_length / 50;
        if (line_length > width)
            width = line_length;
    }
    // Some extra space
    width += 10;

    // Check we don't have a too big or small window
    if (width > DIALOG_MAX_WIDTH)
        width = DIALOG_MAX_WIDTH;
    if (width < DIALOG_MIN_WIDTH)
        width = DIALOG_MIN_WIDTH;

    // Create the window
    win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);
    box(win, 0, 0);

    // Write the message into the screen
    for (guint i = 0; i < g_strv_length(dialog_lines); i++) {
        // Split each line, word by word
        g_auto(GStrv) words = g_strsplit(dialog_lines[i], " ", 0);
        for (guint j = 0; j < g_strv_length(words); j++) {
            if ((gint) (col + strlen(words[j])) > width - 2) {
                line++;
                col = 2;
            }
            mvwprintw(win, line, col, "%s", words[j]);
            col += strlen(words[j]) + 1;
        }
        line++;
        col = 2;
    }

    // Write Accept button
    wattron(win, A_REVERSE);
    mvwprintw(win, height - 2, width / 2 - 5, "[ Accept ]");

    curs_set(0);
    // Disable input timeout
    nocbreak();
    cbreak();

    // Wait for input
    keypad(win, TRUE);
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
        if ((gint) (col + strlen(word)) > width - 2) {
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
        mvwhline(win, 4, 4, ACS_CKBOARD, (width - 10) * ((float) perc / 100));

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

    // Initialize
    memset(opts, 0, 4 * 10);

    // Check how many options exists
    for (i = 0; options[i]; i++) {
        if (options[i] == ',')
            optioncnt++;
    }

    // We only support 4 options
    if (optioncnt > 4)
        return -1;

    // Calculate proper width taking into account longest data
    width = strlen(options) + 6 * optioncnt;
    if ((gint) strlen(title) + 4 > width)
        width = strlen(title) + 4;
    if ((gint) strlen(text) > width && strlen(text) < 50)
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
    for (i = 0; text[i]; i++) {
        if (text[i] == '\n')
            height++;
    }

    // Parse each line of payload looking for sdp information
    tofree = str = strdup((char *) options);
    i = 0;
    while ((option = strsep(&str, ",")) != NULL) {
        strcpy(opts[i++], option);
    }
    g_free(tofree);

    // Create a new panel and show centered
    dialog_win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);
    keypad(dialog_win, TRUE);
    curs = curs_set(0);

    // Set the window title
    mvwprintw(dialog_win, 1, (width - strlen(title)) / 2, title);

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
    tofree = str = strdup((char *) text);
    newl = 0;
    while ((word = strsep(&str, " ")) != NULL) {
        if (word[strlen(word) - 1] == '\n') {
            word[strlen(word) - 1] = '\0';
            newl = 1;
        }

        if ((gint) (col + strlen(word)) > width - 2) {
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
    g_free(tofree);
    wattroff(dialog_win, COLOR_PAIR(CP_CYAN_ON_DEF));

    for (;;) {
        // A list of available keys in this window
        for (i = 0; i < optioncnt; i++) {
            if (i == selected) wattron(dialog_win, A_REVERSE);
            mvwprintw(dialog_win, height - 2, 10 + 10 * i, "[  %s  ]", opts[i]);
            wattroff(dialog_win, A_REVERSE);
        }

        // Get pressed key
        key = wgetch(dialog_win);

        // Check actions for this key
        SngAction action = ACTION_NONE;
        while ((action = key_find_action(key, action)) != ACTION_NONE) {
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
                case ACTION_CLOSE:
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
