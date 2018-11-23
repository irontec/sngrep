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
#include "config.h"
#include <glib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <glib/gstdio.h>
#include "glib-utils.h"
#include "ncurses/manager.h"
#include "call_list_win.h"
#include "column_select_win.h"

/**
 * @brief Get custom information of given panel
 *
 * Return ncurses users pointer of the given panel into panel's
 * information structure pointer.
 *
 * @param ui UI structure pointer
 * @return a pointer to info structure of given panel
 */
static ColumnSelectWinInfo *
column_select_info(Window *ui)
{
    return (ColumnSelectWinInfo*) panel_userptr(ui->panel);
}

/**
 * @brief Move a item to a new position
 *
 * This function can be used to reorder the column list
 *
 * @param ui UI structure pointer
 * @param item Menu item to be moved
 * @param post New position in the menu
 */
static void
column_select_move_item(Window *ui, ITEM *item, int pos)
{
    // Get panel information
    ColumnSelectWinInfo *info = column_select_info(ui);

    // Check we have a valid position
    if (pos == item_count(info->menu) || pos < 0)
        return;

    // Swap position with destination
    int item_pos = item_index(item);
    info->items[item_pos] = info->items[pos];
    info->items[pos] = item;
    set_menu_items(info->menu, info->items);
}

/**
 * @brief Select/Deselect a menu item
 *
 * This function can be used to toggle selection status of
 * the menu item
 *
 * @param ui UI structure pointer
 * @param item Menu item to be (de)selected
 */
static void
column_select_toggle_item(Window *ui, ITEM *item)
{
    // Get panel information
    ColumnSelectWinInfo *info = column_select_info(ui);

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

/**
 * @brief Update menu after a change
 *
 * After moving an item or updating its selectioactivn status
 * menu must be redrawn.
 *
 * @param ui UI structure pointer
 */
static void
column_select_update_menu(Window *ui)
{
    // Get panel information
    ColumnSelectWinInfo *info = column_select_info(ui);
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
    menu_driver(info->menu, REQ_DOWN_ITEM);
    menu_driver(info->menu, REQ_UP_ITEM);
}

/**
 * @brief Update Call List columns
 *
 * This function will update the columns of Call List
 *
 * @param ui UI structure pointer
 */
static void
column_select_update_columns(Window *ui)
{
    int column, attr_id;

    // Get panel information
    ColumnSelectWinInfo *info = column_select_info(ui);
    g_return_if_fail(info != NULL);

    // Reset column count
    g_ptr_array_remove_all(info->selected);

    // Add all selected columns
    for (column = 0; column < item_count(info->menu); column++) {
        // If column is active
        if (!strncmp(item_name(info->items[column]), "[ ]", 3))
            continue;

        // Get column attribute
        attr_id = attr_find_by_name(item_userptr(info->items[column]));

        // Add a new column to the list
        CallListColumn *column = g_malloc0(sizeof(CallListColumn));
        column->id = attr_id;
        column->attr = attr_name(attr_id);
        column->title = attr_title(attr_id);
        column->width = (guint) attr_width(attr_id);
        g_ptr_array_add(info->selected, column);
    }
}

/**
 * @brief Save selected columns to user config file
 *
 * Remove previously configurated columns from user's
 * $SNGREPRC or $HOME/.sngreprc and add new ones
 *
 * @param ui UI structure pointer
 */
static void
column_select_save_columns(Window *ui)
{
    GString *userconf = g_string_new(NULL);
    GString *tmpfile  = g_string_new(NULL);

    // Use $SNGREPRC/.sngreprc file
    const gchar *rcfile = g_getenv("SNGREPRC");
    if (rcfile != NULL) {
        g_string_append(userconf, rcfile);
    } else {
        // Or Use $HOME/.sngreprc file
        rcfile = g_getenv("HOME");
        if (rcfile != NULL) {
            g_string_append_printf(userconf, "%s/.sngreprc", rcfile);
        }
    }

    // No user configuration found!
    if (userconf->len == 0) return;

    // Path for backup file
    g_string_append_printf(tmpfile, "%s.old", userconf->str);

    // Remove old config file
    if (g_file_test(tmpfile->str, G_FILE_TEST_IS_REGULAR)) {
        g_unlink(tmpfile->str);
    }

    // Move user conf file to temporal file
    g_rename(userconf->str, tmpfile->str);

    // Create a new user conf file
    FILE *fo = g_fopen(userconf->str, "w");
    if (fo == NULL) {
        dialog_run("Unable to open %s: %s", userconf->str, g_strerror(errno));
        g_string_free(userconf, TRUE);
        g_string_free(tmpfile, TRUE);
        return;
    }

    // Check if there was configuration already
    if (g_file_test(tmpfile->str, G_FILE_TEST_IS_REGULAR)) {
        // Get old configuration contents
        gchar *usercontents = NULL;
        g_file_get_contents(tmpfile->str, &usercontents, NULL, NULL);

        // Separate configuration in lines
        gchar **contents = g_strsplit(usercontents, "\n", -1);

        // Put exiting config in the new file
        for (guint i = 0; i < g_strv_length(contents); i++) {
            if (g_ascii_strncasecmp(contents[i], "set cl.column", 13) != 0) {
                g_fprintf(fo, "%s\n", contents[i]);
            }
        }

        g_strfreev(contents);
        g_free(usercontents);
    }

    // Get panel information
    ColumnSelectWinInfo *info = column_select_info(ui);

    // Add all selected columns
    for (gint i = 0; i < item_count(info->menu); i++) {
        // If column is active
        if (!strncmp(item_name(info->items[i]), "[ ]", 3))
            continue;

        // Add the columns settings
        g_fprintf(fo, "set cl.column%d %s\n", i, (const char*) item_userptr(info->items[i]));
    }

    fclose(fo);

    // Show a information dialog
    dialog_run("Column layout successfully saved to %s", userconf->str);

    g_string_free(userconf, TRUE);
    g_string_free(tmpfile, TRUE);
}

/**
 * @brief Manage pressed keys for column selection panel
 *
 * This function will handle keys when menu is active.
 * You can switch between menu and rest of the components
 * using TAB
 *
 * @param ui UI structure pointer
 * @param key   key code
 * @return enum @key_handler_ret
 */
static int
column_select_handle_key_menu(Window *ui, int key)
{
    ITEM *current;
    int current_idx;

    // Get panel information
    ColumnSelectWinInfo *info = column_select_info(ui);
    g_return_val_if_fail(info != NULL, KEY_DESTROY);

    current = current_item(info->menu);
    current_idx = item_index(current);

    // Check actions for this key
    enum KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch (action) {
            case ACTION_DOWN:
                menu_driver(info->menu, REQ_DOWN_ITEM);
                break;
            case ACTION_UP:
                menu_driver(info->menu, REQ_UP_ITEM);
                break;
            case ACTION_NPAGE:
                menu_driver(info->menu, REQ_SCR_DPAGE);
                break;
            case ACTION_PPAGE:
                menu_driver(info->menu, REQ_SCR_UPAGE);
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
                set_menu_fore(info->menu, COLOR_PAIR(CP_DEFAULT));
                set_field_back(info->fields[FLD_COLUMNS_ACCEPT], A_REVERSE);
                form_driver(info->form, REQ_VALIDATION);
                break;
            case ACTION_CONFIRM:
                column_select_update_columns(ui);
                return KEY_DESTROY;
            default:
                // Parse next action
                continue;
        }

        // This panel has handled the key successfully
        break;
    }

    // Draw a scrollbar to the right
    info->scroll.pos = (guint) top_row(info->menu);
    scrollbar_draw(info->scroll);
    wnoutrefresh(info->menu_win);

    // Return if this panel has handled or not the key
    return (action == ERR) ? KEY_NOT_HANDLED : KEY_HANDLED;
}


