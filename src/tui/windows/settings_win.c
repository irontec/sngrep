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
 * @file settings_win.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_settings.h
 */
#include "config.h"
#include <glib/gstdio.h>
#include <errno.h>
#include <ncurses.h>
#include <form.h>
#include "tui/tui.h"
#include "tui/dialog.h"
#include "tui/windows/settings_win.h"

G_DEFINE_TYPE(SettingsWindow, settings_win, TUI_TYPE_WINDOW)

SettingsWindowCategory categories[] = {
    { CAT_SETTINGS_INTERFACE, "Interface" },
    { CAT_SETTINGS_CAPTURE, "Capture" },
    { CAT_SETTINGS_CALL_FLOW, "Call Flow" },
#ifdef USE_HEP
    { CAT_SETTINGS_HEP_HOMER, "HEP Homer" },
#endif
    { 0, NULL },
};

SettingsWindowEntry entries[] = {
    { CAT_SETTINGS_INTERFACE, FLD_SETTINGS_BACKGROUND, SETTING_TUI_BACKGROUND,
      "Background * .............................." },
    { CAT_SETTINGS_INTERFACE, FLD_SETTINGS_SYNTAX, SETTING_TUI_SYNTAX,
      "SIP message syntax ........................" },
    { CAT_SETTINGS_INTERFACE, FLD_SETTINGS_SYNTAX_TAG, SETTING_TUI_SYNTAX_TAG,
      "SIP tag syntax ............................" },
    { CAT_SETTINGS_INTERFACE, FLD_SETTINGS_SYNTAX_BRANCH, SETTING_TUI_SYNTAX_BRANCH,
      "SIP branch syntax ........................." },
    { CAT_SETTINGS_INTERFACE, FLD_SETTINGS_ALTKEY_HINT, SETTING_TUI_ALTKEY_HINT,
      "Alternative keybinding hints .............." },
    { CAT_SETTINGS_INTERFACE, FLD_SETTINGS_COLORMODE, SETTING_TUI_COLORMODE,
      "Default message color mode ................" },
    { CAT_SETTINGS_INTERFACE, FLD_SETTINGS_EXITPROMPT, SETTING_TUI_EXITPROMPT,
      "Always prompt on quit ....................." },
    { CAT_SETTINGS_INTERFACE, FLD_SETTINGS_DISPLAY_ALIAS, SETTING_TUI_DISPLAY_ALIAS,
      "Replace addresses with alias .............." },
    { CAT_SETTINGS_CAPTURE, FLD_SETTINGS_CAPTURE_LIMIT, SETTING_CAPTURE_LIMIT,
      "Max dialogs * ............................." },
    { CAT_SETTINGS_CAPTURE, FLD_SETTINGS_CAPTURE_DEVICE, SETTING_CAPTURE_PCAP_DEVICE,
      "Capture device * .........................." },
    { CAT_SETTINGS_CAPTURE, FLD_SETTINGS_SIP_NOINCOMPLETE, SETTING_STORAGE_INCOMPLETE_DLG,
      "Capture full transactions ................." },
    { CAT_SETTINGS_CAPTURE, FLD_SETTINGS_SAVEPATH, SETTING_STORAGE_SAVEPATH,
      "Default Save path ........................." },
    { CAT_SETTINGS_CALL_FLOW, FLD_SETTINGS_CF_FORCERAW, SETTING_TUI_CF_FORCERAW,
      "Show message preview panel ................" },
    { CAT_SETTINGS_CALL_FLOW, FLD_SETTINGS_CF_HIGHTLIGHT, SETTING_TUI_CF_HIGHTLIGHT,
      "Selected message highlight ................" },
    { CAT_SETTINGS_CALL_FLOW, FLD_SETTINGS_CF_LOCALHIGHLIGHT, SETTING_TUI_CF_LOCALHIGHLIGHT,
      "Highlight local addresses ................." },
    { CAT_SETTINGS_CALL_FLOW, FLD_SETTINGS_CF_SPLITCACALLID, SETTING_TUI_CF_SPLITCALLID,
      "Merge columns with same address ..........." },
    { CAT_SETTINGS_CALL_FLOW, FLD_SETTINGS_CF_SDPONLY, SETTING_TUI_CF_SDP_INFO,
      "Show SDP information in messages .........." },
    { CAT_SETTINGS_CALL_FLOW, FLD_SETTINGS_CF_DELTA, SETTING_TUI_CF_DELTA,
      "Show delta time between messages .........." },
    { CAT_SETTINGS_CALL_FLOW, FLD_SETTINGS_CF_MEDIA, SETTING_TUI_CF_MEDIA,
      "Show RTP media streams ...................." },
    { CAT_SETTINGS_CALL_FLOW, FLD_SETTINGS_CF_SCROLLSTEP, SETTING_TUI_CF_SCROLLSTEP,
      "Steps for PgUp/PgDown ....................." },
#ifdef USE_HEP
    { CAT_SETTINGS_HEP_HOMER, FLD_SETTINGS_HEP_SEND, SETTING_CAPTURE_HEP_SEND,
      "Send all captured SIP packets ............." },
    { CAT_SETTINGS_HEP_HOMER, FLD_SETTINGS_HEP_SEND_VER, SETTING_CAPTURE_HEP_SEND_VER,
      "Send EEP version .........................." },
    { CAT_SETTINGS_HEP_HOMER, FLD_SETTINGS_HEP_SEND_ADDR, SETTING_CAPTURE_HEP_SEND_ADDR,
      "Send EEP packet address ..................." },
    { CAT_SETTINGS_HEP_HOMER, FLD_SETTINGS_HEP_SEND_PORT, SETTING_CAPTURE_HEP_SEND_PORT,
      "Send EEP packet port ......................" },
    { CAT_SETTINGS_HEP_HOMER, FLD_SETTINGS_HEP_SEND_PASS, SETTING_CAPTURE_HEP_SEND_PASS,
      "EEP send password ........................." },
    { CAT_SETTINGS_HEP_HOMER, FLD_SETTINGS_HEP_SEND_ID, SETTING_CAPTURE_HEP_SEND_ID,
      "EEP send capture id ......................." },
    { CAT_SETTINGS_HEP_HOMER, FLD_SETTINGS_HEP_LISTEN, SETTING_CAPTURE_HEP_LISTEN,
      "Listen for eep packets ...................." },
    { CAT_SETTINGS_HEP_HOMER, FLD_SETTINGS_HEP_LISTEN_VER, SETTING_CAPTURE_HEP_LISTEN_VER,
      "Listen EEP version  ......................." },
    { CAT_SETTINGS_HEP_HOMER, FLD_SETTINGS_HEP_LISTEN_ADDR, SETTING_CAPTURE_HEP_LISTEN_ADDR,
      "Listen EEP packet address ................." },
    { CAT_SETTINGS_HEP_HOMER, FLD_SETTINGS_HEP_LISTEN_PORT, SETTING_CAPTURE_HEP_LISTEN_PORT,
      "Listen EEP packet port ...................." },
    { CAT_SETTINGS_HEP_HOMER, FLD_SETTINGS_HEP_LISTEN_PASS, SETTING_CAPTURE_HEP_LISTEN_PASS,
      "EEP server password ......................." },
    { CAT_SETTINGS_HEP_HOMER, FLD_SETTINGS_HEP_LISTEN_UUID, SETTING_CAPTURE_HEP_LISTEN_UUID,
      "EEP server expects UUID (Asterisk) ........" },
#endif
    { 0, 0, 0, NULL },
};

