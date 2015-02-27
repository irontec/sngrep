/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2014 Ivan Alonso (Kaian)
 ** Copyright (C) 2014 Irontec SL. All rights reserved.
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
 * @file ui_filter.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_filter.h
 *
 */
#include <string.h>
#include <stdlib.h>
#include <form.h>
#include "ui_filter.h"
#include "option.h"

/**
 * Ui Structure definition for Filter panel
 */
ui_t ui_filter = {
    .type = PANEL_FILTER,
    .panel = NULL,
    .create = filter_create,
    .handle_key = filter_handle_key,
    .destroy = filter_destroy
};

PANEL *
filter_create()
{
    PANEL *panel;
    WINDOW *win;
    int height, width;
    filter_info_t *info;

    // Calculate window dimensions
    height = 17;
    width = 50;

    // Cerate a new indow for the panel and form
    win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);

    // Create a new panel
    panel = new_panel(win);

    // Initialize Filter panel specific data
    info = malloc(sizeof(filter_info_t));
    memset(info, 0, sizeof(filter_info_t));

    // Store it into panel userptr
    set_panel_userptr(panel, (void*) info);

    // Initialize the fields
    info->fields[FLD_FILTER_ENABLE] = new_field(1, 1, 3, 19, 0, 0);
    info->fields[FLD_FILTER_SIPFROM] = new_field(1, 28, 5, 18, 0, 0);
    info->fields[FLD_FILTER_SIPTO] = new_field(1, 28, 6, 18, 0, 0);
    info->fields[FLD_FILTER_SRC] = new_field(1, 18, 7, 18, 0, 0);
    info->fields[FLD_FILTER_DST] = new_field(1, 18, 8, 18, 0, 0);
    info->fields[FLD_FILTER_REGISTER] = new_field(1, 1, 10, 15, 0, 0);
    info->fields[FLD_FILTER_INVITE] = new_field(1, 1, 11, 15, 0, 0);
    info->fields[FLD_FILTER_SUBSCRIBE] = new_field(1, 1, 12, 15, 0, 0);
    info->fields[FLD_FILTER_NOTIFY] = new_field(1, 1, 13, 15, 0, 0);
    info->fields[FLD_FILTER_OPTIONS] = new_field(1, 1, 10, 37, 0, 0);
    info->fields[FLD_FILTER_PUBLISH] = new_field(1, 1, 11, 37, 0, 0);
    info->fields[FLD_FILTER_MESSAGE] = new_field(1, 1, 12, 37, 0, 0);
    info->fields[FLD_FILTER_FILTER] = new_field(1, 10, height - 2, 11, 0, 0);
    info->fields[FLD_FILTER_CANCEL] = new_field(1, 10, height - 2, 30, 0, 0);
    info->fields[FLD_FILTER_COUNT] = NULL;

    // Set fields options
    field_opts_off(info->fields[FLD_FILTER_ENABLE], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_FILTER_SIPFROM], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_FILTER_SIPTO], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_FILTER_SRC], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_FILTER_DST], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_FILTER_REGISTER], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_FILTER_INVITE], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_FILTER_SUBSCRIBE], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_FILTER_NOTIFY], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_FILTER_OPTIONS], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_FILTER_PUBLISH], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_FILTER_MESSAGE], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_FILTER_FILTER], O_EDIT);
    field_opts_off(info->fields[FLD_FILTER_CANCEL], O_EDIT);

    // Change background of input fields
    set_field_back(info->fields[FLD_FILTER_SIPFROM], A_UNDERLINE);
    set_field_back(info->fields[FLD_FILTER_SIPTO], A_UNDERLINE);
    set_field_back(info->fields[FLD_FILTER_SRC], A_UNDERLINE);
    set_field_back(info->fields[FLD_FILTER_DST], A_UNDERLINE);

    // Create the form and post it
    info->form = new_form(info->fields);
    set_form_sub(info->form, win);
    post_form(info->form);

    // Fields labels
    mvwprintw(win, 3, 3, "Enable filters [ ]");
    mvwprintw(win, 5, 3, "SIP From:");
    mvwprintw(win, 6, 3, "SIP To:");
    mvwprintw(win, 7, 3, "Source:");
    mvwprintw(win, 8, 3, "Destination:");
    mvwprintw(win, 10, 3, "REGISTER   [ ]");
    mvwprintw(win, 11, 3, "INVITE     [ ]");
    mvwprintw(win, 12, 3, "SUBSCRIBE  [ ]");
    mvwprintw(win, 13, 3, "NOTIFY     [ ]");
    mvwprintw(win, 10, 25, "OPTIONS    [ ]");
    mvwprintw(win, 11, 25, "PUBLISH    [ ]");
    mvwprintw(win, 12, 25, "MESSAGE    [ ]");

    // Set Default field values
    set_field_buffer(info->fields[FLD_FILTER_ENABLE], 0, "*");
    set_field_buffer(info->fields[FLD_FILTER_SIPFROM], 0, get_option_value("filter.sipfrom"));
    set_field_buffer(info->fields[FLD_FILTER_SIPTO], 0, get_option_value("filter.sipto"));
    set_field_buffer(info->fields[FLD_FILTER_SRC], 0, get_option_value("filter.src"));
    set_field_buffer(info->fields[FLD_FILTER_DST], 0, get_option_value("filter.dst"));
    set_field_buffer(info->fields[FLD_FILTER_REGISTER], 0,
                     is_option_enabled("filter.REGISTER") ? "*" : "");
    set_field_buffer(info->fields[FLD_FILTER_INVITE], 0,
                     is_option_enabled("filter.INVITE") ? "*" : "");
    set_field_buffer(info->fields[FLD_FILTER_SUBSCRIBE], 0,
                     is_option_enabled("filter.SUBSCRIBE") ? "*" : "");
    set_field_buffer(info->fields[FLD_FILTER_NOTIFY], 0,
                     is_option_enabled("filter.NOTIFY") ? "*" : "");
    set_field_buffer(info->fields[FLD_FILTER_OPTIONS], 0,
                     is_option_enabled("filter.OPTIONS") ? "*" : "");
    set_field_buffer(info->fields[FLD_FILTER_PUBLISH], 0,
                     is_option_enabled("filter.PUBLISH") ? "*" : "");
    set_field_buffer(info->fields[FLD_FILTER_MESSAGE], 0,
                     is_option_enabled("filter.MESSAGE") ? "*" : "");
    set_field_buffer(info->fields[FLD_FILTER_FILTER], 0, "[ Filter ]");
    set_field_buffer(info->fields[FLD_FILTER_CANCEL], 0, "[ Cancel ]");

    // Set the window title and boxes
    mvwprintw(win, 1, 18, "Filter options");
    wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    title_foot_box(panel_window(panel));
    mvwhline(win, 4, 1, ACS_HLINE, 49);
    mvwaddch(win, 4, 0, ACS_LTEE);
    mvwaddch(win, 4, 49, ACS_RTEE);
    mvwhline(win, 9, 1, ACS_HLINE, 49);
    mvwaddch(win, 9, 0, ACS_LTEE);
    mvwaddch(win, 9, 49, ACS_RTEE);
    wattroff(win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Set default cursor position
    set_current_field(info->form, info->fields[FLD_FILTER_SIPFROM]);
    wmove(win, 5, 18);
    curs_set(1);

    return panel;
}