/**
 * @brief Manage pressed keys for column selection panel
 *
 * This function will handle keys when form is active.
 * You can switch between menu and rest of the components
 * using TAB
 *
 * @param ui UI structure pointer
 * @param key   key code
 * @return enum @key_handler_ret
 */
static int
column_select_handle_key_form(Window *ui, int key)
{
    int field_idx, new_field_idx;
    char field_value[48];

    // Get panel information
    ColumnSelectWinInfo *info = column_select_info(ui);
    g_return_val_if_fail(info != NULL, KEY_DESTROY);

    // Get current field id
    field_idx = field_index(current_field(info->form));

    // Get current field value.
    memset(field_value, 0, sizeof(field_value));
    strcpy(field_value, field_buffer(current_field(info->form), 0));
    g_strstrip(field_value);

    // Check actions for this key
    enum KeybindingAction action = ACTION_UNKNOWN;
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
                        return KEY_DESTROY;
                    case FLD_COLUMNS_CANCEL:
                        return KEY_DESTROY;
                    case FLD_COLUMNS_SAVE:
                        column_select_update_columns(ui);
                        column_select_save_columns(ui);
                        return KEY_DESTROY;
                    default:break;
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

/**
 * @brief Manage pressed keys for column selection panel
 *
 * This function is called by UI manager every time a
 * key is pressed. This allow the filter panel to manage
 * its own keys.
 *
 * @param window UI structure pointer
 * @param key   key code
 * @return enum @key_handler_ret
 */
static int
column_select_handle_key(Window *window, int key)
{
    // Get panel information
    ColumnSelectWinInfo *info = column_select_info(window);
    g_return_val_if_fail(info != NULL, KEY_DESTROY);

    if (info->form_active) {
        return column_select_handle_key_form(window, key);
    } else {
        return column_select_handle_key_menu(window, key);
    }
}

void
column_select_win_set_columns(Window *window, GPtrArray *columns)
{
    // Get panel information
    ColumnSelectWinInfo *info = column_select_info(window);
    g_return_if_fail(info != NULL);

    // Set selected column array
    info->selected = columns;

    // Toggle current enabled fields and move them to the top
    for (guint i = 0; i < g_ptr_array_len(info->selected); i++) {
        CallListColumn *column = g_ptr_array_index(info->selected, i);

        for (guint attr_id = 0; attr_id < (guint) item_count(info->menu); attr_id++) {
            if (!strcmp(item_userptr(info->items[attr_id]), column->attr)) {
                column_select_toggle_item(window, info->items[attr_id]);
                column_select_update_menu(window);
                column_select_move_item(window, info->items[attr_id], i);
                column_select_update_menu(window);
                break;
            }
        }
    }


}

/**
 * @brief Destroy column selection panel
 *
 * This function do the final cleanups for this panel
 *
 * @param ui UI structure pointer
 */
void
column_select_free(Window *ui)
{
    int i;
    ColumnSelectWinInfo *info = column_select_info(ui);

    // Remove menu and items
    unpost_menu(info->menu);
    free_menu(info->menu);
    for (i = 0; i < ATTR_COUNT; i++)
        free_item(info->items[i]);

    // Remove form and fields
    unpost_form(info->form);
    free_form(info->form);
    for (i = 0; i < FLD_COLUMNS_COUNT; i++)
        free_field(info->fields[i]);

    g_free(info);

    // Remove panel window and custom info
    window_deinit(ui);
}

Window *
column_select_win_new()
{
    Window *window = g_malloc0(sizeof(Window));
    window->type = WINDOW_COLUMN_SELECT;
    window->handle_key = column_select_handle_key;
    window->destroy = column_select_free;

    // Cerate a new indow for the panel and form
    window_init(window, 20, 60);

    // Initialize Filter panel specific data
    ColumnSelectWinInfo *info = g_malloc0(sizeof(ColumnSelectWinInfo));
    set_panel_userptr(window->panel, (void*) info);

    // Initialize attributes
    info->form_active = FALSE;

    // Initialize the fields
    info->fields[FLD_COLUMNS_ACCEPT] = new_field(1, 10, window->height - 2, 13, 0, 0);
    info->fields[FLD_COLUMNS_SAVE]   = new_field(1, 10, window->height - 2, 25, 0, 0);
    info->fields[FLD_COLUMNS_CANCEL] = new_field(1, 10, window->height - 2, 37, 0, 0);
    info->fields[FLD_COLUMNS_COUNT]  = NULL;

    // Field Labels
    set_field_buffer(info->fields[FLD_COLUMNS_ACCEPT], 0, "[ Accept ]");
    set_field_buffer(info->fields[FLD_COLUMNS_SAVE],   0, "[  Save  ]");
    set_field_buffer(info->fields[FLD_COLUMNS_CANCEL], 0, "[ Cancel ]");

    // Create the form and post it
    info->form = new_form(info->fields);
    set_form_sub(info->form, window->win);
    post_form(info->form);

    // Create a subwin for the menu area
    info->menu_win = derwin(window->win, 10, window->width - 2, 7, 0);

    // Initialize one field for each attribute
    for (guint attr_id = 0; attr_id < ATTR_COUNT; attr_id++) {
        // Create a new field for this column
        info->items[attr_id] = new_item("[ ]", attr_description(attr_id));
        set_item_userptr(info->items[attr_id], (void*) attr_name(attr_id));
    }
    info->items[ATTR_COUNT] = NULL;

    // Create the columns menu and post it
    info->menu = new_menu(info->items);

    // Set main window and sub window
    set_menu_win(info->menu, window->win);
    set_menu_sub(info->menu, derwin(window->win, 10, window->width - 5, 7, 2));
    set_menu_format(info->menu, 10, 1);
    set_menu_mark(info->menu, "");
    set_menu_fore(info->menu, COLOR_PAIR(CP_DEF_ON_BLUE));
    menu_opts_off(info->menu, O_ONEVALUE);
    post_menu(info->menu);

    // Draw a scrollbar to the right
    info->scroll = window_set_scrollbar(info->menu_win, SB_VERTICAL, SB_RIGHT);
    info->scroll.max = (guint) (item_count(info->menu) - 1);
    scrollbar_draw(info->scroll);

    // Set the window title and boxes
    mvwprintw(window->win, 1, window->width / 2 - 14, "Call List columns selection");
    wattron(window->win, COLOR_PAIR(CP_BLUE_ON_DEF));
    title_foot_box(window->panel);
    mvwhline(window->win, 6, 1, ACS_HLINE, window->width - 1);
    mvwaddch(window->win, 6, 0, ACS_LTEE);
    mvwaddch(window->win, 6, window->width - 1, ACS_RTEE);
    wattroff(window->win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Some brief explanation abotu what window shows
    wattron(window->win, COLOR_PAIR(CP_CYAN_ON_DEF));
    mvwprintw(window->win, 3, 2, "This windows show the list of columns displayed on Call");
    mvwprintw(window->win, 4, 2, "List. You can enable/disable using Space Bar and reorder");
    mvwprintw(window->win, 5, 2, "them using + and - keys.");
    wattroff(window->win, COLOR_PAIR(CP_CYAN_ON_DEF));

    return window;
}
