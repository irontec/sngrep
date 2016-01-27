/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2015 Ivan Alonso (Kaian)
 ** Copyright (C) 2015 Irontec SL. All rights reserved.
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
 * @file ui_settings.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_settings.h
 */
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ui_manager.h"
#include "ui_settings.h"
#include "setting.h"

/**
 * Ui Structure definition for Settings panel
 */
ui_t ui_settings = {
    .type = PANEL_SETTINGS,
    .panel = NULL,
    .create = settings_create,
    .draw = settings_draw,
    .handle_key = settings_handle_key,
    .destroy = settings_destroy
};

settings_category_t categories[] = {
    { CAT_SETTINGS_INTERFACE,  "Interface" },
    { CAT_SETTINGS_CAPTURE,    "Capture" },
    { CAT_SETTINGS_CALL_FLOW,  "Call Flow" },
    { CAT_SETTINGS_EEP_HOMER,  "EEP/HEP Homer" },
    { 0 , NULL },
};

settings_entry_t entries[] = {
    { CAT_SETTINGS_INTERFACE,  FLD_SETTINGS_BACKGROUND,         SETTING_BACKGROUND,         "Background * .............................." },
    { CAT_SETTINGS_INTERFACE,  FLD_SETTINGS_SYNTAX,             SETTING_SYNTAX,             "SIP message syntax ........................" },
    { CAT_SETTINGS_INTERFACE,  FLD_SETTINGS_SYNTAX_TAG,         SETTING_SYNTAX_TAG,         "SIP tag syntax ............................" },
    { CAT_SETTINGS_INTERFACE,  FLD_SETTINGS_SYNTAX_BRANCH,      SETTING_SYNTAX_BRANCH,      "SIP branch syntax ........................." },
    { CAT_SETTINGS_INTERFACE,  FLD_SETTINGS_ALTKEY_HINT,        SETTING_ALTKEY_HINT,        "Alternative keybinding hints .............." },
    { CAT_SETTINGS_INTERFACE,  FLD_SETTINGS_COLORMODE,          SETTING_COLORMODE,          "Default message color mode ................" },
    { CAT_SETTINGS_INTERFACE,  FLD_SETTINGS_EXITPROMPT,         SETTING_EXITPROMPT,         "Always prompt on quit ....................." },
    { CAT_SETTINGS_INTERFACE,  FLD_SETTINGS_DISPLAY_ALIAS,      SETTING_DISPLAY_ALIAS,      "Replace addresses with alias .............." },
    { CAT_SETTINGS_CAPTURE,    FLD_SETTINGS_CAPTURE_RTP,        SETTING_CAPTURE_RTP,        "Capture RTP packets * ....................." },
    { CAT_SETTINGS_CAPTURE,    FLD_SETTINGS_CAPTURE_LIMIT,      SETTING_CAPTURE_LIMIT,      "Max dialogs * ............................." },
    { CAT_SETTINGS_CAPTURE,    FLD_SETTINGS_CAPTURE_DEVICE,     SETTING_CAPTURE_DEVICE,     "Capture device * .........................." },
    { CAT_SETTINGS_CAPTURE,    FLD_SETTINGS_SIP_NOINCOMPLETE,   SETTING_SIP_NOINCOMPLETE,   "Capture full transactions ................." },
    { CAT_SETTINGS_CAPTURE,    FLD_SETTINGS_SIP_CALLS,          SETTING_SIP_CALLS,          "Only capture calls * ......................" },
    { CAT_SETTINGS_CAPTURE,    FLD_SETTINGS_SAVEPATH,           SETTING_SAVEPATH,           "Default Save path ........................." },
    { CAT_SETTINGS_CALL_FLOW,  FLD_SETTINGS_CF_FORCERAW,        SETTING_CF_FORCERAW,        "Show message preview panel ................" },
    { CAT_SETTINGS_CALL_FLOW,  FLD_SETTINGS_CF_HIGHTLIGHT,      SETTING_CF_HIGHTLIGHT,      "Selected message hightlight ..............." },
    { CAT_SETTINGS_CALL_FLOW,  FLD_SETTINGS_CF_LOCALHIGHLIGHT,  SETTING_CF_LOCALHIGHLIGHT,  "Highlight local addresses ................." },
    { CAT_SETTINGS_CALL_FLOW,  FLD_SETTINGS_CF_SPLITCACALLID,   SETTING_CF_SPLITCALLID,     "Merge columns with same address ..........." },
    { CAT_SETTINGS_CALL_FLOW,  FLD_SETTINGS_CF_SDPONLY,         SETTING_CF_SDP_INFO,        "Show SDP information in messages .........." },
    { CAT_SETTINGS_CALL_FLOW,  FLD_SETTINGS_CF_DELTA,           SETTING_CF_DELTA,           "Show delta time between messages .........." },
    { CAT_SETTINGS_CALL_FLOW,  FLD_SETTINGS_CF_MEDIA,           SETTING_CF_MEDIA,           "Show RTP media streams ...................." },
    { CAT_SETTINGS_CALL_FLOW,  FLD_SETTINGS_CF_SCROLLSTEP,      SETTING_CF_SCROLLSTEP,      "Steps for PgUp/PgDown ....................." },
    { CAT_SETTINGS_EEP_HOMER,  FLD_SETTINGS_EEP_SEND,           SETTING_EEP_SEND,           "Send all captured SIP packets ............." },
    { CAT_SETTINGS_EEP_HOMER,  FLD_SETTINGS_EEP_SEND_VER,       SETTING_EEP_SEND_VER,       "Send EEP version .........................." },
    { CAT_SETTINGS_EEP_HOMER,  FLD_SETTINGS_EEP_SEND_ADDR,      SETTING_EEP_SEND_ADDR,      "Send EEP packet address ..................." },
    { CAT_SETTINGS_EEP_HOMER,  FLD_SETTINGS_EEP_SEND_PORT,      SETTING_EEP_SEND_PORT,      "Send EEP packet port ......................" },
    { CAT_SETTINGS_EEP_HOMER,  FLD_SETTINGS_EEP_SEND_PASS,      SETTING_EEP_SEND_PASS,      "EEP send password ........................." },
    { CAT_SETTINGS_EEP_HOMER,  FLD_SETTINGS_EEP_LISTEN,         SETTING_EEP_LISTEN,         "Listen for eep packets ...................." },
    { CAT_SETTINGS_EEP_HOMER,  FLD_SETTINGS_EEP_LISTEN_VER,     SETTING_EEP_LISTEN_VER,     "Listen EEP version  ......................." },
    { CAT_SETTINGS_EEP_HOMER,  FLD_SETTINGS_EEP_LISTEN_ADDR,    SETTING_EEP_LISTEN_ADDR,    "Listen EEP packet address ................." },
    { CAT_SETTINGS_EEP_HOMER,  FLD_SETTINGS_EEP_LISTEN_PORT,    SETTING_EEP_LISTEN_PORT,    "Listen EEP packet port ...................." },
    { CAT_SETTINGS_EEP_HOMER,  FLD_SETTINGS_EEP_LISTEN_PASS,    SETTING_EEP_LISTEN_PASS,    "EEP server password ......................." },
    { 0 , 0, 0, NULL },
};

