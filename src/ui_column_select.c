/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2014,2015 Ivan Alonso (Kaian)
 ** Copyright (C) 2014,2015 Irontec SL. All rights reserved.
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
 * @file ui_column_select.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_column_select.h
 *
 */
#include <string.h>
#include <stdlib.h>
#include "ui_manager.h"
#include "ui_call_list.h"
#include "ui_column_select.h"

/**
 * Ui Structure definition for Message Diff panel
 */
ui_t ui_column_select =
    {
      .type = PANEL_COLUMN_SELECT,
      .panel = NULL,
      .create = column_select_create,
      .handle_key = column_select_handle_key,
      .destroy = column_select_destroy };

PANEL *
column_select_create()
{
    int attr_id, column;
    PANEL *panel;
    WINDOW *win;
    MENU *menu;
    int height, width;
    column_select_info_t *info;

    // Calculate window dimensions
    height = 20;
    width = 60;

    // Cerate a new indow for the panel and form
    win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);

    // Create a new panel
    panel = new_panel(win);

    // Initialize Filter panel specific data
    info = malloc(sizeof(column_select_info_t));
    memset(info, 0, sizeof(column_select_info_t));

    // Store it into panel userptr
    set_panel_userptr(panel, (void*) info);

    // Create a subwin for the menu area
    info->menu_win = derwin(win, 10, width - 2, 7, 0);

    // Initialize one field for each attribute
    for (attr_id = 0; attr_id < SIP_ATTR_SENTINEL; attr_id++) {
        // Create a new field for this column
        info->items[attr_id] = new_item("[ ]", sip_attr_get_description(attr_id));
        set_item_userptr(info->items[attr_id], (void*) sip_attr_get_name(attr_id));
    }
    info->items[SIP_ATTR_SENTINEL] = NULL;

    // Create the columns menu and post it
    info->menu = menu = new_menu(info->items);

    // Set current enabled fields
    call_list_info_t *list_info = (call_list_info_t*) panel_userptr(
            ui_get_panel(ui_find_by_type(PANEL_CALL_LIST)));

    // Enable current enabled fields and move them to the top
    for (column = 0; column < list_info->columncnt; column++) {
        const char *desc = list_info->columns[column].title;
        for (attr_id = 0; attr_id < item_count(menu); attr_id++) {
            if (!strcmp(item_description(info->items[attr_id]), desc)) {
                column_select_toggle_item(panel, info->items[attr_id]);
                column_select_move_item(panel, info->items[attr_id], column);
                break;
            }
        }
    }

    // Set main window and sub window
    set_menu_win(menu, win);
    set_menu_sub(menu, derwin(win, 10, width - 5, 7, 2));
    set_menu_format(menu, 10, 1);
    set_menu_mark(menu, "");
    set_menu_fore(menu, COLOR_PAIR(CP_DEF_ON_BLUE));
    menu_opts_off(menu, O_ONEVALUE);
    post_menu(menu);

    // Draw a scrollbar to the right
    draw_vscrollbar(info->menu_win, top_row(menu), item_count(menu) - 1, 0);

    // Set the window title and boxes
    mvwprintw(win, 1, width / 2 - 14, "Call List columns selection");
    wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    title_foot_box(panel_window(panel));
    mvwhline(win, 6, 1, ACS_HLINE, width - 1);
    mvwaddch(win, 6, 0, ACS_LTEE);
    mvwaddch(win, 6, width - 1, ACS_RTEE);
    wattroff(win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Some brief explanation abotu what window shows
    wattron(win, COLOR_PAIR(CP_CYAN_ON_DEF));
    mvwprintw(win, 3, 2, "This windows show the list of columns displayed on Call");
    mvwprintw(win, 4, 2, "List. You can enable/disable using Space Bar and reorder");
    mvwprintw(win, 5, 2, "them using + and - keys.");
    mvwprintw(win, height - 2, 12, "Press Enter when done. Esc to exit.");
    wattroff(win, COLOR_PAIR(CP_CYAN_ON_DEF));

    return panel;
}

