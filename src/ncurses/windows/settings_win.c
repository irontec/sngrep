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
 * @file settings_win.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_settings.h
 */
#include "config.h"
#include <glib/gstdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ncurses/manager.h"
#include "settings_win.h"
#include "setting.h"

SettingsWinCategory categories[] = {
    { CAT_SETTINGS_INTERFACE,  "Interface" },
    { CAT_SETTINGS_CAPTURE,    "Capture" },
    { CAT_SETTINGS_CALL_FLOW,  "Call Flow" },
#ifdef USE_HEP
    { CAT_SETTINGS_HEP_HOMER,  "HEP Homer" },
#endif
    { 0 , NULL },
};

SettingsWinEntry entries[] = {
    { CAT_SETTINGS_INTERFACE,  FLD_SETTINGS_BACKGROUND,         SETTING_BACKGROUND,         "Background * .............................." },
    { CAT_SETTINGS_INTERFACE,  FLD_SETTINGS_SYNTAX,             SETTING_SYNTAX,             "SIP message syntax ........................" },
    { CAT_SETTINGS_INTERFACE,  FLD_SETTINGS_SYNTAX_TAG,         SETTING_SYNTAX_TAG,         "SIP tag syntax ............................" },
    { CAT_SETTINGS_INTERFACE,  FLD_SETTINGS_SYNTAX_BRANCH,      SETTING_SYNTAX_BRANCH,      "SIP branch syntax ........................." },
    { CAT_SETTINGS_INTERFACE,  FLD_SETTINGS_ALTKEY_HINT,        SETTING_ALTKEY_HINT,        "Alternative keybinding hints .............." },
    { CAT_SETTINGS_INTERFACE,  FLD_SETTINGS_COLORMODE,          SETTING_COLORMODE,          "Default message color mode ................" },
    { CAT_SETTINGS_INTERFACE,  FLD_SETTINGS_EXITPROMPT,         SETTING_EXITPROMPT,         "Always prompt on quit ....................." },
    { CAT_SETTINGS_INTERFACE,  FLD_SETTINGS_DISPLAY_ALIAS,      SETTING_DISPLAY_ALIAS,      "Replace addresses with alias .............." },
    { CAT_SETTINGS_CAPTURE,    FLD_SETTINGS_CAPTURE_LIMIT,      SETTING_CAPTURE_LIMIT,      "Max dialogs * ............................." },
    { CAT_SETTINGS_CAPTURE,    FLD_SETTINGS_CAPTURE_DEVICE,     SETTING_CAPTURE_DEVICE,     "Capture device * .........................." },
    { CAT_SETTINGS_CAPTURE,    FLD_SETTINGS_SIP_NOINCOMPLETE,   SETTING_SIP_NOINCOMPLETE,   "Capture full transactions ................." },
    { CAT_SETTINGS_CAPTURE,    FLD_SETTINGS_SAVEPATH,           SETTING_SAVEPATH,           "Default Save path ........................." },
    { CAT_SETTINGS_CALL_FLOW,  FLD_SETTINGS_CF_FORCERAW,        SETTING_CF_FORCERAW,        "Show message preview panel ................" },
    { CAT_SETTINGS_CALL_FLOW,  FLD_SETTINGS_CF_HIGHTLIGHT,      SETTING_CF_HIGHTLIGHT,      "Selected message highlight ................" },
    { CAT_SETTINGS_CALL_FLOW,  FLD_SETTINGS_CF_LOCALHIGHLIGHT,  SETTING_CF_LOCALHIGHLIGHT,  "Highlight local addresses ................." },
    { CAT_SETTINGS_CALL_FLOW,  FLD_SETTINGS_CF_SPLITCACALLID,   SETTING_CF_SPLITCALLID,     "Merge columns with same address ..........." },
    { CAT_SETTINGS_CALL_FLOW,  FLD_SETTINGS_CF_SDPONLY,         SETTING_CF_SDP_INFO,        "Show SDP information in messages .........." },
    { CAT_SETTINGS_CALL_FLOW,  FLD_SETTINGS_CF_DELTA,           SETTING_CF_DELTA,           "Show delta time between messages .........." },
    { CAT_SETTINGS_CALL_FLOW,  FLD_SETTINGS_CF_MEDIA,           SETTING_CF_MEDIA,           "Show RTP media streams ...................." },
    { CAT_SETTINGS_CALL_FLOW,  FLD_SETTINGS_CF_SCROLLSTEP,      SETTING_CF_SCROLLSTEP,      "Steps for PgUp/PgDown ....................." },
#ifdef USE_HEP
    { CAT_SETTINGS_HEP_HOMER,  FLD_SETTINGS_HEP_SEND,           SETTING_HEP_SEND,           "Send all captured SIP packets ............." },
    { CAT_SETTINGS_HEP_HOMER,  FLD_SETTINGS_HEP_SEND_VER,       SETTING_HEP_SEND_VER,       "Send EEP version .........................." },
    { CAT_SETTINGS_HEP_HOMER,  FLD_SETTINGS_HEP_SEND_ADDR,      SETTING_HEP_SEND_ADDR,      "Send EEP packet address ..................." },
    { CAT_SETTINGS_HEP_HOMER,  FLD_SETTINGS_HEP_SEND_PORT,      SETTING_HEP_SEND_PORT,      "Send EEP packet port ......................" },
    { CAT_SETTINGS_HEP_HOMER,  FLD_SETTINGS_HEP_SEND_PASS,      SETTING_HEP_SEND_PASS,      "EEP send password ........................." },
    { CAT_SETTINGS_HEP_HOMER,  FLD_SETTINGS_HEP_SEND_ID,        SETTING_HEP_SEND_ID,        "EEP send capture id ......................." },
    { CAT_SETTINGS_HEP_HOMER,  FLD_SETTINGS_HEP_LISTEN,         SETTING_HEP_LISTEN,         "Listen for eep packets ...................." },
    { CAT_SETTINGS_HEP_HOMER,  FLD_SETTINGS_HEP_LISTEN_VER,     SETTING_HEP_LISTEN_VER,     "Listen EEP version  ......................." },
    { CAT_SETTINGS_HEP_HOMER,  FLD_SETTINGS_HEP_LISTEN_ADDR,    SETTING_HEP_LISTEN_ADDR,    "Listen EEP packet address ................." },
    { CAT_SETTINGS_HEP_HOMER,  FLD_SETTINGS_HEP_LISTEN_PORT,    SETTING_HEP_LISTEN_PORT,    "Listen EEP packet port ...................." },
    { CAT_SETTINGS_HEP_HOMER,  FLD_SETTINGS_HEP_LISTEN_PASS,    SETTING_HEP_LISTEN_PASS,    "EEP server password ......................." },
    { CAT_SETTINGS_HEP_HOMER,  FLD_SETTINGS_HEP_LISTEN_UUID,    SETTING_HEP_LISTEN_UUID,    "EEP server expects UUID (Asterisk) ........" },
#endif
    { 0 , 0, 0, NULL },
};