PANEL *
settings_create()
{
    PANEL *panel;
    WINDOW *win;
    int height, width, i, j, line;
    settings_info_t *info;
    FIELD *entry, *label;
    int field = 0;

    // Calculate window dimensions
    height = 21;
    width = 70;

    // Cerate a new window for the panel and form
    win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);

    // Create a new panel
    panel = new_panel(win);

    // Initialize Filter panel specific data
    info = sng_malloc(sizeof(settings_info_t));

    // Store it into panel userptr
    set_panel_userptr(panel, (void*) info);

    // Create a scrollable subwindow for settings
    info->form_win = derwin(win, height - 11, width - 2, 8, 1);

    // Configure panel buttons
    info->buttons[BTN_SETTINGS_ACCEPT] = new_field(1, 10, height - 2, 12, 0, 0);
    info->buttons[BTN_SETTINGS_SAVE]   = new_field(1, 10, height - 2, 29, 0, 0);
    info->buttons[BTN_SETTINGS_CANCEL] = new_field(1, 10, height - 2, 46, 0, 0);
    info->buttons[BTN_SETTINGS_COUNT]  = NULL;
    field_opts_off(info->buttons[BTN_SETTINGS_ACCEPT], O_EDIT);
    field_opts_off(info->buttons[BTN_SETTINGS_SAVE], O_EDIT);
    field_opts_off(info->buttons[BTN_SETTINGS_CANCEL], O_EDIT);
    set_field_buffer(info->buttons[BTN_SETTINGS_ACCEPT], 0, "[ Accept ]");
    set_field_buffer(info->buttons[BTN_SETTINGS_SAVE],   0, "[  Save  ]");
    set_field_buffer(info->buttons[BTN_SETTINGS_CANCEL], 0, "[ Cancel ]");
    info->buttons_form = new_form(info->buttons);
    set_form_sub(info->buttons_form, win);
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
    mvwprintw(win, 1, width / 2 - 5, "Settings");
    wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    title_foot_box(panel);
    mvwhline(win, 6, 1, ACS_HLINE, width - 1);
    mvwaddch(win, 6, 0, ACS_LTEE);
    mvwaddch(win, 6, width - 1, ACS_RTEE);
    wattroff(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    wattron(win, COLOR_PAIR(CP_CYAN_ON_DEF));
    mvwprintw(win, 3, 1, " Use arrow keys, PgUp, PgDown and Tab to move arround settings.");
    mvwprintw(win, 4, 1, " Settings with (*) requires restart.");
    wattroff(win, COLOR_PAIR(CP_CYAN_ON_DEF));

    // Set default field
    info->active_form = info->form;
    set_current_field(info->form, *info->fields);
    info->active_category = form_page(info->form) + 1;

    return panel;
}