void
column_select_destroy(PANEL *panel)
{
    int i, itemcnt;
    column_select_info_t *info = (column_select_info_t*) panel_userptr(panel);

    // Get item count
    itemcnt = item_count(info->menu);

    // Unpost the menu
    unpost_menu(info->menu);

    // Free items
    for (i = 0; i < itemcnt; i++) {
        free_item(info->items[i]);
    }
}

int
column_select_handle_key(PANEL *panel, int key)
{
    MENU *menu;
    ITEM *current;
    int current_idx;

    // Get panel information
    column_select_info_t *info = (column_select_info_t*) panel_userptr(panel);
    menu = info->menu;

    current = current_item(menu);
    current_idx = item_index(current);

    switch (key) {
    case KEY_DOWN:
        menu_driver(menu, REQ_DOWN_ITEM);
        break;
    case KEY_UP:
        menu_driver(menu, REQ_UP_ITEM);
        break;
    case KEY_NPAGE:
        menu_driver(menu, REQ_SCR_DPAGE);
        break;
    case KEY_PPAGE:
        menu_driver(menu, REQ_SCR_UPAGE);
        break;
    case ' ':
        column_select_toggle_item(panel, current);
        column_select_update_menu(panel);
        break;
    case '+':
        column_select_move_item(panel, current, current_idx + 1);
        column_select_update_menu(panel);
        break;
    case '-':
        column_select_move_item(panel, current, current_idx - 1);
        column_select_update_menu(panel);
        break;
    case 10:
        column_select_update_columns(panel);
        return 27;
    default:
        return key;
    }

    // Draw a scrollbar to the right
    draw_vscrollbar(info->menu_win, top_row(menu), item_count(menu) - 1, 0);
    wnoutrefresh(info->menu_win);

    return 0;
}

void
column_select_update_columns(PANEL *panel)
{
    int column, attr_id;

    // Get panel information
    column_select_info_t *info = (column_select_info_t*) panel_userptr(panel);

    // Set enabled fields
    PANEL *list_panel = ui_get_panel(ui_find_by_type(PANEL_CALL_LIST));
    call_list_info_t *list_info = (call_list_info_t*) panel_userptr(list_panel);

    // Reset column count
    list_info->columncnt = 0;

    // Add all selected columns
    for (column = 0; column < item_count(info->menu); column++) {
        // If column is active
        if (!strncmp(item_name(info->items[column]), "[ ]", 3))
            continue;

        // Get column attribute
        attr_id = sip_attr_from_name(item_userptr(info->items[column]));
        // Add a new column to the list
        call_list_add_column(list_panel, attr_id, sip_attr_get_name(attr_id),
                             sip_attr_get_description(attr_id), sip_attr_get_width(attr_id));
    }
}

void
column_select_move_item(PANEL *panel, ITEM *item, int pos)
{
    // Get panel information
    column_select_info_t *info = (column_select_info_t*) panel_userptr(panel);

    // Check we have a valid position
    if (pos == item_count(info->menu) || pos < 0)
        return;

    // Swap position with destination
    int item_pos = item_index(item);
    info->items[item_pos] = info->items[pos];
    info->items[item_pos]->index = item_pos;
    info->items[pos] = item;
    info->items[pos]->index = pos;
}

void
column_select_toggle_item(PANEL *panel, ITEM *item)
{
    // Change item name
    if (!strncmp(item_name(item), "[ ]", 3)) {
        item->name.str = "[*]";
    } else {
        item->name.str = "[ ]";
    }
}

void
column_select_update_menu(PANEL *panel)
{
    // Get panel information
    column_select_info_t *info = (column_select_info_t*) panel_userptr(panel);
    ITEM *current = current_item(info->menu);
    int top_idx = top_row(info->menu);

    // Remove the menu from the subwindow
    unpost_menu(info->menu);
    // Set menu items
    set_menu_items(info->menu, info->items);
    // Put the menu agin into its subwindow
    post_menu(info->menu);

    // Move until the current position is set
    set_top_row(info->menu, top_idx);
    set_current_item(info->menu, current);
}