/**
 * @brief Get custom information of given panel
 *
 * Return ncurses users pointer of the given panel into panel's
 * information structure pointer.
 *
 * @param ui UI structure pointer
 * @return a pointer to info structure of given panel
 */
static SettingsWinInfo *
settings_info(Window *ui)
{
    return (SettingsWinInfo*) panel_userptr(ui->panel);
}

/**
 * @brief Draw the settings panel
 *
 * This function will drawn the panel into the screen with
 * current status settings
 *
 * @param ui UI structure pointer
 * @return 0 if the panel has been drawn, -1 otherwise
 */
static int
settings_draw(Window *ui)
{
    int i;
    int cury, curx;


    // Get panel information
    SettingsWinInfo *info = settings_info(ui);

    // Store cursor position
    getyx(ui->win, cury, curx);

    // Print category headers
    int colpos = 2;
    for (i = 0; categories[i].cat_id; i++) {
        if (categories[i].cat_id == info->active_category) {
            mvwprintw(ui->win, 6, colpos, "%c %s %c", '[', categories[i].title, ']');
        } else {
            wattron(ui->win, COLOR_PAIR(CP_BLUE_ON_DEF));
            mvwprintw(ui->win, 6, colpos, "%c %s %c", '[', categories[i].title, ']');
            wattroff(ui->win, COLOR_PAIR(CP_BLUE_ON_DEF));
        }
        colpos += strlen(categories[i].title) + 5;
    }

    // Reset all field background
    for (i = 0; i < FLD_SETTINGS_COUNT; i++) {
        set_field_fore(info->fields[i + 1], A_NORMAL);
        if (!strncmp(field_buffer(info->fields[i], 0), "on", 2))
            set_field_fore(info->fields[i], COLOR_PAIR(CP_GREEN_ON_DEF));
        if (!strncmp(field_buffer(info->fields[i], 0), "off", 3))
            set_field_fore(info->fields[i], COLOR_PAIR(CP_RED_ON_DEF));
    }
    for (i=0; i < BTN_SETTINGS_COUNT; i++) {
        set_field_back(info->buttons[i], A_NORMAL);
    }

    // Highlight current field
    if (info->active_form == info->buttons_form) {
        set_field_back(current_field(info->buttons_form), A_REVERSE);
    } else {
        set_field_fore(info->fields[field_index(current_field(info->form)) + 1], A_BOLD);
    }

    touchwin(ui->win);

    // Restore cursor position
    wmove(ui->win, cury, curx);

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
static SettingsWinEntry *
settings_is_entry(FIELD *field)
{
    return (SettingsWinEntry *) field_userptr(field);
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
static int
settings_update_settings(Window *ui)
{
    int i;
    char field_value[180];
    SettingsWinEntry *entry;

    // Get panel information
    SettingsWinInfo *info = settings_info(ui);

    for (i=0; i < FLD_SETTINGS_COUNT; i++) {
        if ((entry = settings_is_entry(info->fields[i]))) {
            // Get field value.
            memset(field_value, 0, sizeof(field_value));
            strcpy(field_value, field_buffer(info->fields[i], 0));
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
settings_save(Window *ui)
{
    // Get panel information
    SettingsWinInfo *info = settings_info(ui);

    g_autoptr(GString) userconf = g_string_new(NULL);
    g_autoptr(GString) tmpfile  = g_string_new(NULL);

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
        SettingsWinEntry *entry = settings_is_entry(info->fields[i]);
        if (entry != NULL) {
            // Change setting value
            gchar *field_value = g_strchomp(g_strdup(field_buffer(info->fields[i], 0)));
            g_fprintf(fo, "set %s %s\n", setting_name(entry->setting_id), field_value);
            g_free(field_value);

        }
    }

    fclose(fo);

    dialog_run("Settings successfully saved to %s", userconf->str);
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
static int
settings_handle_key(Window *ui, int key)
{
    int field_idx;
    SettingsWinEntry *entry;
    enum setting_fmt sett_fmt = -1;

    // Get panel information
    SettingsWinInfo *info = settings_info(ui);

    // Get current field id
    field_idx = field_index(current_field(info->active_form));

    // Get current setting id;
    if ((entry = settings_is_entry(current_field(info->active_form)))) {
        sett_fmt = setting_format(entry->setting_id);
    }

    // Check actions for this key
    enum KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ERR) {
        if (info->active_form == info->form) {
            // Check if we handle this action
            switch (action) {
                case ACTION_PRINTABLE:
                    if (sett_fmt == SETTING_FMT_NUMBER || sett_fmt == SETTING_FMT_STRING) {
                        form_driver(info->form, key);
                        break;
                    }
                    continue;
                case ACTION_UP:
                case ACTION_HPPAGE:
                    form_driver(info->form, REQ_PREV_FIELD);
                    form_driver(info->form, REQ_END_LINE);
                    break;
                case ACTION_DOWN:
                case ACTION_HNPAGE:
                    form_driver(info->form, REQ_NEXT_FIELD);
                    form_driver(info->form, REQ_END_LINE);
                    break;
                case ACTION_SELECT:
                case ACTION_RIGHT:
                    form_driver(info->form, REQ_NEXT_CHOICE);
                    form_driver(info->form, REQ_RIGHT_CHAR);
                    break;
                case ACTION_LEFT:
                    form_driver(info->form, REQ_PREV_CHOICE);
                    form_driver(info->form, REQ_LEFT_CHAR);
                    break;
                case ACTION_NPAGE:
                    form_driver(info->form, REQ_NEXT_PAGE);
                    form_driver(info->form, REQ_END_LINE);
                    info->active_category = form_page(info->form) + 1;
                    break;
                case ACTION_PPAGE:
                    form_driver(info->form, REQ_PREV_PAGE);
                    form_driver(info->form, REQ_END_LINE);
                    info->active_category = form_page(info->form) + 1;
                    break;
                case ACTION_BEGIN:
                    form_driver(info->form, REQ_BEG_LINE);
                    break;
                case ACTION_END:
                    form_driver(info->form, REQ_END_LINE);
                    break;
                case ACTION_NEXT_FIELD:
                    info->active_form = info->buttons_form;
                    set_current_field(info->active_form, info->buttons[BTN_SETTINGS_ACCEPT]);
                    break;
                case ACTION_CLEAR:
                    if (sett_fmt == SETTING_FMT_NUMBER || sett_fmt == SETTING_FMT_STRING) {
                        form_driver(info->form, REQ_BEG_LINE);
                        form_driver(info->form, REQ_CLR_EOL);
                    }
                    break;
                case ACTION_DELETE:
                    if (sett_fmt == SETTING_FMT_NUMBER || sett_fmt == SETTING_FMT_STRING) {
                        form_driver(info->form, REQ_DEL_CHAR);
                    }
                    break;
                case ACTION_BACKSPACE:
                    if (sett_fmt == SETTING_FMT_NUMBER || sett_fmt == SETTING_FMT_STRING) {
                        form_driver(info->form, REQ_DEL_PREV);
                    }
                    break;
                case ACTION_CONFIRM:
                    settings_update_settings(ui);
                    window_destroy(ui);
                    return KEY_HANDLED;
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
                        info->active_form = info->form;
                    } else {
                        form_driver(info->buttons_form, REQ_NEXT_FIELD);
                    }
                    break;
                case ACTION_LEFT:
                case ACTION_UP:
                case ACTION_PREV_FIELD:
                    if (field_idx == BTN_SETTINGS_ACCEPT) {
                        info->active_form = info->form;
                    } else {
                        form_driver(info->buttons_form, REQ_PREV_FIELD);
                    }
                    break;
                case ACTION_SELECT:
                case ACTION_CONFIRM:
                    if (field_idx == BTN_SETTINGS_SAVE)
                        settings_save(ui);
                    settings_update_settings(ui);
                    window_destroy(ui);
                    return KEY_HANDLED;
                default:
                    continue;
            }
        }

        // This panel has handled the key successfully
        break;
    }

    // Validate all input data
    form_driver(info->active_form, REQ_VALIDATION);

    // Get current setting id
    if ((entry = settings_is_entry(current_field(info->active_form)))) {
        // Enable cursor on string and number fields
        curs_set(setting_format(entry->setting_id) != SETTING_FMT_ENUM);
    } else {
        curs_set(0);
    }

    // Return if this panel has handled or not the key
    return (action == ERR) ? KEY_NOT_HANDLED : KEY_HANDLED;
}

void
settings_win_free(Window *window)
{
    curs_set(0);
    window_deinit(window);
}

Window *
settings_win_new()
{
    int i, j, line;
    FIELD *entry, *label;
    int field = 0;

    Window *window = g_malloc0(sizeof(Window));
    window->type = WINDOW_SETTINGS;
    window->destroy = settings_win_free;
    window->draw = settings_draw;
    window->handle_key = settings_handle_key;

    // Cerate a new window for the panel and form
    window_init(window, 24, 70);

    // Initialize Filter panel specific data
    SettingsWinInfo *info = g_malloc0(sizeof(SettingsWinInfo));
    set_panel_userptr(window->panel, (void*) info);

    // Create a scrollable subwindow for settings
    info->form_win = derwin(window->win, window->height - 11, window->width - 2, 8, 1);

    // Configure panel buttons
    info->buttons[BTN_SETTINGS_ACCEPT] = new_field(1, 10, window->height - 2, 12, 0, 0);
    info->buttons[BTN_SETTINGS_SAVE]   = new_field(1, 10, window->height - 2, 29, 0, 0);
    info->buttons[BTN_SETTINGS_CANCEL] = new_field(1, 10, window->height - 2, 46, 0, 0);
    info->buttons[BTN_SETTINGS_COUNT]  = NULL;
    field_opts_off(info->buttons[BTN_SETTINGS_ACCEPT], O_EDIT);
    field_opts_off(info->buttons[BTN_SETTINGS_SAVE], O_EDIT);
    field_opts_off(info->buttons[BTN_SETTINGS_CANCEL], O_EDIT);
    set_field_buffer(info->buttons[BTN_SETTINGS_ACCEPT], 0, "[ Accept ]");
    set_field_buffer(info->buttons[BTN_SETTINGS_SAVE],   0, "[  Save  ]");
    set_field_buffer(info->buttons[BTN_SETTINGS_CANCEL], 0, "[ Cancel ]");
    info->buttons_form = new_form(info->buttons);
    set_form_sub(info->buttons_form, window->win);
    post_form(info->buttons_form);

    // Initialize rest of settings fields
    for (i = 0; categories[i].cat_id; i++) {
        // Each category section begins with fields in the first line
        line = 0;

        for (j = 0; entries[j].cat_id; j++) {
            // Ignore entries of other categories
            if (entries[j].cat_id != categories[i].cat_id)
                continue;

            // Create the label
            label = new_field(1, 45, line, 3, 0, 0);
            set_field_buffer(label, 0, entries[j].label);
            field_opts_off(label, O_ACTIVE);

            // Change field properties according to field type
            switch(setting_format(entries[j].setting_id)) {
                case SETTING_FMT_NUMBER:
                    entry = new_field(1, 18, line, 48, 0, 0);
                    set_field_back(entry, A_UNDERLINE);
                    set_field_type(entry, TYPE_REGEXP, "[0-9]+");
                    break;
                case SETTING_FMT_STRING:
                    entry = new_field(1, 18, line, 48, 0, 0);
                    field_opts_off(entry, O_STATIC);
                    set_field_back(entry, A_UNDERLINE);
                    break;
                case SETTING_FMT_ENUM:
                    entry = new_field(1, 12, line, 48, 0, 0);
                    field_opts_off(entry, O_EDIT);
                    set_field_type(entry, TYPE_ENUM, setting_valid_values(entries[j].setting_id), 0, 0);
                    break;
                default: break;
            }

            field_opts_off(entry, O_AUTOSKIP);
            set_field_buffer(entry, 0, setting_get_value(entries[j].setting_id));
            set_field_userptr(entry, (void *) &entries[j]);

            if (line == 0) {
                // Set last field as page breaker
                set_new_page(entry, TRUE);
            }

            // Store field
            info->fields[field++] = entry;
            info->fields[field++] = label;

            line++;
        }
    }

    // Create the form and post it
    info->form = new_form(info->fields);
    set_form_sub(info->form, info->form_win);
    post_form(info->form);

    // Set the window title and boxes
    mvwprintw(window->win, 1, window->width / 2 - 5, "Settings");
    wattron(window->win, COLOR_PAIR(CP_BLUE_ON_DEF));
    title_foot_box(window->panel);
    mvwhline(window->win, 6, 1, ACS_HLINE, window->width - 1);
    mvwaddch(window->win, 6, 0, ACS_LTEE);
    mvwaddch(window->win, 6, window->width - 1, ACS_RTEE);
    wattroff(window->win, COLOR_PAIR(CP_BLUE_ON_DEF));
    wattron(window->win, COLOR_PAIR(CP_CYAN_ON_DEF));
    mvwprintw(window->win, 3, 1, " Use arrow keys, PgUp, PgDown and Tab to move around settings.");
    mvwprintw(window->win, 4, 1, " Settings with (*) requires restart.");
    wattroff(window->win, COLOR_PAIR(CP_CYAN_ON_DEF));

    // Set default field
    info->active_form = info->form;
    set_current_field(info->form, *info->fields);
    info->active_category = form_page(info->form) + 1;

    return window;
}