/**
 * @brief Draw the settings panel
 *
 * This function will drawn the panel into the screen with
 * current status settings
 *
 * @param ui UI structure pointer
 * @return 0 if the panel has been drawn, -1 otherwise
 */
static gint
settings_win_draw(Window *window)
{
    SettingsWindow *self = TUI_SETTINGS(window);
    WINDOW *win = window_get_ncurses_window(window);

    // Store cursor position
    gint cury, curx;
    getyx(win, cury, curx);

    // Print category headers
    gint colpos = 2;
    for (gint i = 0; categories[i].cat_id; i++) {
        if (categories[i].cat_id == self->active_category) {
            mvwprintw(win, 6, colpos, "%c %s %c", '[', categories[i].title, ']');
        } else {
            wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
            mvwprintw(win, 6, colpos, "%c %s %c", '[', categories[i].title, ']');
            wattroff(win, COLOR_PAIR(CP_BLUE_ON_DEF));
        }
        colpos += strlen(categories[i].title) + 5;
    }

    // Reset all field background
    for (gint i = 0; i < FLD_SETTINGS_COUNT; i++) {
        set_field_fore(self->fields[i + 1], A_NORMAL);
        if (!strncmp(field_buffer(self->fields[i], 0), "on", 2))
            set_field_fore(self->fields[i], COLOR_PAIR(CP_GREEN_ON_DEF));
        if (!strncmp(field_buffer(self->fields[i], 0), "off", 3))
            set_field_fore(self->fields[i], COLOR_PAIR(CP_RED_ON_DEF));
    }
    for (gint i = 0; i < BTN_SETTINGS_COUNT; i++) {
        set_field_back(self->buttons[i], A_NORMAL);
    }

    // Highlight current field
    if (self->active_form == self->buttons_form) {
        set_field_back(current_field(self->buttons_form), A_REVERSE);
    } else {
        set_field_fore(self->fields[field_index(current_field(self->form)) + 1], A_BOLD);
    }

    touchwin(win);

    // Restore cursor position
    wmove(win, cury, curx);

    return 0;
}

