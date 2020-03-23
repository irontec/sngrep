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
 * @file protocol_Select_win.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions to enable/disable used protocols
 *
 */
#include "config.h"
#include <glib.h>
#include <errno.h>
#include <glib/gstdio.h>
#include "glib-extra/glib.h"
#include "ncurses/manager.h"
#include "ncurses/dialog.h"
#include "ncurses/windows/protocol_select_win.h"

G_DEFINE_TYPE(ProtocolSelectWindow, protocol_select_win, NCURSES_TYPE_WINDOW)

/**
 * @brief Select/Deselect a menu item
 *
 * This function can be used to toggle selection status of
 * the menu item
 *
 * @param self Window structure pointer
 * @param item Menu item to be (de)selected
 */
static void
protocol_select_win_toggle_item(ProtocolSelectWindow *self, ITEM *item)
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
protocol_select_win_update_menu(ProtocolSelectWindow *self)
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
 * @brief Save selected protocols to user config file
 *
 * Remove previously configured protocols from user's
 * $SNGREPRC or $HOME/.sngreprc and add new ones
 *
 * @param ui UI structure pointer
 */
static void
protocol_select_win_save_protocols(ProtocolSelectWindow *self)
{
    GString *userconf = g_string_new(NULL);
    GString *tmpfile = g_string_new(NULL);

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
            if (g_ascii_strncasecmp(contents[i], "set capture.packet.", 13) != 0) {
                g_fprintf(fo, "%s\n", contents[i]);
            }
        }

        g_strfreev(contents);
        g_free(usercontents);
    }

    // Add all selected protocols
    for (gint i = 0; i < item_count(self->menu); i++) {
        // If protocol is active
        g_autofree const gchar *proto_name = g_ascii_strdown(
            item_userptr(self->items[i]),
            strlen(item_userptr(self->items[i]))
        );
        g_fprintf(fo, "set capture.packet.%s %s\n",
                  proto_name,
                  (strncmp(item_name(self->items[i]), "[*]", 3) == 0) ? "on" : "off"
        );
    }

    fclose(fo);

    // Show a information dialog
    dialog_run("Protocol configuration successfully saved to %s\nRestart is required to take effect.", userconf->str);

    g_string_free(userconf, TRUE);
    g_string_free(tmpfile, TRUE);
}