void
filter_destroy(PANEL *panel)
{
    // Disable cursor position
    curs_set(0);
}

int
filter_handle_key(PANEL *panel, int key)
{
    int field_idx, i;
    char field_value[30];

    // Get panel information
    filter_info_t *info = (filter_info_t*) panel_userptr(panel);

    // Get current field id
    field_idx = field_index(current_field(info->form));

    // Get current field value.
    // We trim spaces with sscanf because and empty field is stored as
    // space characters
    memset(field_value, 0, sizeof(field_value));
    sscanf(field_buffer(current_field(info->form), 0), "%[^ ]", field_value);

    switch (key) {
        case 9 /*KEY_TAB*/:
        case KEY_DOWN:
            form_driver(info->form, REQ_NEXT_FIELD);
            form_driver(info->form, REQ_END_LINE);
            break;
        case KEY_UP:
            form_driver(info->form, REQ_PREV_FIELD);
            form_driver(info->form, REQ_END_LINE);
            break;
        case KEY_RIGHT:
            form_driver(info->form, REQ_RIGHT_CHAR);
            break;
        case KEY_LEFT:
            form_driver(info->form, REQ_LEFT_CHAR);
            break;
        case KEY_HOME:
            form_driver(info->form, REQ_BEG_LINE);
            break;
        case KEY_END:
            form_driver(info->form, REQ_END_LINE);
            break;
        case KEY_DC:
            form_driver(info->form, REQ_DEL_CHAR);
            break;
        case 27 /*KEY_ESC*/:
            return key;
            break;
        case 8:
        case 127:
        case KEY_BACKSPACE:
            if (strlen(field_value) > 0)
                form_driver(info->form, REQ_DEL_PREV);
            break;
        case 'I':
            for (i = 0; i < FLD_FILTER_COUNT; i++) {
                set_current_field(info->form, info->fields[i]);
                if (i == FLD_FILTER_INVITE || i == FLD_FILTER_ENABLE) {
                    form_driver(info->form, '*');
                } else {
                    form_driver(info->form, REQ_CLR_FIELD);
                }
            }
            set_current_field(info->form, info->fields[FLD_FILTER_FILTER]);
            break;
        case ' ':
            switch (field_idx) {
                case FLD_FILTER_ENABLE:
                case FLD_FILTER_REGISTER:
                case FLD_FILTER_INVITE:
                case FLD_FILTER_SUBSCRIBE:
                case FLD_FILTER_NOTIFY:
                case FLD_FILTER_OPTIONS:
                case FLD_FILTER_PUBLISH:
                case FLD_FILTER_MESSAGE:
                    if (field_value[0] == '*') {
                        form_driver(info->form, REQ_DEL_CHAR);
                    } else {
                        form_driver(info->form, '*');
                    }
                    break;
                default:
                    form_driver(info->form, REQ_NEXT_FIELD);
                    form_driver(info->form, REQ_END_LINE);
                    break;
            }
            break;
        case 10: /* KEY_ENTER */
            if (field_idx != FLD_FILTER_CANCEL)
                filter_save_options(panel);
            return 27;
        default:
            // If this is a normal character on input field, print it
            switch (field_idx) {
                case FLD_FILTER_SIPFROM:
                case FLD_FILTER_SIPTO:
                case FLD_FILTER_SRC:
                case FLD_FILTER_DST:
                    form_driver(info->form, key);
                    break;
            }
            break;
    }

    // Validate all input data
    form_driver(info->form, REQ_VALIDATION);

    // Change background and cursor of "button fields"
    set_field_back(info->fields[FLD_FILTER_FILTER], A_NORMAL);
    set_field_back(info->fields[FLD_FILTER_CANCEL], A_NORMAL);
    curs_set(1);

    // Change current field background
    field_idx = field_index(current_field(info->form));
    if (field_idx == FLD_FILTER_FILTER || field_idx == FLD_FILTER_CANCEL) {
        set_field_back(info->fields[field_idx], A_REVERSE);
        curs_set(0);
    }

    return 0;
}