/**
 * @brief Return entry information of the field
 *
 * If field is storing a setting value, return the entry
 * structure associated to the setting
 *
 * @param field Ncurses field pointer of screen
 * @return Setting information structure
 */
static SettingsWindowEntry *
settings_is_entry(FIELD *field)
{
    return (SettingsWindowEntry *) field_userptr(field);
}

/**
 * @brief Update settings with panel values
 *
 * Update all settings with the selected on screen.
 * Note that some settings require application restart to
 * take effect.
 *
 * @param ui UI structure pointer
 * @return 0 in all cases
 */
static gint
settings_update_settings(Window *window)
{
    SettingsWindow *self = TUI_SETTINGS(window);

    gchar field_value[180];

    for (gint i = 0; i < FLD_SETTINGS_COUNT; i++) {
        SettingsWindowEntry *entry = settings_is_entry(self->fields[i]);
        if (entry) {
            // Get field value.
            memset(field_value, 0, sizeof(field_value));
            strcpy(field_value, field_buffer(self->fields[i], 0));
            g_strstrip(field_value);
            // Change setting value
            setting_set_value(entry->setting_id, field_value);
        }
    }

    return 0;
}

/**
 * @brief Update user resource file with panel values
 *
 * Save all settings into user configuration file located
 * in it's home directory.
 *
 * @param ui UI structure pointer
 */
static void
settings_win_save(Window *window)
{
    SettingsWindow *self = TUI_SETTINGS(window);

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
    if (userconf->len == 0) {
        dialog_run("Unable to save configuration. User has no $SNGREPRC or $HOME dir.");
        g_string_free(userconf, TRUE);
        g_string_free(tmpfile, TRUE);
        return;
    }

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
            if (g_ascii_strncasecmp(contents[i], "set cl.column", 13) == 0) {
                g_fprintf(fo, "%s\n", contents[i]);
            }
        }

        g_strfreev(contents);
        g_free(usercontents);
    }

    for (guint i = 0; i < FLD_SETTINGS_COUNT; i++) {
        SettingsWindowEntry *entry = settings_is_entry(self->fields[i]);
        if (entry != NULL) {
            // Change setting value
            gchar *field_value = g_strchomp(g_strdup(field_buffer(self->fields[i], 0)));
            g_fprintf(fo, "set %s %s\n", entry->setting_id, field_value);
            g_free(field_value);

        }
    }

    fclose(fo);

    dialog_run("Settings successfully saved to %s", userconf->str);

    g_string_free(userconf, TRUE);
    g_string_free(tmpfile, TRUE);
}

