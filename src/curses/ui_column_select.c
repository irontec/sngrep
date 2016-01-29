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
#include <regex.h>
#include <unistd.h>
#include <errno.h>
#include "ui_manager.h"
#include "ui_call_list.h"
#include "ui_column_select.h"

/**
 * Ui Structure definition for Message Diff panel
 */
ui_panel_t ui_column_select = {
    .type = PANEL_COLUMN_SELECT,
    .panel = NULL,
    .create = column_select_create,
    .handle_key = column_select_handle_key,
    .destroy = column_select_destroy
};

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
    info = sng_malloc(sizeof(column_select_info_t));

    // Store it into panel userptr
    set_panel_userptr(panel, (void*) info);

    // Initialize the fields
    info->fields[FLD_COLUMNS_ACCEPT] = new_field(1, 10, height - 2, 13, 0, 0);
    info->fields[FLD_COLUMNS_SAVE]   = new_field(1, 10, height - 2, 25, 0, 0);
    info->fields[FLD_COLUMNS_CANCEL] = new_field(1, 10, height - 2, 37, 0, 0);
    info->fields[FLD_COLUMNS_COUNT] = NULL;

    // Field Labels
    set_field_buffer(info->fields[FLD_COLUMNS_ACCEPT], 0, "[ Accept ]");
    set_field_buffer(info->fields[FLD_COLUMNS_SAVE],   0, "[  Save  ]");
    set_field_buffer(info->fields[FLD_COLUMNS_CANCEL], 0, "[ Cancel ]");

    // Create the form and post it
    info->form = new_form(info->fields);
    set_form_sub(info->form, win);
    post_form(info->form);

    // Create a subwin for the menu area
    info->menu_win = derwin(win, 10, width - 2, 7, 0);

    // Initialize one field for each attribute
    for (attr_id = 0; attr_id < SIP_ATTR_COUNT; attr_id++) {
        // Create a new field for this column
        info->items[attr_id] = new_item("[ ]", sip_attr_get_description(attr_id));
        set_item_userptr(info->items[attr_id], (void*) sip_attr_get_name(attr_id));
    }
    info->items[SIP_ATTR_COUNT] = NULL;

    // Create the columns menu and post it
    info->menu = menu = new_menu(info->items);

    // Set current enabled fields
    // FIXME Stealing Call list columns :/
    call_list_info_t *list_info = call_list_info(ui_get_panel(ui_find_by_type(PANEL_CALL_LIST)));

    // Enable current enabled fields and move them to the top
    for (column = 0; column < list_info->columncnt; column++) {
        const char *attr = list_info->columns[column].attr;
        for (attr_id = 0; attr_id < item_count(menu); attr_id++) {
            if (!strcmp(item_userptr(info->items[attr_id]), attr)) {
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
    title_foot_box(panel);
    mvwhline(win, 6, 1, ACS_HLINE, width - 1);
    mvwaddch(win, 6, 0, ACS_LTEE);
    mvwaddch(win, 6, width - 1, ACS_RTEE);
    wattroff(win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Some brief explanation abotu what window shows
    wattron(win, COLOR_PAIR(CP_CYAN_ON_DEF));
    mvwprintw(win, 3, 2, "This windows show the list of columns displayed on Call");
    mvwprintw(win, 4, 2, "List. You can enable/disable using Space Bar and reorder");
    mvwprintw(win, 5, 2, "them using + and - keys.");
    wattroff(win, COLOR_PAIR(CP_CYAN_ON_DEF));

    info->form_active = 0;

    return panel;
}

void
column_select_destroy(PANEL *panel)
{
    int i;
    column_select_info_t *info = column_select_info(panel);

    // Remove menu and items
    unpost_menu(info->menu);
    free_menu(info->menu);
    for (i = 0; i < SIP_ATTR_COUNT; i++)
        free_item(info->items[i]);

    // Remove form and fields
    unpost_form(info->form);
    free_form(info->form);
    for (i = 0; i < FLD_COLUMNS_COUNT; i++)
        free_field(info->fields[i]);

    // Remove panel window and custom info
    delwin(panel_window(panel));
    del_panel(panel);
    sng_free(info);

}


column_select_info_t *
column_select_info(PANEL *panel)
{
    return (column_select_info_t*) panel_userptr(panel);
}

int
column_select_handle_key(PANEL *panel, int key)
{
    // Get panel information
    column_select_info_t *info = column_select_info(panel);

    if (info->form_active) {
        return column_select_handle_key_form(panel, key);
    } else {
        return column_select_handle_key_menu(panel, key);
    }
    return 0;
}

int
column_select_handle_key_menu(PANEL *panel, int key)
{
    MENU *menu;
    ITEM *current;
    int current_idx;
    int action = -1;

    // Get panel information
    column_select_info_t *info = column_select_info(panel);

    menu = info->menu;
    current = current_item(menu);
    current_idx = item_index(current);

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
            case ACTION_SELECT:
                column_select_toggle_item(panel, current);
                column_select_update_menu(panel);
                break;
            case ACTION_COLUMN_MOVE_DOWN:
                column_select_move_item(panel, current, current_idx + 1);
                column_select_update_menu(panel);
                break;
            case ACTION_COLUMN_MOVE_UP:
                column_select_move_item(panel, current, current_idx - 1);
                column_select_update_menu(panel);
                break;
            case ACTION_NEXT_FIELD:
                info->form_active = 1;
                set_menu_fore(menu, COLOR_PAIR(CP_DEFAULT));
                set_field_back(info->fields[FLD_COLUMNS_ACCEPT], A_REVERSE);
                form_driver(info->form, REQ_VALIDATION);
                break;
            case ACTION_CONFIRM:
                column_select_update_columns(panel);
                return 27;
            default:
                // Parse next action
                continue;
        }

        // This panel has handled the key successfully
        break;
    }

    // Draw a scrollbar to the right
    draw_vscrollbar(info->menu_win, top_row(menu), item_count(menu) - 1, 0);
    wnoutrefresh(info->menu_win);
    // Return if this panel has handled or not the key
    return (action == ERR) ? key : 0;
}

int
column_select_handle_key_form(PANEL *panel, int key)
{
    int field_idx, new_field_idx;
    char field_value[48];
    int action = -1;

    // Get panel information
    column_select_info_t *info = column_select_info(panel);

    // Get current field id
    field_idx = field_index(current_field(info->form));

    // Get current field value.
    memset(field_value, 0, sizeof(field_value));
    strcpy(field_value, field_buffer(current_field(info->form), 0));
    strtrim(field_value);

    // Check actions for this key
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch (action) {
            case ACTION_RIGHT:
            case ACTION_NEXT_FIELD:
                form_driver(info->form, REQ_NEXT_FIELD);
                break;
            case ACTION_LEFT:
            case ACTION_PREV_FIELD:
                form_driver(info->form, REQ_PREV_FIELD);
                break;
            case ACTION_SELECT:
            case ACTION_CONFIRM:
                switch(field_idx) {
                    case FLD_COLUMNS_ACCEPT:
                        column_select_update_columns(panel);
                        return 27;
                    case FLD_COLUMNS_CANCEL:
                        return 27;
                    case FLD_COLUMNS_SAVE:
                        column_select_update_columns(panel);
                        column_select_save_columns(panel);
                        return 27;
                }
                break;
            default:
                // Parse next action
                continue;
        }

        // This panel has handled the key successfully
        break;
    }

    // Validate all input data
    form_driver(info->form, REQ_VALIDATION);

    // Change background and cursor of "button fields"
    set_field_back(info->fields[FLD_COLUMNS_ACCEPT], A_NORMAL);
    set_field_back(info->fields[FLD_COLUMNS_SAVE],   A_NORMAL);
    set_field_back(info->fields[FLD_COLUMNS_CANCEL], A_NORMAL);

    // Get current selected field
    new_field_idx = field_index(current_field(info->form));

    // Swap between menu and form
    if (field_idx == FLD_COLUMNS_CANCEL && new_field_idx == FLD_COLUMNS_ACCEPT) {
        set_menu_fore(info->menu, COLOR_PAIR(CP_DEF_ON_BLUE));
        info->form_active = 0;
    } else {
        // Change current field background
        set_field_back(info->fields[new_field_idx], A_REVERSE);
    }

    // Return if this panel has handled or not the key
    return (action == ERR) ? key : 0;
}