void
filter_save_options(PANEL *panel)
{
    sip_call_t *call = NULL;
    char field_value[30];
    int i;

    // Get panel information
    filter_info_t *info = (filter_info_t*) panel_userptr(panel);

    for (i = 0; i < FLD_FILTER_COUNT; i++) {
        // Get current field value.
        // We trim spaces with sscanf because and empty field is stored as
        // space characters
        memset(field_value, 0, sizeof(field_value));
        sscanf(field_buffer(info->fields[i], 0), "%[^ ]", field_value);

        switch (i) {
            case FLD_FILTER_ENABLE:
                set_option_value("filter.enable", strlen(field_value) ? "on" : "off");
                break;
            case FLD_FILTER_SIPFROM:
                set_option_value("filter.sipfrom", field_value);
                break;
            case FLD_FILTER_SIPTO:
                set_option_value("filter.sipto", field_value);
                break;
            case FLD_FILTER_SRC:
                set_option_value("filter.src", field_value);
                break;
            case FLD_FILTER_DST:
                set_option_value("filter.dst", field_value);
                break;
            case FLD_FILTER_REGISTER:
                set_option_value("filter.REGISTER", strlen(field_value) ? "on" : "off");
                break;
            case FLD_FILTER_INVITE:
                set_option_value("filter.INVITE", strlen(field_value) ? "on" : "off");
                break;
            case FLD_FILTER_SUBSCRIBE:
                set_option_value("filter.SUBSCRIBE", strlen(field_value) ? "on" : "off");
                break;
            case FLD_FILTER_NOTIFY:
                set_option_value("filter.NOTIFY", strlen(field_value) ? "on" : "off");
                break;
            case FLD_FILTER_OPTIONS:
                set_option_value("filter.OPTIONS", strlen(field_value) ? "on" : "off");
                break;
            case FLD_FILTER_PUBLISH:
                set_option_value("filter.PUBLISH", strlen(field_value) ? "on" : "off");
                break;
            case FLD_FILTER_MESSAGE:
                set_option_value("filter.MESSAGE", strlen(field_value) ? "on" : "off");
                break;
            default:
                break;
        }
    }

    // Force filter evaluation
    while((call = call_get_next(call)))
        call->filtered = -1;
}