/**
 * @brief Manage pressed keys for settings panel
 *
 * This function is called by UI manager every time a
 * key is pressed. This allow the filter panel to manage
 * its own keys.
 *
 * @param ui UI structure pointer
 * @param key   key code
 * @return enum @key_handler_ret
 */
static gint
settings_win_handle_key(Window *window, gint key)
{
    int field_idx;
    SettingsWindowEntry *entry;
    GType setting_type = -1;

    // Get panel information
    SettingsWindow *self = TUI_SETTINGS(window);

    // Get current field id
    field_idx = field_index(current_field(self->active_form));

    // Get current setting id;
    if ((entry = settings_is_entry(current_field(self->active_form)))) {
        setting_type = setting_get_type(entry->setting_id);
    }

    // Check actions for this key
    KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ERR) {
        if (self->active_form == self->form) {
            // Check if we handle this action
            switch (action) {
                case ACTION_PRINTABLE:
                    if (setting_type == G_TYPE_INT || setting_type == G_TYPE_STRING) {
                        form_driver(self->form, key);
                        break;
                    }
                    continue;
                case ACTION_UP:
                case ACTION_HPPAGE:
                    form_driver(self->form, REQ_PREV_FIELD);
                    form_driver(self->form, REQ_END_LINE);
                    break;
                case ACTION_DOWN:
                case ACTION_HNPAGE:
                    form_driver(self->form, REQ_NEXT_FIELD);
                    form_driver(self->form, REQ_END_LINE);
                    break;
                case ACTION_SELECT:
                case ACTION_RIGHT:
                    form_driver(self->form, REQ_NEXT_CHOICE);
                    form_driver(self->form, REQ_RIGHT_CHAR);
                    break;
                case ACTION_LEFT:
                    form_driver(self->form, REQ_PREV_CHOICE);
                    form_driver(self->form, REQ_LEFT_CHAR);
                    break;
                case ACTION_NPAGE:
                    form_driver(self->form, REQ_NEXT_PAGE);
                    form_driver(self->form, REQ_END_LINE);
                    self->active_category = form_page(self->form) + 1;
                    break;
                case ACTION_PPAGE:
                    form_driver(self->form, REQ_PREV_PAGE);
                    form_driver(self->form, REQ_END_LINE);
                    self->active_category = form_page(self->form) + 1;
                    break;
                case ACTION_BEGIN:
                    form_driver(self->form, REQ_BEG_LINE);
                    break;
                case ACTION_END:
                    form_driver(self->form, REQ_END_LINE);
                    break;
                case ACTION_NEXT_FIELD:
                    self->active_form = self->buttons_form;
                    set_current_field(self->active_form, self->buttons[BTN_SETTINGS_ACCEPT]);
                    break;
                case ACTION_CLEAR:
                    if (setting_type == G_TYPE_INT || setting_type == G_TYPE_STRING) {
                        form_driver(self->form, REQ_BEG_LINE);
                        form_driver(self->form, REQ_CLR_EOL);
                    }
                    break;
                case ACTION_DELETE:
                    if (setting_type == G_TYPE_INT || setting_type == G_TYPE_STRING) {
                        form_driver(self->form, REQ_DEL_CHAR);
                    }
                    break;
                case ACTION_BACKSPACE:
                    if (setting_type == G_TYPE_INT || setting_type == G_TYPE_STRING) {
                        form_driver(self->form, REQ_DEL_PREV);
                    }
                    break;
                case ACTION_CONFIRM:
                    settings_update_settings(window);
                    return KEY_DESTROY;
                default:
                    // Parse next action
                    continue;
            }
        } else {
            // Check if we handle this action
            switch (action) {
                case ACTION_RIGHT:
                case ACTION_DOWN:
                case ACTION_NEXT_FIELD:
                    if (field_idx == BTN_SETTINGS_CANCEL) {
                        self->active_form = self->form;
                    } else {
                        form_driver(self->buttons_form, REQ_NEXT_FIELD);
                    }
                    break;
                case ACTION_LEFT:
                case ACTION_UP:
                case ACTION_PREV_FIELD:
                    if (field_idx == BTN_SETTINGS_ACCEPT) {
                        self->active_form = self->form;
                    } else {
                        form_driver(self->buttons_form, REQ_PREV_FIELD);
                    }
                    break;
                case ACTION_SELECT:
                case ACTION_CONFIRM:
                    if (field_idx == BTN_SETTINGS_SAVE)
                        settings_win_save(window);
                    settings_update_settings(window);
                    return KEY_DESTROY;
                default:
                    continue;
            }
        }

        // This panel has handled the key successfully
        break;
    }

    // Validate all input data
    form_driver(self->active_form, REQ_VALIDATION);

    // Get current setting id
    if ((entry = settings_is_entry(current_field(self->active_form)))) {
        // Enable cursor on string and number fields
        curs_set(
            setting_get_type(entry->setting_id) == G_TYPE_INT
            || setting_get_type(entry->setting_id) == G_TYPE_STRING
        );
    } else {
        curs_set(0);
    }

    // Return if this panel has handled or not the key
    return (action == ERR) ? KEY_NOT_HANDLED : KEY_HANDLED;
}

