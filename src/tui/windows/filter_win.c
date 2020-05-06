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
 * @file ui_filter.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_filter.h
 *
 */
#include "config.h"
#include <glib.h>
#include <string.h>
#include <form.h>
#include "packet/packet_sip.h"
#include "tui/tui.h"
#include "filter_win.h"
#include "call_list_win.h"
#include "storage/filter.h"
#include "setting.h"

G_DEFINE_TYPE(FilterWindow, filter_win, SNG_TYPE_WINDOW)

/**
 * @brief Return String value for a filter field
 *
 * @return method name
 */
static const char *
filter_win_field_method(int field_id)
{
    int method;

    switch (field_id) {
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
        case FLD_FILTER_INFO:
            method = SIP_METHOD_INFO;
            break;
        case FLD_FILTER_REFER:
            method = SIP_METHOD_REFER;
            break;
        case FLD_FILTER_UPDATE:
            method = SIP_METHOD_UPDATE;
            break;
        default:
            method = -1;
            break;
    }

    return sip_method_str(method);
}

/**
 * @brief Save form data to options
 *
 * This function will update the options values
 * of filter fields with its new value.
 *
 * @param ui UI structure pointer
 */
static void
filter_win_save_options(FilterWindow *self)
{
    char field_value[SETTING_MAX_LEN];
    char *expr;
    int field_id;
    char method_expr[256];

    // Initialize variables
    memset(method_expr, 0, sizeof(method_expr));

    for (field_id = 0; field_id < FLD_FILTER_COUNT; field_id++) {
        // Get current field value.
        // We trim spaces with because an empty field is stored as space characters
        memset(field_value, 0, sizeof(field_value));
        strcpy(field_value, field_buffer(self->fields[field_id], 0));
        g_strstrip(field_value);

        // Set filter expression
        expr = strlen(field_value) ? field_value : NULL;

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
            case FLD_FILTER_PAYLOAD:
                filter_set(FILTER_PAYLOAD, expr);
                break;
            case FLD_FILTER_REGISTER:
            case FLD_FILTER_INVITE:
            case FLD_FILTER_SUBSCRIBE:
            case FLD_FILTER_NOTIFY:
            case FLD_FILTER_OPTIONS:
            case FLD_FILTER_PUBLISH:
            case FLD_FILTER_MESSAGE:
            case FLD_FILTER_INFO:
            case FLD_FILTER_REFER:
            case FLD_FILTER_UPDATE:
                if (!strcmp(field_value, "*")) {
                    if (strlen(method_expr)) {
                        sprintf(method_expr + strlen(method_expr), ",%s", filter_win_field_method(field_id));
                    } else {
                        strcpy(method_expr, filter_win_field_method(field_id));
                    }
                }
                break;
            default:
                break;
        }
    }

    // Set Method filter
    filter_method_from_setting(method_expr);

    // Force filter evaluation
    filter_reset_calls();

    // TODO FIXME Refresh call list FIXME
//    call_list_win_clear(tui_find_by_type(WINDOW_CALL_LIST));
}


/**
 * @brief Manage pressed keys for filter panel
 *
 * This function is called by UI manager every time a
 * key is pressed. This allow the filter panel to manage
 * its own keys.
 *
 * @param ui UI structure pointer
 * @param key   key code
 * @return enum @key_handler_ret
 */