void
settings_destroy()
{
    curs_set(0);
}

settings_info_t *
settings_info(PANEL *panel)
{
    return (settings_info_t*) panel_userptr(panel);
}

int
settings_draw(PANEL *panel)
{
    WINDOW *win;
    int field_idx;
    int i;
    int cury, curx;


    // Get panel information
    settings_info_t *info = settings_info(panel);

    win = panel_window(panel);

    // Store cursor position
    getyx(win, cury, curx);


    // Get current field id
    field_idx = field_index(current_field(info->form));

    // Print category headers
    int colpos = 2;
    for (i = 0; categories[i].cat_id; i++) {
        if (categories[i].cat_id == info->active_category) {
            mvwprintw(win, 6, colpos, "%c %s %c", '[', categories[i].title, ']');
        } else {
            wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
            mvwprintw(win, 6, colpos, "%c %s %c", '[', categories[i].title, ']');
            wattroff(win, COLOR_PAIR(CP_BLUE_ON_DEF));
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

    touchwin(win);

    // Restore cursor position
    wmove(win, cury, curx);

    return 0;
}

int
settings_handle_key(PANEL *panel, int key)
{
    int action = -1;
    int field_idx;
    settings_entry_t *entry;
    enum setting_fmt sett_fmt = -1;

    // Get panel information
    settings_info_t *info = settings_info(panel);

    // Get current field id
    field_idx = field_index(current_field(info->active_form));

    // Get current setting id;
    if ((entry = ui_settings_is_entry(current_field(info->active_form)))) {
        sett_fmt = setting_format(entry->setting_id);
    }

    // Check actions for this key
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
                    ui_settings_update_settings(panel);
                    return KEY_ESC;
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
                    if (field_idx == BTN_SETTINGS_CANCEL)
                        return KEY_ESC;
                    if (field_idx == BTN_SETTINGS_SAVE)
                        ui_settings_save(panel);
                    ui_settings_update_settings(panel);
                    return KEY_ESC;
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
    if ((entry = ui_settings_is_entry(current_field(info->active_form)))) {
        // Enable cursor on string and number fields
        curs_set(setting_format(entry->setting_id) != SETTING_FMT_ENUM);
    } else {
        curs_set(0);
    }

    // Return if this panel has handled or not the key
    return (action == ERR) ? key : 0;
}

settings_entry_t *
ui_settings_is_entry(FIELD *field)
{
    return (settings_entry_t *) field_userptr(field);
}

int
ui_settings_update_settings(PANEL *panel)
{
    int i;
    char field_value[180];
    settings_entry_t *entry;

    // Get panel information
    settings_info_t *info = settings_info(panel);

    for (i=0; i < FLD_SETTINGS_COUNT; i++) {
        if ((entry = ui_settings_is_entry(info->fields[i]))) {
            // Get field value.
            memset(field_value, 0, sizeof(field_value));
            strcpy(field_value, field_buffer(info->fields[i], 0));
            strtrim(field_value);
            // Change setting value
            setting_set_value(entry->setting_id, field_value);
        }
    }

    return 0;
}

void
ui_settings_save(PANEL *panel)
{
    int i;
    FILE *fi, *fo;
    char line[1024];
    char *home = getenv("HOME");
    char userconf[128], tmpfile[128];
    char field_value[180];
    settings_entry_t *entry;

    // Get panel information
    settings_info_t *info = settings_info(panel);

    // No home dir...
    if (!home) {
        dialog_run("Unable to save configuration. User has no $HOME dir.");
        return;
    }

    // Read current $HOME/.sngreprc file
    sprintf(userconf, "%s/.sngreprc", home);
    sprintf(tmpfile, "%s/.sngreprc.old", home);

    // Remove old config file
    unlink(tmpfile);

    // Move home file to temporal dir
    rename(userconf, tmpfile);

    // Create a new user conf file
    if (!(fo = fopen(userconf, "w"))) {
        return;
    }

    // Read all lines of old sngreprc file
    if ((fi = fopen(tmpfile, "r"))) {
        // Read all configuration file
        while (fgets(line, 1024, fi) != NULL) {
            // Ignore lines starting with set (but keep set column ones)
            if (strncmp(line, "set ", 4) || !strncmp(line, "set cl.column", 13)) {
                // Put everyting in new .sngreprc file
                fputs(line, fo);
            }
        }
        fclose(fi);
    }

    for (i=0; i < FLD_SETTINGS_COUNT; i++) {
        if ((entry = ui_settings_is_entry(info->fields[i]))) {
            // Get field value.
            memset(field_value, 0, sizeof(field_value));
            strcpy(field_value, field_buffer(info->fields[i], 0));
            strtrim(field_value);

            // Change setting value
            fprintf(fo, "set %s %s\n", setting_name(entry->setting_id), field_value);
        }
    }
    fclose(fo);

    dialog_run("Settings successfully saved to %s", userconf);
}