void
settings_win_free(Window *window)
{
    g_object_unref(window);
}

Window *
settings_win_new()
{
    return g_object_new(
        WINDOW_TYPE_SETTINGS,
        "height", 24,
        "width", 70,
        NULL
    );
}

static void
settings_win_finalize(G_GNUC_UNUSED GObject *object)
{
    // Hide cursor
    curs_set(0);

    // Chain-up parent finalize
    G_OBJECT_CLASS(settings_win_parent_class)->finalize(object);
}

static void
settings_win_constructed(GObject *object)
{
    // Chain-up parent constructed
    G_OBJECT_CLASS(settings_win_parent_class)->constructed(object);

    gint line;
    FIELD *entry = NULL, *label;
    gint field = 0;

    // Get parent window information
    SettingsWindow *self = TUI_SETTINGS(object);
    Window *parent = TUI_WINDOW(self);
    WINDOW *win = window_get_ncurses_window(parent);
    PANEL *panel = window_get_ncurses_panel(parent);

    gint height = window_get_height(parent);
    gint width = window_get_width(parent);

    // Create a scrollable subwindow for settings
    self->form_win = derwin(win, height - 11, width - 2, 8, 1);

    // Configure panel buttons
    self->buttons[BTN_SETTINGS_ACCEPT] = new_field(1, 10, height - 2, 12, 0, 0);
    self->buttons[BTN_SETTINGS_SAVE] = new_field(1, 10, height - 2, 29, 0, 0);
    self->buttons[BTN_SETTINGS_CANCEL] = new_field(1, 10, height - 2, 46, 0, 0);
    self->buttons[BTN_SETTINGS_COUNT] = NULL;
    field_opts_off(self->buttons[BTN_SETTINGS_ACCEPT], O_EDIT);
    field_opts_off(self->buttons[BTN_SETTINGS_SAVE], O_EDIT);
    field_opts_off(self->buttons[BTN_SETTINGS_CANCEL], O_EDIT);
    set_field_buffer(self->buttons[BTN_SETTINGS_ACCEPT], 0, "[ Accept ]");
    set_field_buffer(self->buttons[BTN_SETTINGS_SAVE], 0, "[  Save  ]");
    set_field_buffer(self->buttons[BTN_SETTINGS_CANCEL], 0, "[ Cancel ]");
    self->buttons_form = new_form(self->buttons);
    set_form_sub(self->buttons_form, win);
    post_form(self->buttons_form);

    // Initialize rest of settings fields
    for (gint i = 0; categories[i].cat_id; i++) {
        // Each category section begins with fields in the first line
        line = 0;

        for (gint j = 0; entries[j].cat_id; j++) {
            // Ignore entries of other categories
            if (entries[j].cat_id != categories[i].cat_id)
                continue;

            // Create the label
            label = new_field(1, 45, line, 3, 0, 0);
            set_field_buffer(label, 0, entries[j].label);
            field_opts_off(label, O_ACTIVE);

            // Change field properties according to field type
            switch (setting_get_type(entries[j].setting_id)) {
                case G_TYPE_INT:
                    entry = new_field(1, 18, line, 48, 0, 0);
                    set_field_back(entry, A_UNDERLINE);
                    set_field_type(entry, TYPE_REGEXP, "[0-9]+");
                    break;
                case G_TYPE_STRING:
                    entry = new_field(1, 18, line, 48, 0, 0);
                    field_opts_off(entry, O_STATIC);
                    set_field_back(entry, A_UNDERLINE);
                    break;
                case G_TYPE_BOOLEAN:
                    entry = new_field(1, 12, line, 48, 0, 0);
                    field_opts_off(entry, O_EDIT);
                    set_field_type(entry, TYPE_ENUM, g_strsplit("on,off", ",", 0), 0, 0);
                    break;
                case G_TYPE_ENUM:
                    entry = new_field(1, 12, line, 48, 0, 0);
                    field_opts_off(entry, O_EDIT);
                    set_field_type(entry, TYPE_ENUM, setting_valid_values(entries[j].setting_id), 0, 0);
                    break;
                default:
                    g_warning("Unknown setting type for setting %s\n", entries[j].setting_id);
                    break;
            }

            field_opts_off(entry, O_AUTOSKIP);
            set_field_buffer(entry, 0, setting_get_value(entries[j].setting_id));
            set_field_userptr(entry, (void *) &entries[j]);

            if (line == 0) {
                // Set last field as page breaker
                set_new_page(entry, TRUE);
            }

            // Store field
            self->fields[field++] = entry;
            self->fields[field++] = label;

            line++;
        }
    }

    // Create the form and post it
    self->form = new_form(self->fields);
    set_form_sub(self->form, self->form_win);
    post_form(self->form);

    // Set the window title and boxes
    mvwprintw(win, 1, width / 2 - 5, "Settings");
    wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    title_foot_box(panel);
    mvwhline(win, 6, 1, ACS_HLINE, width - 1);
    mvwaddch(win, 6, 0, ACS_LTEE);
    mvwaddch(win, 6, width - 1, ACS_RTEE);
    wattroff(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    wattron(win, COLOR_PAIR(CP_CYAN_ON_DEF));
    mvwprintw(win, 3, 1, " Use arrow keys, PgUp, PgDown and Tab to move around settings.");
    mvwprintw(win, 4, 1, " Settings with (*) requires restart.");
    wattroff(win, COLOR_PAIR(CP_CYAN_ON_DEF));

    // Set default field
    self->active_form = self->form;
    set_current_field(self->form, *self->fields);
    self->active_category = form_page(self->form) + 1;
}

static void
settings_win_class_init(SettingsWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = settings_win_constructed;
    object_class->finalize = settings_win_finalize;

    WindowClass *window_class = TUI_WINDOW_CLASS(klass);
    window_class->draw = settings_win_draw;
    window_class->handle_key = settings_win_handle_key;

}

static void
settings_win_init(SettingsWindow *self)
{
    // Initialize attributes
    window_set_window_type(TUI_WINDOW(self), WINDOW_SETTINGS);
}