/**
 * @brief Manage pressed keys for protocol selection panel
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
protocol_select_win_handle_key_menu(ProtocolSelectWindow *self, gint key)
{
    ITEM *current = current_item(self->menu);

    // Check actions for this key
    enum KeybindingAction action = ACTION_UNKNOWN;
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
                protocol_select_win_toggle_item(self, current);
                protocol_select_win_update_menu(self);
                break;
            case ACTION_NEXT_FIELD:
                self->form_active = 1;
                set_menu_fore(self->menu, COLOR_PAIR(CP_DEFAULT));
                set_field_back(self->fields[FLD_PROTOCOLS_SAVE], A_REVERSE);
                form_driver(self->form, REQ_VALIDATION);
                break;
            case ACTION_CONFIRM:
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
 * @brief Manage pressed keys for protocol selection panel
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
protocol_select_win_handle_key_form(ProtocolSelectWindow *self, int key)
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
    enum KeybindingAction action = ACTION_UNKNOWN;
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
                    case FLD_PROTOCOLS_CANCEL:
                        return KEY_DESTROY;
                    case FLD_PROTOCOLS_SAVE:
                        protocol_select_win_save_protocols(self);
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
    set_field_back(self->fields[FLD_PROTOCOLS_SAVE], A_NORMAL);
    set_field_back(self->fields[FLD_PROTOCOLS_CANCEL], A_NORMAL);

    // Get current selected field
    new_field_idx = field_index(current_field(self->form));

    // Swap between menu and form
    if (field_idx == FLD_PROTOCOLS_CANCEL && new_field_idx == FLD_PROTOCOLS_SAVE) {
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
 * @brief Manage pressed keys for protocol selection panel
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
protocol_select_win_handle_key(Window *window, gint key)
{
    // Get panel information
    ProtocolSelectWindow *self = NCURSES_PROTOCOL_SELECT(window);

    if (self->form_active) {
        return protocol_select_win_handle_key_form(self, key);
    } else {
        return protocol_select_win_handle_key_menu(self, key);
    }
}

static void
protocol_select_win_finalize(GObject *object)
{
    ProtocolSelectWindow *self = NCURSES_PROTOCOL_SELECT(object);

    // Remove menu and items
    unpost_menu(self->menu);
    free_menu(self->menu);
    for (guint i = 0; i < PACKET_PROTO_COUNT; i++)
        free_item(self->items[i]);

    // Remove form and fields
    unpost_form(self->form);
    free_form(self->form);
    for (guint i = 0; i < FLD_PROTOCOLS_COUNT; i++)
        free_field(self->fields[i]);

    // Chain-up parent finalize function
    G_OBJECT_CLASS(protocol_select_win_parent_class)->finalize(object);
}

Window *
protocol_select_win_new()
{
    return g_object_new(
        WINDOW_TYPE_PROTOCOL_SELECT,
        "height", 20,
        "width", 60,
        NULL
    );
}

static void
protocol_select_win_constructed(GObject *object)
{
    // Chain-up parent constructed
    G_OBJECT_CLASS(protocol_select_win_parent_class)->constructed(object);

    // Get parent window information
    ProtocolSelectWindow *self = NCURSES_PROTOCOL_SELECT(object);
    Window *parent = NCURSES_WINDOW(self);
    WINDOW *win = window_get_ncurses_window(parent);
    PANEL *panel = window_get_ncurses_panel(parent);

    gint height = window_get_height(parent);
    gint width = window_get_width(parent);

    // Initialize the fields
    self->fields[FLD_PROTOCOLS_SAVE] = new_field(1, 10, height - 2, 15, 0, 0);
    self->fields[FLD_PROTOCOLS_CANCEL] = new_field(1, 10, height - 2, 35, 0, 0);
    self->fields[FLD_PROTOCOLS_COUNT] = NULL;

    // Field Labels
    set_field_buffer(self->fields[FLD_PROTOCOLS_SAVE], 0, "[  Save  ]");
    set_field_buffer(self->fields[FLD_PROTOCOLS_CANCEL], 0, "[ Cancel ]");

    // Create the form and post it
    self->form = new_form(self->fields);
    set_form_sub(self->form, win);
    post_form(self->form);

    // Create a subwin for the menu area
    self->menu_win = derwin(win, 10, width - 2, 7, 0);

    // Initialize one field for each attribute
    gint field_cnt = 0;
    for (guint proto_id = 1; proto_id < PACKET_PROTO_COUNT; proto_id++) {
        // Create a new field for this protocol
        PacketDissector *dissector = storage_find_dissector(proto_id);
        if (dissector == NULL)
            continue;

        const gchar *proto_name = packet_dissector_get_name(dissector);
        if (packet_dissector_enabled(proto_id)) {
            self->items[field_cnt] = new_item("[*]", proto_name);
        } else {
            self->items[field_cnt] = new_item("[ ]", proto_name);
        }
        set_item_userptr(self->items[field_cnt], (gpointer) proto_name);
        field_cnt++;
    }
    self->items[field_cnt] = NULL;

    // Create the protocols menu and post it
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
    mvwprintw(win, 1, width / 2 - 14, "Parser protocol selection");
    wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    title_foot_box(panel);
    mvwhline(win, 6, 1, ACS_HLINE, width - 1);
    mvwaddch(win, 6, 0, ACS_LTEE);
    mvwaddch(win, 6, width - 1, ACS_RTEE);
    wattroff(win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Some brief explanation about what window shows
    wattron(win, COLOR_PAIR(CP_CYAN_ON_DEF));
    mvwprintw(win, 3, 2, "These are the available protocols to parse packets.");
    mvwprintw(win, 4, 2, "Use only required protocols for better performance.");
    mvwprintw(win, 5, 2, "Toggle protocol checkbox using Spacebar");
    wattroff(win, COLOR_PAIR(CP_CYAN_ON_DEF));
}

static void
protocol_select_win_class_init(ProtocolSelectWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = protocol_select_win_constructed;
    object_class->finalize = protocol_select_win_finalize;

    WindowClass *window_class = NCURSES_WINDOW_CLASS(klass);
    window_class->handle_key = protocol_select_win_handle_key;

}

static void
protocol_select_win_init(ProtocolSelectWindow *self)
{
    // Initialize attributes
    window_set_window_type(NCURSES_WINDOW(self), WINDOW_PROTOCOL_SELECT);
    self->form_active = FALSE;
}