static void
filter_win_handle_key(SngWidget *widget, gint key)
{
    int field_idx;
    char field_value[SETTING_MAX_LEN];

    // Get panel information
    FilterWindow *self = TUI_FILTER(widget);

    // Get current field id
    field_idx = field_index(current_field(self->form));

    // Get current field value.
    memset(field_value, 0, sizeof(field_value));
    strcpy(field_value, field_buffer(current_field(self->form), 0));
    // We trim spaces because an empty field is stored as space characters
    g_strstrip(field_value);

    // Check actions for this key
    SngAction action = ACTION_NONE;
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch (action) {
            case ACTION_PRINTABLE:
                // If this is a normal character on input field, print it
                if (field_idx == FLD_FILTER_SIPFROM || field_idx == FLD_FILTER_SIPTO
                    || field_idx == FLD_FILTER_SRC || field_idx == FLD_FILTER_DST
                    || field_idx == FLD_FILTER_PAYLOAD) {
                    form_driver(self->form, key);
                    break;
                }
                continue;
            case ACTION_NEXT_FIELD:
                form_driver(self->form, REQ_NEXT_FIELD);
                form_driver(self->form, REQ_END_LINE);
                break;
            case ACTION_PREV_FIELD:
                form_driver(self->form, REQ_PREV_FIELD);
                form_driver(self->form, REQ_END_LINE);
                break;
            case ACTION_RIGHT:
                form_driver(self->form, REQ_RIGHT_CHAR);
                break;
            case ACTION_LEFT:
                form_driver(self->form, REQ_LEFT_CHAR);
                break;
            case ACTION_BEGIN:
                form_driver(self->form, REQ_BEG_LINE);
                break;
            case ACTION_END:
                form_driver(self->form, REQ_END_LINE);
                break;
            case ACTION_CLEAR:
                form_driver(self->form, REQ_CLR_FIELD);
                break;
            case ACTION_DELETE:
                form_driver(self->form, REQ_DEL_CHAR);
                break;
            case ACTION_BACKSPACE:
                if (strlen(field_value) > 0)
                    form_driver(self->form, REQ_DEL_PREV);
                break;
            case ACTION_SELECT:
                switch (field_idx) {
                    case FLD_FILTER_REGISTER:
                    case FLD_FILTER_INVITE:
                    case FLD_FILTER_SUBSCRIBE:
                    case FLD_FILTER_NOTIFY:
                    case FLD_FILTER_INFO:
                    case FLD_FILTER_OPTIONS:
                    case FLD_FILTER_PUBLISH:
                    case FLD_FILTER_MESSAGE:
                    case FLD_FILTER_REFER:
                    case FLD_FILTER_UPDATE:
                        if (field_value[0] == '*') {
                            form_driver(self->form, REQ_DEL_CHAR);
                        } else {
                            form_driver(self->form, '*');
                        }
                        break;
                    case FLD_FILTER_CANCEL:
                        return;
                    case FLD_FILTER_FILTER:
                        filter_win_save_options(self);
                        return;
                    default:
                        break;
                }
                break;
            case ACTION_CONFIRM:
                if (field_idx != FLD_FILTER_CANCEL)
                    filter_win_save_options(self);
                return ;
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
    set_field_back(self->fields[FLD_FILTER_FILTER], A_NORMAL);
    set_field_back(self->fields[FLD_FILTER_CANCEL], A_NORMAL);
    curs_set(1);

    // Change current field background
    field_idx = field_index(current_field(self->form));
    if (field_idx == FLD_FILTER_FILTER || field_idx == FLD_FILTER_CANCEL) {
        set_field_back(self->fields[field_idx], A_REVERSE);
        curs_set(0);
    }

}

SngAppWindow *
filter_win_new()
{
    return g_object_new(
        WINDOW_TYPE_FILTER,
        "window-type", SNG_WINDOW_TYPE_FILTER,
        "height", 17,
        "width", 50,
        NULL
    );
}

static void
filter_win_constructed(GObject *object)
{
    // Chain-up parent constructed
    G_OBJECT_CLASS(filter_win_parent_class)->constructed(object);

    FilterWindow *self = TUI_FILTER(object);
    SngWidget *widget = SNG_WIDGET(object);
    WINDOW *win = sng_widget_get_ncurses_window(widget);
    gint height = sng_widget_get_height(widget);

    // Initialize the fields
    self->fields[FLD_FILTER_SIPFROM] = new_field(1, 28, 3, 18, 0, 0);
    self->fields[FLD_FILTER_SIPTO] = new_field(1, 28, 4, 18, 0, 0);
    self->fields[FLD_FILTER_SRC] = new_field(1, 18, 5, 18, 0, 0);
    self->fields[FLD_FILTER_DST] = new_field(1, 18, 6, 18, 0, 0);
    self->fields[FLD_FILTER_PAYLOAD] = new_field(1, 28, 7, 18, 0, 0);
    self->fields[FLD_FILTER_REGISTER] = new_field(1, 1, 9, 15, 0, 0);
    self->fields[FLD_FILTER_INVITE] = new_field(1, 1, 10, 15, 0, 0);
    self->fields[FLD_FILTER_SUBSCRIBE] = new_field(1, 1, 11, 15, 0, 0);
    self->fields[FLD_FILTER_NOTIFY] = new_field(1, 1, 12, 15, 0, 0);
    self->fields[FLD_FILTER_INFO] = new_field(1, 1, 13, 15, 0, 0);
    self->fields[FLD_FILTER_OPTIONS] = new_field(1, 1, 9, 37, 0, 0);
    self->fields[FLD_FILTER_PUBLISH] = new_field(1, 1, 10, 37, 0, 0);
    self->fields[FLD_FILTER_MESSAGE] = new_field(1, 1, 11, 37, 0, 0);
    self->fields[FLD_FILTER_REFER] = new_field(1, 1, 12, 37, 0, 0);
    self->fields[FLD_FILTER_UPDATE] = new_field(1, 1, 13, 37, 0, 0);
    self->fields[FLD_FILTER_FILTER] = new_field(1, 10, height - 2, 11, 0, 0);
    self->fields[FLD_FILTER_CANCEL] = new_field(1, 10, height - 2, 30, 0, 0);
    self->fields[FLD_FILTER_COUNT] = NULL;

    // Set fields options
    field_opts_off(self->fields[FLD_FILTER_SIPFROM], O_AUTOSKIP);
    field_opts_off(self->fields[FLD_FILTER_SIPTO], O_AUTOSKIP);
    field_opts_off(self->fields[FLD_FILTER_SRC], O_AUTOSKIP);
    field_opts_off(self->fields[FLD_FILTER_DST], O_AUTOSKIP);
    field_opts_off(self->fields[FLD_FILTER_PAYLOAD], O_AUTOSKIP | O_STATIC);
    field_opts_off(self->fields[FLD_FILTER_REGISTER], O_AUTOSKIP);
    field_opts_off(self->fields[FLD_FILTER_INVITE], O_AUTOSKIP);
    field_opts_off(self->fields[FLD_FILTER_SUBSCRIBE], O_AUTOSKIP);
    field_opts_off(self->fields[FLD_FILTER_NOTIFY], O_AUTOSKIP);
    field_opts_off(self->fields[FLD_FILTER_INFO], O_AUTOSKIP);
    field_opts_off(self->fields[FLD_FILTER_OPTIONS], O_AUTOSKIP);
    field_opts_off(self->fields[FLD_FILTER_PUBLISH], O_AUTOSKIP);
    field_opts_off(self->fields[FLD_FILTER_MESSAGE], O_AUTOSKIP);
    field_opts_off(self->fields[FLD_FILTER_REFER], O_AUTOSKIP);
    field_opts_off(self->fields[FLD_FILTER_UPDATE], O_AUTOSKIP);
    field_opts_off(self->fields[FLD_FILTER_FILTER], O_EDIT);
    field_opts_off(self->fields[FLD_FILTER_CANCEL], O_EDIT);

    // Change background of input fields
    set_field_back(self->fields[FLD_FILTER_SIPFROM], A_UNDERLINE);
    set_field_back(self->fields[FLD_FILTER_SIPTO], A_UNDERLINE);
    set_field_back(self->fields[FLD_FILTER_SRC], A_UNDERLINE);
    set_field_back(self->fields[FLD_FILTER_DST], A_UNDERLINE);
    set_field_back(self->fields[FLD_FILTER_PAYLOAD], A_UNDERLINE);

    // Create the form and post it
    self->form = new_form(self->fields);
    set_form_sub(self->form, win);
    post_form(self->form);

    // Fields labels
    mvwprintw(win, 3, 3, "SIP From:");
    mvwprintw(win, 4, 3, "SIP To:");
    mvwprintw(win, 5, 3, "Source:");
    mvwprintw(win, 6, 3, "Destination:");
    mvwprintw(win, 7, 3, "Payload:");
    mvwprintw(win, 9, 3, "REGISTER   [ ]");
    mvwprintw(win, 10, 3, "INVITE     [ ]");
    mvwprintw(win, 11, 3, "SUBSCRIBE  [ ]");
    mvwprintw(win, 12, 3, "NOTIFY     [ ]");
    mvwprintw(win, 13, 3, "INFO       [ ]");
    mvwprintw(win, 9, 25, "OPTIONS    [ ]");
    mvwprintw(win, 10, 25, "PUBLISH    [ ]");
    mvwprintw(win, 11, 25, "MESSAGE    [ ]");
    mvwprintw(win, 12, 25, "REFER      [ ]");
    mvwprintw(win, 13, 25, "UPDATE     [ ]");

    // Get Method filter
    const gchar *method = filter_get(FILTER_METHOD);
    if (method == NULL) {
        method = setting_get_value(SETTING_STORAGE_FILTER_METHODS);
    }
    if (method == NULL) {
        method = "";
    }

    // Set Default field values
    set_field_buffer(self->fields[FLD_FILTER_SIPFROM], 0, filter_get(FILTER_SIPFROM));
    set_field_buffer(self->fields[FLD_FILTER_SIPTO], 0, filter_get(FILTER_SIPTO));
    set_field_buffer(self->fields[FLD_FILTER_SRC], 0, filter_get(FILTER_SOURCE));
    set_field_buffer(self->fields[FLD_FILTER_DST], 0, filter_get(FILTER_DESTINATION));
    set_field_buffer(self->fields[FLD_FILTER_PAYLOAD], 0, filter_get(FILTER_PAYLOAD));
    set_field_buffer(self->fields[FLD_FILTER_REGISTER], 0,
                     g_strrstr(method, sip_method_str(SIP_METHOD_REGISTER)) ? "*" : "");
    set_field_buffer(self->fields[FLD_FILTER_INVITE], 0,
                     g_strrstr(method, sip_method_str(SIP_METHOD_INVITE)) ? "*" : "");
    set_field_buffer(self->fields[FLD_FILTER_SUBSCRIBE], 0,
                     g_strrstr(method, sip_method_str(SIP_METHOD_SUBSCRIBE)) ? "*" : "");
    set_field_buffer(self->fields[FLD_FILTER_NOTIFY], 0,
                     g_strrstr(method, sip_method_str(SIP_METHOD_NOTIFY)) ? "*" : "");
    set_field_buffer(self->fields[FLD_FILTER_INFO], 0,
                     g_strrstr(method, sip_method_str(SIP_METHOD_INFO)) ? "*" : "");
    set_field_buffer(self->fields[FLD_FILTER_OPTIONS], 0,
                     g_strrstr(method, sip_method_str(SIP_METHOD_OPTIONS)) ? "*" : "");
    set_field_buffer(self->fields[FLD_FILTER_PUBLISH], 0,
                     g_strrstr(method, sip_method_str(SIP_METHOD_PUBLISH)) ? "*" : "");
    set_field_buffer(self->fields[FLD_FILTER_MESSAGE], 0,
                     g_strrstr(method, sip_method_str(SIP_METHOD_MESSAGE)) ? "*" : "");
    set_field_buffer(self->fields[FLD_FILTER_REFER], 0,
                     g_strrstr(method, sip_method_str(SIP_METHOD_REFER)) ? "*" : "");
    set_field_buffer(self->fields[FLD_FILTER_UPDATE], 0,
                     g_strrstr(method, sip_method_str(SIP_METHOD_UPDATE)) ? "*" : "");
    set_field_buffer(self->fields[FLD_FILTER_FILTER], 0, "[ Filter ]");
    set_field_buffer(self->fields[FLD_FILTER_CANCEL], 0, "[ Cancel ]");

    // Set the window title and boxes
    mvwprintw(win, 1, 18, "Filter options");
    wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    mvwhline(win, 8, 1, ACS_HLINE, 49);
    mvwaddch(win, 8, 0, ACS_LTEE);
    mvwaddch(win, 8, 49, ACS_RTEE);
    wattroff(win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Set default cursor position
    set_current_field(self->form, self->fields[FLD_FILTER_SIPFROM]);
    wmove(win, 3, 18);
    curs_set(1);
}

static void
filter_win_finalized(GObject *self)
{
    // Disable cursor
    curs_set(0);
    // Chain-up parent finalize function
    G_OBJECT_CLASS(filter_win_parent_class)->finalize(self);
}

static void
filter_win_class_init(FilterWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = filter_win_constructed;
    object_class->finalize = filter_win_finalized;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->key_pressed = filter_win_handle_key;

}

static void
filter_win_init(G_GNUC_UNUSED FilterWindow *self)
{
}
