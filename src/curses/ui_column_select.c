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
ui_t ui_column_select = {
    .type = PANEL_COLUMN_SELECT,
    .panel = NULL,
    .create = column_select_create,
    .handle_key = column_select_handle_key,
    .destroy = column_select_destroy
};

void
column_select_create(ui_t *ui)
{
    int attr_id, column;
    MENU *menu;
    column_select_info_t *info;

    // Cerate a new indow for the panel and form
    ui_panel_create(ui, 20, 60);

    // Initialize Filter panel specific data
    info = sng_malloc(sizeof(column_select_info_t));

    // Store it into panel userptr
    set_panel_userptr(ui->panel, (void*) info);

    // Initialize the fields
    info->fields[FLD_COLUMNS_ACCEPT] = new_field(1, 10, ui->height - 2, 13, 0, 0);
    info->fields[FLD_COLUMNS_SAVE]   = new_field(1, 10, ui->height - 2, 25, 0, 0);
    info->fields[FLD_COLUMNS_CANCEL] = new_field(1, 10, ui->height - 2, 37, 0, 0);
    info->fields[FLD_COLUMNS_COUNT] = NULL;

    // Field Labels
    set_field_buffer(info->fields[FLD_COLUMNS_ACCEPT], 0, "[ Accept ]");
    set_field_buffer(info->fields[FLD_COLUMNS_SAVE],   0, "[  Save  ]");
    set_field_buffer(info->fields[FLD_COLUMNS_CANCEL], 0, "[ Cancel ]");

    // Create the form and post it
    info->form = new_form(info->fields);
    set_form_sub(info->form, ui->win);
    post_form(info->form);

    // Create a subwin for the menu area
    info->menu_win = derwin(ui->win, 10, ui->width - 2, 7, 0);

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
    call_list_info_t *list_info = call_list_info(ui_find_by_type(PANEL_CALL_LIST));

    // Enable current enabled fields and move them to the top
    for (column = 0; column < list_info->columncnt; column++) {
        const char *attr = list_info->columns[column].attr;
        for (attr_id = 0; attr_id < item_count(menu); attr_id++) {
            if (!strcmp(item_userptr(info->items[attr_id]), attr)) {
                column_select_toggle_item(ui, info->items[attr_id]);
                column_select_move_item(ui, info->items[attr_id], column);
                break;
            }
        }
    }

    // Set main window and sub window
    set_menu_win(menu, ui->win);
    set_menu_sub(menu, derwin(ui->win, 10, ui->width - 5, 7, 2));
    set_menu_format(menu, 10, 1);
    set_menu_mark(menu, "");
    set_menu_fore(menu, COLOR_PAIR(CP_DEF_ON_BLUE));
    menu_opts_off(menu, O_ONEVALUE);
    post_menu(menu);

    // Draw a scrollbar to the right
    info->scroll = ui_set_scrollbar(info->menu_win, SB_VERTICAL, SB_RIGHT);
    info->scroll.max = item_count(menu) - 1;
    ui_scrollbar_draw(info->scroll);

    // Set the window title and boxes
    mvwprintw(ui->win, 1, ui->width / 2 - 14, "Call List columns selection");
    wattron(ui->win, COLOR_PAIR(CP_BLUE_ON_DEF));
    title_foot_box(ui->panel);
    mvwhline(ui->win, 6, 1, ACS_HLINE, ui->width - 1);
    mvwaddch(ui->win, 6, 0, ACS_LTEE);
    mvwaddch(ui->win, 6, ui->width - 1, ACS_RTEE);
    wattroff(ui->win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Some brief explanation abotu what window shows
    wattron(ui->win, COLOR_PAIR(CP_CYAN_ON_DEF));
    mvwprintw(ui->win, 3, 2, "This windows show the list of columns displayed on Call");
    mvwprintw(ui->win, 4, 2, "List. You can enable/disable using Space Bar and reorder");
    mvwprintw(ui->win, 5, 2, "them using + and - keys.");
    wattroff(ui->win, COLOR_PAIR(CP_CYAN_ON_DEF));

    info->form_active = 0;
}

void
column_select_destroy(ui_t *ui)
{
    int i;
    column_select_info_t *info = column_select_info(ui);

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

    sng_free(info);

    // Remove panel window and custom info
    ui_panel_destroy(ui);
}


column_select_info_t *
column_select_info(ui_t *ui)
{
    return (column_select_info_t*) panel_userptr(ui->panel);
}

int
column_select_handle_key(ui_t *ui, int key)
{
    // Get panel information
    column_select_info_t *info = column_select_info(ui);

    if (info->form_active) {
        return column_select_handle_key_form(ui, key);
    } else {
        return column_select_handle_key_menu(ui, key);
    }
    return 0;
}

int
column_select_handle_key_menu(ui_t *ui, int key)
{
    MENU *menu;
    ITEM *current;
    int current_idx;
    int action = -1;

    // Get panel information
    column_select_info_t *info = column_select_info(ui);

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
                column_select_toggle_item(ui, current);
                column_select_update_menu(ui);
                break;
            case ACTION_COLUMN_MOVE_DOWN:
                column_select_move_item(ui, current, current_idx + 1);
                column_select_update_menu(ui);
                break;
            case ACTION_COLUMN_MOVE_UP:
                column_select_move_item(ui, current, current_idx - 1);
                column_select_update_menu(ui);
                break;
            case ACTION_NEXT_FIELD:
                info->form_active = 1;
                set_menu_fore(menu, COLOR_PAIR(CP_DEFAULT));
                set_field_back(info->fields[FLD_COLUMNS_ACCEPT], A_REVERSE);
                form_driver(info->form, REQ_VALIDATION);
                break;
            case ACTION_CONFIRM:
                column_select_update_columns(ui);
                ui_destroy(ui);
                return KEY_HANDLED;
            default:
                // Parse next action
                continue;
        }

        // This panel has handled the key successfully
        break;
    }

    // Draw a scrollbar to the right
    info->scroll.pos = top_row(menu);
    ui_scrollbar_draw(info->scroll);
    wnoutrefresh(info->menu_win);

    // Return if this panel has handled or not the key
    return (action == ERR) ? KEY_NOT_HANDLED : KEY_HANDLED;
}

