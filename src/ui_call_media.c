/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2015 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2015 Irontec SL. All rights reserved.
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
 * @file ui_call_media.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_call_media.h
 *
 */
#include <stdlib.h>
#include <string.h>
#include "capture.h"
#include "ui_manager.h"
#include "ui_call_media.h"

/***
 *
 * Some basic ascii art of this panel.
 *
 * +--------------------------------+
 * |            Title               |
 * |   addr1  addr2  addr3  addr4   |
 * |   -----  -----  -----  -----   |
 * |     |      |      |      |     |
 * | port|----->|port  |      |     |
 * |     |  port|<---->|port  |     |
 * | port|<------------|port  |     |
 * |     |      | port |      |     |
 * |     |      |      |      |port |
 * |     |      |      |      |     |
 * | Usefull hotkeys                |
 * +--------------------------------+
 *
 */

/**
 * Ui Structure definition for Call media panel
 */
ui_t ui_call_media =
{
  .type = PANEL_CALL_MEDIA,
  .panel = NULL,
  .create = call_media_create,
  .draw = call_media_draw,
  .handle_key = call_media_handle_key,
  .help = call_media_help
};

PANEL *
call_media_create()
{
    PANEL *panel;
    WINDOW *win;
    int height, width;
    call_media_info_t *info;

    // Calculate window dimensions
    height = 31;
    width = 90;

    // Create a new panel to fill all the screen
    win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);

    // Create a new panel
    panel = new_panel(win);

    // Initialize Call List specific data
    info = malloc(sizeof(call_media_info_t));
    memset(info, 0, sizeof(call_media_info_t));

    // Store it into panel userptr
    set_panel_userptr(panel, (void*) info);

    // Draw a box around the panel window
    wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    title_foot_box(win);
    wattroff(win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Let's draw the fixed elements of the screen
    win = panel_window(panel);
    getmaxyx(win, height, width);

    return panel;
}

call_media_info_t *
call_media_info(PANEL *panel)
{
    return (call_media_info_t*) panel_userptr(panel);
}