void
column_select_update_columns(PANEL *panel)
{
    int column, attr_id;

    // Get panel information
    column_select_info_t *info = column_select_info(panel);

    // Set enabled fields
    PANEL *list_panel = ui_get_panel(ui_find_by_type(PANEL_CALL_LIST));
    call_list_info_t *list_info = call_list_info(list_panel);

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
                             sip_attr_get_title(attr_id), sip_attr_get_width(attr_id));
    }
}

void
column_select_save_columns(PANEL *panel)
{
    int column;
    FILE *fi, *fo;
    char columnopt[128];
    char line[1024];
    char *home = getenv("HOME");
    char userconf[128], tmpfile[128];

    // No home dir...
    if (!home)
        return;

    // Read current $HOME/.sngreprc file
    sprintf(userconf, "%s/.sngreprc", home);
    sprintf(tmpfile, "%s/.sngreprc.old", home);

    // Remove old config file
    unlink(tmpfile);

    // Move home file to temporal dir
    rename(userconf, tmpfile);

    // Create a new user conf file
    if (!(fo = fopen(userconf, "w"))) {
        dialog_run("Unable to open %s: %s", userconf, strerror(errno));
        return;
    }

    // Read all lines of old sngreprc file
    if ((fi = fopen(tmpfile, "r"))) {

        // Read all configuration file
        while (fgets(line, 1024, fi) != NULL) {
            // Ignore lines starting with set (but keep settings)
            if (strncmp(line, "set ", 4) || strncmp(line, "set cl.column", 13)) {
                // Put everyting in new .sngreprc file
                fputs(line, fo);
            }
        }
        fclose(fi);
    }

    // Get panel information
    column_select_info_t *info = column_select_info(panel);

    // Add all selected columns
    for (column = 0; column < item_count(info->menu); column++) {
        // If column is active
        if (!strncmp(item_name(info->items[column]), "[ ]", 3))
            continue;

        // Add the columns settings
        sprintf(columnopt, "set cl.column%d %s\n", column, (const char*) item_userptr(info->items[column]));
        fputs(columnopt, fo);
    }
    fclose(fo);

    // Show a information dialog
    dialog_run("Column layout successfully saved to %s", userconf);
}


void
column_select_move_item(PANEL *panel, ITEM *item, int pos)
{
    // Get panel information
    column_select_info_t *info = column_select_info(panel);

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
    column_select_info_t *info = column_select_info(panel);
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
