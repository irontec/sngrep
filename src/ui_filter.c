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
#include "sip.h"
#include "filter.h"
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
    const char *method;

    // Calculate window dimensions
    height = 15;
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
    info->fields[FLD_FILTER_SIPFROM] = new_field(1, 28, 3, 18, 0, 0);
    info->fields[FLD_FILTER_SIPTO] = new_field(1, 28, 4, 18, 0, 0);
    info->fields[FLD_FILTER_SRC] = new_field(1, 18, 5, 18, 0, 0);
    info->fields[FLD_FILTER_DST] = new_field(1, 18, 6, 18, 0, 0);
    info->fields[FLD_FILTER_REGISTER] = new_field(1, 1, 8, 15, 0, 0);
    info->fields[FLD_FILTER_INVITE] = new_field(1, 1, 9, 15, 0, 0);
    info->fields[FLD_FILTER_SUBSCRIBE] = new_field(1, 1, 10, 15, 0, 0);
    info->fields[FLD_FILTER_NOTIFY] = new_field(1, 1, 11, 15, 0, 0);
    info->fields[FLD_FILTER_OPTIONS] = new_field(1, 1, 8, 37, 0, 0);
    info->fields[FLD_FILTER_PUBLISH] = new_field(1, 1, 9, 37, 0, 0);
    info->fields[FLD_FILTER_MESSAGE] = new_field(1, 1, 10, 37, 0, 0);
    info->fields[FLD_FILTER_FILTER] = new_field(1, 10, height - 2, 11, 0, 0);
    info->fields[FLD_FILTER_CANCEL] = new_field(1, 10, height - 2, 30, 0, 0);
    info->fields[FLD_FILTER_COUNT] = NULL;

    // Set fields options
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
    mvwprintw(win, 3, 3, "SIP From:");
    mvwprintw(win, 4, 3, "SIP To:");
    mvwprintw(win, 5, 3, "Source:");
    mvwprintw(win, 6, 3, "Destination:");
    mvwprintw(win, 8, 3, "REGISTER   [ ]");
    mvwprintw(win, 9, 3, "INVITE     [ ]");
    mvwprintw(win, 10, 3, "SUBSCRIBE  [ ]");
    mvwprintw(win, 11, 3, "NOTIFY     [ ]");
    mvwprintw(win, 8, 25, "OPTIONS    [ ]");
    mvwprintw(win, 9, 25, "PUBLISH    [ ]");
    mvwprintw(win, 10, 25, "MESSAGE    [ ]");

    // Get Method filter
    if (!(method = filter_get(FILTER_METHOD)))
        method = "";

    // Set Default field values
    set_field_buffer(info->fields[FLD_FILTER_SIPFROM], 0, filter_get(FILTER_SIPFROM));
    set_field_buffer(info->fields[FLD_FILTER_SIPTO], 0, filter_get(FILTER_SIPTO));
    set_field_buffer(info->fields[FLD_FILTER_SRC], 0, filter_get(FILTER_SOURCE));
    set_field_buffer(info->fields[FLD_FILTER_DST], 0, filter_get(FILTER_DESTINATION));
    set_field_buffer(info->fields[FLD_FILTER_REGISTER], 0,
                     strstr(method, sip_method_str(SIP_METHOD_REGISTER)) ? "*" : "");
    set_field_buffer(info->fields[FLD_FILTER_INVITE], 0,
                     strstr(method, sip_method_str(SIP_METHOD_INVITE)) ? "*" : "");
    set_field_buffer(info->fields[FLD_FILTER_SUBSCRIBE], 0,
                     strstr(method,sip_method_str(SIP_METHOD_SUBSCRIBE)) ? "*" : "");
    set_field_buffer(info->fields[FLD_FILTER_NOTIFY], 0,
                     strstr(method, sip_method_str(SIP_METHOD_NOTIFY)) ? "*" : "");
    set_field_buffer(info->fields[FLD_FILTER_OPTIONS], 0,
                     strstr(method, sip_method_str(SIP_METHOD_OPTIONS)) ? "*" : "");
    set_field_buffer(info->fields[FLD_FILTER_PUBLISH], 0,
                     strstr(method,  sip_method_str(SIP_METHOD_PUBLISH)) ? "*" : "");
    set_field_buffer(info->fields[FLD_FILTER_MESSAGE], 0,
                     strstr(method,  sip_method_str(SIP_METHOD_MESSAGE)) ? "*" : "");
    set_field_buffer(info->fields[FLD_FILTER_FILTER], 0, "[ Filter ]");
    set_field_buffer(info->fields[FLD_FILTER_CANCEL], 0, "[ Cancel ]");

    // Set the window title and boxes
    mvwprintw(win, 1, 18, "Filter options");
    wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    title_foot_box(panel_window(panel));
    mvwhline(win, 7, 1, ACS_HLINE, 49);
    mvwaddch(win, 7, 0, ACS_LTEE);
    mvwaddch(win, 7, 49, ACS_RTEE);
    wattroff(win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Set default cursor position
    set_current_field(info->form, info->fields[FLD_FILTER_SIPFROM]);
    wmove(win, 3, 18);
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
    int action = -1;

    // Get panel information
    filter_info_t *info = (filter_info_t*) panel_userptr(panel);

    // Get current field id
    field_idx = field_index(current_field(info->form));

    // Get current field value.
    // We trim spaces with sscanf because and empty field is stored as
    // space characters
    memset(field_value, 0, sizeof(field_value));
    sscanf(field_buffer(current_field(info->form), 0), "%[^ ]", field_value);

    // Check actions for this key
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch (action) {
            case ACTION_PRINTABLE:
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
            case ACTION_NEXT_FIELD:
                form_driver(info->form, REQ_NEXT_FIELD);
                form_driver(info->form, REQ_END_LINE);
                break;
            case ACTION_PREV_FIELD:
                form_driver(info->form, REQ_PREV_FIELD);
                form_driver(info->form, REQ_END_LINE);
                break;
            case ACTION_RIGHT:
                form_driver(info->form, REQ_RIGHT_CHAR);
                break;
            case ACTION_LEFT:
                form_driver(info->form, REQ_LEFT_CHAR);
                break;
            case ACTION_BEGIN:
                form_driver(info->form, REQ_BEG_LINE);
                break;
            case ACTION_END:
                form_driver(info->form, REQ_END_LINE);
                break;
            case ACTION_CLEAR:
                form_driver(info->form, REQ_CLR_FIELD);
                break;
            case KEY_DC:
                form_driver(info->form, REQ_DEL_CHAR);
                break;
            case ACTION_DELETE:
                form_driver(info->form, REQ_DEL_CHAR);
                break;
            case ACTION_BACKSPACE:
                if (strlen(field_value) > 0)
                    form_driver(info->form, REQ_DEL_PREV);
                break;
            case ACTION_SELECT:
                switch (field_idx) {
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
                        form_driver(info->form, ' ');
                        break;
                }
                break;
            case ACTION_CONFIRM:
                if (field_idx != FLD_FILTER_CANCEL)
                    filter_save_options(panel);
                return KEY_ESC;
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
    set_field_back(info->fields[FLD_FILTER_FILTER], A_NORMAL);
    set_field_back(info->fields[FLD_FILTER_CANCEL], A_NORMAL);
    curs_set(1);

    // Change current field background
    field_idx = field_index(current_field(info->form));
    if (field_idx == FLD_FILTER_FILTER || field_idx == FLD_FILTER_CANCEL) {
        set_field_back(info->fields[field_idx], A_REVERSE);
        curs_set(0);
    }

    // Return if this panel has handled or not the key
    return (action == ERR) ? key : 0;
}

void
filter_save_options(PANEL *panel)
{
    char field_value[30];
    char *expr;
    int field_id;
    char method_expr[256];

    // Initialize variables
    memset(method_expr, 0, sizeof(method_expr));

    // Get panel information
    filter_info_t *info = (filter_info_t*) panel_userptr(panel);

    for (field_id = 0; field_id < FLD_FILTER_COUNT; field_id++) {
        // Get current field value.
        // We trim spaces with sscanf because and empty field is stored as
        // space characters
        memset(field_value, 0, sizeof(field_value));
        sscanf(field_buffer(info->fields[field_id], 0), "%[^ ]", field_value);

        // Set filter expression
        if (strlen(field_value)) {
            expr = field_value;
        } else {
            expr = NULL;
        }

        switch (field_id) {
            case FLD_FILTER_SIPFROM:
                filter_set(FILTER_SIPFROM, expr);
                break;
            case FLD_FILTER_SIPTO:
                filter_set(FILTER_SIPTO, expr);
                break;
            case FLD_FILTER_SRC:
                filter_set(FILTER_SOURCE, expr);
                break;
            case FLD_FILTER_DST:
                filter_set(FILTER_DESTINATION, expr);
                break;
            case FLD_FILTER_REGISTER:
            case FLD_FILTER_INVITE:
            case FLD_FILTER_SUBSCRIBE:
            case FLD_FILTER_NOTIFY:
            case FLD_FILTER_OPTIONS:
            case FLD_FILTER_PUBLISH:
            case FLD_FILTER_MESSAGE:
                if (!strcmp(field_value, "*"))
                    sprintf(method_expr + strlen(method_expr), "|%s", filter_field_method(field_id));
                break;
            default:
                break;
        }
    }

    // Set Method filter
    if (strlen(method_expr)) {
        method_expr[0] = '(';
        method_expr[strlen(method_expr)+1] = '\0';
        method_expr[strlen(method_expr)] = ')';
        filter_set(FILTER_METHOD, method_expr);
    } else {
        filter_set(FILTER_METHOD, NULL);
    }

    // Force filter evaluation
    filter_reset_calls();
}

const char*
filter_field_method(int field_id)
{
    int method;
    switch(field_id) {
        case FLD_FILTER_REGISTER:
            method = SIP_METHOD_REGISTER;
            break;
        case FLD_FILTER_INVITE:
            method = SIP_METHOD_INVITE;
            break;
        case FLD_FILTER_SUBSCRIBE:
            method = SIP_METHOD_SUBSCRIBE;
            break;
        case FLD_FILTER_NOTIFY:
            method = SIP_METHOD_NOTIFY;
            break;
        case FLD_FILTER_OPTIONS:
            method = SIP_METHOD_OPTIONS;
            break;
        case FLD_FILTER_PUBLISH:
            method = SIP_METHOD_PUBLISH;
            break;
        case FLD_FILTER_MESSAGE:
            method = SIP_METHOD_MESSAGE;
            break;
    }

    return sip_method_str(method);
}