int
call_media_draw(PANEL *panel)
{
    call_media_info_t *info;
    WINDOW *win;
    sip_call_t *call;
    sdp_media_t *media;
    int height, width;
    int cline = 5;

    // Get panel information
    info = call_media_info(panel);

    // Get window dimensions
    win = panel_window(panel);
    getmaxyx(win, height, width);
    werase(win);

    // Print panel title
    mvwprintw(win, 1, width / 2 - 10, "Media flows screen");

    // Draw a box around the panel window
    wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    title_foot_box(win);
    wattroff(win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Redraw columns
    call_media_draw_columns(panel);

    // Draw each media line
    for (call = call_group_get_next(info->group, NULL); call;
            call = call_group_get_next(info->group, call)) {
        for (media = call->medias; media; media = media->next) {
            if (call_media_draw_media(panel, media, cline) != 0)
                break;
            // One media fills 2 lines
            cline += 2;
        }
    }

    // Print close "botton"
    wattron(win, A_REVERSE);
    mvwprintw(win, height - 2, width / 2 - 4, "[ Close ]");
    wattroff(win, A_REVERSE);

    return 0;
}

int
call_media_handle_key(PANEL *panel, int key)
{
    int action = -1;

    // Check actions for this key
    while ((action = key_find_action(key, action)) != ERR) {
       // Check if we handle this action
       switch (action) {
           case ACTION_SELECT:
           case ACTION_CONFIRM:
               return KEY_ESC;
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
call_media_help(PANEL *panel)
{
    return 0;
}

int
call_media_draw_media(PANEL *panel, sdp_media_t *media, int cline)
{
    WINDOW *win;
    char packetcnt[30];
    int cnt;
    chtype hline;

    // Get panel window
    win = panel_window(panel);

    // Get origin and destination column
    call_media_column_t *column1 = call_media_column_get(panel, media->addr1);
    call_media_column_t *column2 = call_media_column_get(panel, media->addr2);

    if (column2) {
        int distance = 20 * abs(column1->colpos - column2->colpos) - 1;
        sprintf(packetcnt, "%d/%d", media->txcnt, media->rvcnt);

        cnt = media->rvcnt + media->txcnt;
        if (cnt)
            hline = ACS_HLINE;
        else
            hline = '~';

        if (column1->colpos <= column2->colpos) {
            mvwprintw(win, cline + 1, 3 + 10 - 6 + 20 * column1->colpos, "%d", media->port1);
            mvwprintw(win, cline + 1, 3 + 10 + 2 + 20 * column2->colpos, "%d", media->port2);
            mvwhline(win, cline + 1, 14 + 20 * column1->colpos, hline, distance);
            if (cnt)
                mvwprintw(win, cline,
                          3 + 10 + 20 * column1->colpos + distance / 2 - strlen(packetcnt) / 2,
                          packetcnt);
        } else {
            mvwprintw(win, cline + 1, 3 + 10 - 6 + 20 * column2->colpos, "%d", media->port2);
            mvwprintw(win, cline + 1, 3 + 10 + 2 + 20 * column1->colpos, "%d", media->port1);
            mvwhline(win, cline + 1, 14 + 20 * column2->colpos, hline, distance);
            if (cnt)
                mvwprintw(win, cline,
                          3 + 10 + 20 * column2->colpos + distance / 2 - strlen(packetcnt) / 2,
                          packetcnt);
        }
    } else {
        mvwprintw(win, cline + 1, 7 + 20 * column1->colpos, "%d", media->port1);
    }

    return 0;
}

int
call_media_set_group(sip_call_group_t *group)
{
    call_media_info_t *info;

    if (!ui_call_media.panel)
        return -1;

    if (!(info = call_media_info(ui_call_media.panel)))
        return -1;

    info->group = group;

    return 0;
}

int
call_media_draw_columns(PANEL *panel)
{
    call_media_info_t *info;
    call_media_column_t *column;
    WINDOW *win;
    sip_call_t *call;
    sdp_media_t *media;
    int height, width;
    const char *coltext;
    char address[50], *end;

    // Get panel information
    info = call_media_info(panel);
    // Get window of main panel
    win = panel_window(panel);
    getmaxyx(win, height, width);

    // Load columns
    for (call = call_group_get_next(info->group, NULL); call;
            call = call_group_get_next(info->group, call)) {
        for (media = call->medias; media; media = media->next) {
            call_media_column_add(panel, media->addr1);
            call_media_column_add(panel, media->addr2);
        }
    }

    // Draw vertical columns lines
    for (column = info->columns; column; column = column->next) {
        mvwvline(win, 5, 3 + 10 + 20 * column->colpos, ACS_VLINE, height - 8);
        mvwhline(win, 4, 3 + 3 + 20 * column->colpos, ACS_HLINE, 15);
        mvwaddch(win, 4, 3 + 10 + 20 * column->colpos, ACS_TTEE);

        // Set bold to this address if it's local
        strcpy(address, column->addr);
        if ((end = strchr(address, ':')))
            *end = '\0';
        if (is_local_address_str(address) && setting_enabled(SETTING_CF_LOCALHIGHLIGHT))
            wattron(win, A_BOLD);

        coltext = sip_address_port_format(column->addr);
        mvwprintw(win, 3, 3 + 20 * column->colpos + (22 - strlen(coltext)) / 2, "%s", coltext);
        wattroff(win, A_BOLD);
    }

    return 0;
}

void
call_media_column_add(PANEL *panel, const char *addr)
{
    call_media_info_t *info;
    call_media_column_t *column;
    int colpos = 0;

    if (!addr)
        return;

    if (!(info = call_media_info(panel)))
        return;

    if (call_media_column_get(panel, addr))
        return;

    if (info->columns)
        colpos = info->columns->colpos + 1;

    column = malloc(sizeof(call_media_column_t));
    memset(column, 0, sizeof(call_media_column_t));
    column->addr = addr;
    column->colpos = colpos;
    column->next = info->columns;
    info->columns = column;
}

call_media_column_t *
call_media_column_get(PANEL *panel, const char *addr)
{
    call_media_info_t *info;
    call_media_column_t *columns;

    if (!addr)
        return NULL;

    if (!(info = call_media_info(panel)))
        return NULL;

    columns = info->columns;
    while (columns) {
        if (!strcasecmp(addr, columns->addr)) {
            return columns;
        }
        columns = columns->next;
    }
    return NULL;
}
