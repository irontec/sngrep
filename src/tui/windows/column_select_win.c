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
 * @file column_select_win.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_column_select.h
 *
 */
#include "config.h"
#include <glib.h>
#include <errno.h>
#include <glib/gstdio.h>
#include "glib-extra/glib.h"
#include "tui/tui.h"
#include "tui/dialog.h"
#include "tui/windows/call_list_win.h"
#include "tui/windows/column_select_win.h"

G_DEFINE_TYPE(ColumnSelectWindow, column_select, TUI_TYPE_WINDOW)

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
column_select_move_item(ColumnSelectWindow *self, ITEM *item, gint pos)
{
    // Check we have a valid position
    if (pos == item_count(self->menu) || pos < 0)
        return;

    // Swap position with destination
    int item_pos = item_index(item);
    self->items[item_pos] = self->items[pos];
    self->items[pos] = item;
    set_menu_items(self->menu, self->items);
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
column_select_toggle_item(ColumnSelectWindow *self, ITEM *item)
{
    int pos = item_index(item);

    // Change item name
    if (!strncmp(item_name(item), "[ ]", 3)) {
        self->items[pos] = new_item("[*]", item_description(item));
    } else {
        self->items[pos] = new_item("[ ]", item_description(item));
    }

    // Restore menu item
    set_item_userptr(self->items[pos], item_userptr(item));
    set_menu_items(self->menu, self->items);

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
column_select_update_menu(ColumnSelectWindow *self)
{
    // Get panel information
    ITEM *current = current_item(self->menu);
    int top_idx = top_row(self->menu);

    // Remove the menu from the subwindow
    unpost_menu(self->menu);
    // Set menu items
    set_menu_items(self->menu, self->items);
    // Put the menu again into its subwindow
    post_menu(self->menu);

    // Move until the current position is set
    set_top_row(self->menu, top_idx);
    set_current_item(self->menu, current);

    // Force menu redraw
    menu_driver(self->menu, REQ_DOWN_ITEM);
    menu_driver(self->menu, REQ_UP_ITEM);
}

/**
 * @brief Update Call List columns
 *
 * This function will update the columns of Call List
 *
 * @param ui UI structure pointer
 */
static void
column_select_update_columns(ColumnSelectWindow *self)
{
    // Reset column count
    g_ptr_array_remove_all(self->selected);

    // Add all selected columns
    for (gint i = 0; i < item_count(self->menu); i++) {
        // If column is active
        if (!strncmp(item_name(self->items[i]), "[ ]", 3))
            continue;

        // Get column attribute
        Attribute *attr = attribute_find_by_name(item_userptr(self->items[i]));

        // Add a new column to the list
        CallListColumn *column = g_malloc0(sizeof(CallListColumn));
        column->attr = attr;
        column->name = attribute_get_name(attr);
        column->title = attribute_get_title(attr);
        column->width = attribute_get_length(attr);
        g_ptr_array_add(self->selected, column);
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
column_select_save_columns(ColumnSelectWindow *self)
{
    // Calculate Columns setting string
    g_autoptr(GString) columns = g_string_new(NULL);
    for (gint i = 0; i < item_count(self->menu); i++) {
        if (g_ascii_strncasecmp(item_name(self->items[i]), "[*]", 3) == 0) {
            if (columns->len) {
                g_string_append_printf(columns, ",%s", (const gchar *) item_userptr(self->items[i]));
            } else {
                g_string_append(columns, (const gchar *) item_userptr(self->items[i]));
            }
        }
    }

    // Use $SNGREPRC/.sngreprc file
    g_autoptr(GString) userconf = g_string_new(NULL);
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
    g_autoptr(GString) tmpfile = g_string_new(NULL);
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
        return;
    }

    // Check if there was configuration already
    if (g_file_test(tmpfile->str, G_FILE_TEST_IS_REGULAR)) {
        // Get old configuration contents
        g_autofree gchar *usercontents = NULL;
        g_file_get_contents(tmpfile->str, &usercontents, NULL, NULL);

        // Separate configuration in lines
        g_auto(GStrv) contents = g_strsplit(usercontents, "\n", -1);

        // Put exiting config in the new file
        for (guint i = 0; i < g_strv_length(contents); i++) {
            if (g_ascii_strncasecmp(contents[i], "set cl.columns", 14) != 0) {
                g_fprintf(fo, "%s\n", contents[i]);
            }
        }
        g_fprintf(fo, "set %s %s\n", "cl.columns", columns->str);
    }
    fclose(fo);

    // Show a information dialog
    dialog_run("Column layout successfully saved to %s", userconf->str);
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
static gint
column_select_handle_key_menu(ColumnSelectWindow *self, gint key)
{
    ITEM *current = current_item(self->menu);
    gint current_idx = item_index(current);

    // Check actions for this key
    KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch (action) {
            case ACTION_DOWN:
                menu_driver(self->menu, REQ_DOWN_ITEM);
                break;
            case ACTION_UP:
                menu_driver(self->menu, REQ_UP_ITEM);
                break;
            case ACTION_NPAGE:
                menu_driver(self->menu, REQ_SCR_DPAGE);
                break;
            case ACTION_PPAGE:
                menu_driver(self->menu, REQ_SCR_UPAGE);
                break;
            case ACTION_SELECT:
                column_select_toggle_item(self, current);
                column_select_update_menu(self);
                break;
            case ACTION_COLUMN_MOVE_DOWN:
                column_select_move_item(self, current, current_idx + 1);
                column_select_update_menu(self);
                break;
            case ACTION_COLUMN_MOVE_UP:
                column_select_move_item(self, current, current_idx - 1);
                column_select_update_menu(self);
                break;
            case ACTION_NEXT_FIELD:
                self->form_active = 1;
                set_menu_fore(self->menu, COLOR_PAIR(CP_DEFAULT));
                set_field_back(self->fields[FLD_COLUMNS_ACCEPT], A_REVERSE);
                form_driver(self->form, REQ_VALIDATION);
                break;
            case ACTION_CONFIRM:
                column_select_update_columns(self);
                return KEY_DESTROY;
            default:
                // Parse next action
                continue;
        }

        // This panel has handled the key successfully
        break;
    }

    // Draw a scrollbar to the right
    self->scroll.pos = (guint) top_row(self->menu);
    scrollbar_draw(self->scroll);
    wnoutrefresh(self->menu_win);

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
static gint
column_select_handle_key_form(ColumnSelectWindow *self, int key)
{
    int field_idx, new_field_idx;
    char field_value[48];

    // Get current field id
    field_idx = field_index(current_field(self->form));

    // Get current field value.
    memset(field_value, 0, sizeof(field_value));
    strcpy(field_value, field_buffer(current_field(self->form), 0));
    g_strstrip(field_value);

    // Check actions for this key
    KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch (action) {
            case ACTION_RIGHT:
            case ACTION_NEXT_FIELD:
                form_driver(self->form, REQ_NEXT_FIELD);
                break;
            case ACTION_LEFT:
            case ACTION_PREV_FIELD:
                form_driver(self->form, REQ_PREV_FIELD);
                break;
            case ACTION_SELECT:
            case ACTION_CONFIRM:
                switch (field_idx) {
                    case FLD_COLUMNS_ACCEPT:
                        column_select_update_columns(self);
                        return KEY_DESTROY;
                    case FLD_COLUMNS_CANCEL:
                        return KEY_DESTROY;
                    case FLD_COLUMNS_SAVE:
                        column_select_update_columns(self);
                        column_select_save_columns(self);
                        return KEY_DESTROY;
                    default:
                        break;
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
    form_driver(self->form, REQ_VALIDATION);

    // Change background and cursor of "button fields"
    set_field_back(self->fields[FLD_COLUMNS_ACCEPT], A_NORMAL);
    set_field_back(self->fields[FLD_COLUMNS_SAVE], A_NORMAL);
    set_field_back(self->fields[FLD_COLUMNS_CANCEL], A_NORMAL);

    // Get current selected field
    new_field_idx = field_index(current_field(self->form));

    // Swap between menu and form
    if (field_idx == FLD_COLUMNS_CANCEL && new_field_idx == FLD_COLUMNS_ACCEPT) {
        set_menu_fore(self->menu, COLOR_PAIR(CP_DEF_ON_BLUE));
        self->form_active = 0;
    } else {
        // Change current field background
        set_field_back(self->fields[new_field_idx], A_REVERSE);
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
static gint
column_select_handle_key(Window *window, gint key)
{
    // Get panel information
    ColumnSelectWindow *self = TUI_COLUMN_SELECT(window);

    if (self->form_active) {
        return column_select_handle_key_form(self, key);
    } else {
        return column_select_handle_key_menu(self, key);
    }
}

void
column_select_win_set_columns(Window *window, GPtrArray *columns)
{
    ColumnSelectWindow *self = TUI_COLUMN_SELECT(window);

    // Set selected column array
    self->selected = columns;

    // Toggle current enabled fields and move them to the top
    for (guint i = 0; i < g_ptr_array_len(self->selected); i++) {
        CallListColumn *column = g_ptr_array_index(self->selected, i);

        for (guint attr_id = 0; attr_id < (guint) item_count(self->menu); attr_id++) {
            if (!strcmp(item_userptr(self->items[attr_id]), column->name)) {
                column_select_toggle_item(self, self->items[attr_id]);
                column_select_update_menu(self);
                column_select_move_item(self, self->items[attr_id], i);
                column_select_update_menu(self);
                break;
            }
        }
    }
}

static void
column_select_finalize(GObject *object)
{
    ColumnSelectWindow *self = TUI_COLUMN_SELECT(object);

    guint menu_item_count = item_count(self->menu);

    // Remove menu and items
    unpost_menu(self->menu);
    free_menu(self->menu);
    for (guint i = 0; i < menu_item_count; i++)
        free_item(self->items[i]);

    // Remove form and fields
    unpost_form(self->form);
    free_form(self->form);
    for (guint i = 0; i < FLD_COLUMNS_COUNT; i++)
        free_field(self->fields[i]);

    // Chain-up parent finalize function
    G_OBJECT_CLASS(column_select_parent_class)->finalize(object);
}

Window *
column_select_win_new()
{
    return g_object_new(
        WINDOW_TYPE_COLUMN_SELECT,
        "height", 20,
        "width", 60,
        NULL
    );
}

static void
column_select_constructed(GObject *object)
{
    // Chain-up parent constructed
    G_OBJECT_CLASS(column_select_parent_class)->constructed(object);

    // Get parent window information
    ColumnSelectWindow *self = TUI_COLUMN_SELECT(object);
    Window *parent = TUI_WINDOW(self);
    WINDOW *win = window_get_ncurses_window(parent);
    PANEL *panel = window_get_ncurses_panel(parent);

    gint height = window_get_height(parent);
    gint width = window_get_width(parent);

    // Initialize the fields
    self->fields[FLD_COLUMNS_ACCEPT] = new_field(1, 10, height - 2, 13, 0, 0);
    self->fields[FLD_COLUMNS_SAVE] = new_field(1, 10, height - 2, 25, 0, 0);
    self->fields[FLD_COLUMNS_CANCEL] = new_field(1, 10, height - 2, 37, 0, 0);
    self->fields[FLD_COLUMNS_COUNT] = NULL;

    // Field Labels
    set_field_buffer(self->fields[FLD_COLUMNS_ACCEPT], 0, "[ Accept ]");
    set_field_buffer(self->fields[FLD_COLUMNS_SAVE], 0, "[  Save  ]");
    set_field_buffer(self->fields[FLD_COLUMNS_CANCEL], 0, "[ Cancel ]");

    // Create the form and post it
    self->form = new_form(self->fields);
    set_form_sub(self->form, win);
    post_form(self->form);

    // Create a subwin for the menu area
    self->menu_win = derwin(win, 10, width - 2, 7, 0);

    // Allocate memory for items
    GPtrArray *attributes = attribute_get_internal_array();
    guint attr_count = g_ptr_array_len(attributes);
    self->items = g_malloc0_n(attr_count + 1, sizeof(ITEM *));

    // Initialize one field for each attribute
    for (guint i = 0; i < attr_count; i++) {
        Attribute *attr = g_ptr_array_index(attributes, i);
        // Create a new field for this column
        self->items[i] = new_item("[ ]", attribute_get_description(attr));
        set_item_userptr(self->items[i], (void *) attribute_get_name(attr));
    }
    self->items[attr_count] = NULL;

    // Create the columns menu and post it
    self->menu = new_menu(self->items);

    // Set main window and sub window
    set_menu_win(self->menu, win);
    set_menu_sub(self->menu, derwin(win, 10, width - 5, 7, 2));
    set_menu_format(self->menu, 10, 1);
    set_menu_mark(self->menu, "");
    set_menu_fore(self->menu, COLOR_PAIR(CP_DEF_ON_BLUE));
    menu_opts_off(self->menu, O_ONEVALUE);
    post_menu(self->menu);

    // Draw a scrollbar to the right
    self->scroll = window_set_scrollbar(self->menu_win, SB_VERTICAL, SB_RIGHT);
    self->scroll.max = (guint) (item_count(self->menu) - 1);
    scrollbar_draw(self->scroll);

    // Set the window title and boxes
    mvwprintw(win, 1, width / 2 - 14, "Call List columns selection");
    wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    title_foot_box(panel);
    mvwhline(win, 6, 1, ACS_HLINE, width - 1);
    mvwaddch(win, 6, 0, ACS_LTEE);
    mvwaddch(win, 6, width - 1, ACS_RTEE);
    wattroff(win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Some brief explanation about what window shows
    wattron(win, COLOR_PAIR(CP_CYAN_ON_DEF));
    mvwprintw(win, 3, 2, "This windows show the list of columns displayed on Call");
    mvwprintw(win, 4, 2, "List. You can enable/disable using Space Bar and reorder");
    mvwprintw(win, 5, 2, "them using + and - keys.");
    wattroff(win, COLOR_PAIR(CP_CYAN_ON_DEF));
}

static void
column_select_class_init(ColumnSelectWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = column_select_constructed;
    object_class->finalize = column_select_finalize;

    WindowClass *window_class = TUI_WINDOW_CLASS(klass);
    window_class->handle_key = column_select_handle_key;

}

static void
column_select_init(ColumnSelectWindow *self)
{
    // Initialize attributes
    window_set_window_type(TUI_WINDOW(self), WINDOW_COLUMN_SELECT);
    self->form_active = FALSE;
}