int
column_select_handle_key_form(ui_t *ui, int key)
{
    int field_idx, new_field_idx;
    char field_value[48];
    int action = -1;

    // Get panel information
    column_select_info_t *info = column_select_info(ui);

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
                        column_select_update_columns(ui);
                        ui_destroy(ui);
                        return KEY_HANDLED;
                    case FLD_COLUMNS_CANCEL:
                        ui_destroy(ui);
                        return KEY_HANDLED;
                    case FLD_COLUMNS_SAVE:
                        column_select_update_columns(ui);
                        column_select_save_columns(ui);
                        ui_destroy(ui);
                        return KEY_HANDLED;
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
    return (action == ERR) ? KEY_NOT_HANDLED : KEY_HANDLED;
}

void
column_select_update_columns(ui_t *ui)
{
    int column, attr_id;

    // Get panel information
    column_select_info_t *info = column_select_info(ui);

    // Set enabled fields
    ui_t *ui_list = ui_find_by_type(PANEL_CALL_LIST);
    call_list_info_t *list_info = call_list_info(ui_list);

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
        call_list_add_column(ui_list, attr_id, sip_attr_get_name(attr_id),
                             sip_attr_get_title(attr_id), sip_attr_get_width(attr_id));
    }
}

void
column_select_save_columns(ui_t *ui)
{
    int column;
    FILE *fi, *fo;
    char columnopt[128];
    char line[1024];
    char *rcfile;
    char *userconf = NULL;
    char *tmpfile  = NULL;

    // Use current $SNGREPRC or $HOME/.sngreprc file
    if ((rcfile = getenv("SNGREPRC"))) {
        if ((userconf = sng_malloc(strlen(rcfile) + RCFILE_EXTRA_LEN))) {
            if ((tmpfile = sng_malloc(strlen(rcfile) + RCFILE_EXTRA_LEN))) {
                sprintf(userconf, "%s", rcfile);
                sprintf(tmpfile, "%s.old", rcfile);
            } else {
                sng_free(userconf);
                return;
            }
        } else {
            return;
        }
    } else if ((rcfile = getenv("HOME"))) {
        if ((userconf = sng_malloc(strlen(rcfile) + RCFILE_EXTRA_LEN))) {
            if ((tmpfile = sng_malloc(strlen(rcfile) + RCFILE_EXTRA_LEN))) {
                sprintf(userconf, "%s/.sngreprc", rcfile);
                sprintf(tmpfile, "%s/.sngreprc.old", rcfile);
            } else {
                sng_free(userconf);
                return;
            }
        } else {
            return;
        }
    } else {
        return;
    }

    // Remove old config file
    unlink(tmpfile);

    // Move user conf file to temporal file
    rename(userconf, tmpfile);

    // Create a new user conf file
    if (!(fo = fopen(userconf, "w"))) {
        dialog_run("Unable to open %s: %s", userconf, strerror(errno));
        sng_free(userconf);
        sng_free(tmpfile);
        return;
    }

    // Read all lines of old sngreprc file
    if ((fi = fopen(tmpfile, "r"))) {

        // Read all configuration file
        while (fgets(line, 1024, fi) != NULL) {
            // Ignore lines starting with set (but keep settings)
            if (strncmp(line, "set ", 4) || strncmp(line, "set cl.column", 13)) {
                // Put everyting in new user conf file
                fputs(line, fo);
            }
        }
        fclose(fi);
    }

    // Get panel information
    column_select_info_t *info = column_select_info(ui);

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

    sng_free(userconf);
    sng_free(tmpfile);
}


void
column_select_move_item(ui_t *ui, ITEM *item, int pos)
{
    // Get panel information
    column_select_info_t *info = column_select_info(ui);

    // Check we have a valid position
    if (pos == item_count(info->menu) || pos < 0)
        return;

    // Swap position with destination
    int item_pos = item_index(item);
    info->items[item_pos] = info->items[pos];
    info->items[pos] = item;
    set_menu_items(info->menu, info->items);
}

void
column_select_toggle_item(ui_t *ui, ITEM *item)
{
    // Get panel information
    column_select_info_t *info = column_select_info(ui);

    int pos = item_index(item);

    // Change item name
    if (!strncmp(item_name(item), "[ ]", 3)) {
        info->items[pos] = new_item("[*]", item_description(item));
    } else {
        info->items[pos] = new_item("[ ]", item_description(item));
    }

    // Restore menu item
    set_item_userptr(info->items[pos], item_userptr(item));
    set_menu_items(info->menu, info->items);

    // Destroy old item
    free_item(item);
}

void
column_select_update_menu(ui_t *ui)
{
    // Get panel information
    column_select_info_t *info = column_select_info(ui);
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

    // Force menu redraw
    menu_driver(info->menu, REQ_UP_ITEM);
    menu_driver(info->menu, REQ_DOWN_ITEM);
}
