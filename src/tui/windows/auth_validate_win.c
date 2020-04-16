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
 * @file auth_validate_win.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in auth_validate_win.h
 */
#include "config.h"
#include <glib.h>
#include <storage/message.h>
#include "tui/dialog.h"
#include "auth_validate_win.h"

G_DEFINE_TYPE(AuthValidateWindow, auth_validate_win, SNG_TYPE_WINDOW)

/**
 * @brief Draw the Auth validator panel
 *
 * This function will drawn the panel into the screen based on its stored
 * status
 *
 * @param window UI structure pointer
 * @return 0 if the panel has been drawn, -1 otherwise
 */
static gint
auth_validate_win_draw(SngWidget *widget)
{
    // Get panel information
    SngWindow *window = SNG_WINDOW(widget);
    AuthValidateWindow *self = TUI_AUTH_VALIDATE(window);
    g_return_val_if_fail(self != NULL, -1);

    WINDOW *win = sng_window_get_ncurses_window(window);

    // No message with Authorization header
    if (self->msg == NULL) {
        dialog_run("No Authorization header found in current dialog.");
        return -1;
    }

    if (self->algorithm == NULL || g_strcmp0(self->algorithm, "MD5") != 0) {
        dialog_run("Unsupported auth validation algorithm.");
        return -1;
    }

    // Set calculated color depending on expected result
    if (self->calculated != NULL) {
        if (g_strcmp0(self->response, self->calculated) == 0) {
            wattron(win, COLOR_PAIR(CP_GREEN_ON_DEF));
        } else {
            wattron(win, COLOR_PAIR(CP_RED_ON_DEF));
        }

        // Set current calculated value
        mvwprintw(win, 11, 15, "%s", self->calculated);
        wattroff(win, COLOR_PAIR(CP_GREEN_ON_DEF));
        wattroff(win, COLOR_PAIR(CP_RED_ON_DEF));
    }

    set_current_field(self->form, current_field(self->form));
    form_driver(self->form, REQ_VALIDATION);

    return 0;
}

static void
auth_validate_win_calculate(SngWindow *window)
{
    // Get panel information
    AuthValidateWindow *self = TUI_AUTH_VALIDATE(window);
    g_return_if_fail(self != NULL);

    char password[100];
    memset(password, 0, sizeof(password));
    strcpy(password, field_buffer(self->fields[FLD_AUTH_PASS], 0));
    g_strchomp(password);

    g_autofree gchar *str1 = g_strdup_printf("%s:%s:%s", self->username, self->realm, password);
    g_return_if_fail(str1 != NULL);
    g_autofree gchar *md51 = g_compute_checksum_for_data(G_CHECKSUM_MD5, (guchar *) str1, (gsize) strlen(str1));
    g_return_if_fail(md51 != NULL);
    g_autofree gchar *str2 = g_strdup_printf("%s:%s", self->method, self->uri);
    g_return_if_fail(str2 != NULL);
    g_autofree gchar *md52 = g_compute_checksum_for_data(G_CHECKSUM_MD5, (guchar *) str2, (gsize) strlen(str2));
    g_return_if_fail(md52 != NULL);
    g_autofree gchar *str3 = g_strdup_printf("%s:%s:%s", md51, self->nonce, md52);
    g_return_if_fail(str3 != NULL);

    self->calculated = g_compute_checksum_for_data(G_CHECKSUM_MD5, (guchar *) str3, (gsize) strlen(str3));;
}

/**
 * @brief Manage pressed keys for save panel
 *
 * This function is called by UI manager every time a
 * key is pressed. This allow the save panel to manage
 * its own keys.
 *
 * @param window UI structure pointer
 * @param key   key code
 * @return enum @key_handler_ret
 */
static gint
auth_validate_win_handle_key(SngWidget *widget, gint key)
{
    // Get panel information
    SngWindow *window = SNG_WINDOW(widget);
    AuthValidateWindow *self = TUI_AUTH_VALIDATE(window);
    g_return_val_if_fail(self != NULL, KEY_NOT_HANDLED);

    // Get current field id
    gint field_idx = field_index(current_field(self->form));

    // Check actions for this key
    KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch (action) {
            case ACTION_PRINTABLE:
                form_driver(self->form, key);
                break;
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
            case ACTION_DELETE:
                form_driver(self->form, REQ_DEL_CHAR);
                break;
            case ACTION_BACKSPACE:
                form_driver(self->form, REQ_DEL_PREV);
                break;
            case ACTION_CLEAR:
                form_driver(self->form, REQ_CLR_FIELD);
                break;
            case ACTION_CONFIRM:
                if (field_idx == FLD_AUTH_CLOSE) {
                    return KEY_DESTROY;
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
    set_field_back(self->fields[FLD_AUTH_CLOSE], A_NORMAL);
    curs_set(1);

    // Change current field background
    field_idx = field_index(current_field(self->form));
    if (field_idx == FLD_AUTH_CLOSE) {
        set_field_back(self->fields[field_idx], A_REVERSE);
        curs_set(0);
    }

    // Calculate hash based on current password
    if (field_idx == FLD_AUTH_PASS) {
        auth_validate_win_calculate(window);
    }

    // Return if this panel has handled or not the key
    return (action == ERR) ? KEY_NOT_HANDLED : KEY_HANDLED;
}

void
auth_validate_win_set_group(SngWindow *window, CallGroup *group)
{
    // Get panel information
    AuthValidateWindow *self = TUI_AUTH_VALIDATE(window);
    g_return_if_fail(self != NULL);
    g_return_if_fail(group != NULL);

    Message *msg = NULL;
    while ((msg = call_group_get_next_msg(group, msg)) != NULL) {
        if (!msg_is_request(msg))
            continue;

        if (msg_get_auth_hdr(msg) != NULL) {
            auth_validate_win_set_msg(window, msg);
            break;
        }
    }
}

void
auth_validate_win_set_msg(SngWindow *window, Message *msg)
{
    // Get panel information
    AuthValidateWindow *self = TUI_AUTH_VALIDATE(window);
    g_return_if_fail(self != NULL);
    g_return_if_fail(msg != NULL);
    WINDOW *win = sng_window_get_ncurses_window(window);

    // Authorization is only checked in request messages
    if (!msg_is_request(msg))
        return;

    self->method = msg_get_method_str(msg);

    GRegex *auth_param = g_regex_new(
        "^(?P<authhdrname>\\w+)=\"?(?P<authhdrvalue>[^\"]+)\"?",
        G_REGEX_OPTIMIZE | G_REGEX_CASELESS, G_REGEX_MATCH_NEWLINE_CR, NULL);

    gchar *auth_value = g_strdup(msg_get_auth_hdr(msg));
    if (strncasecmp(auth_value, "Digest", 6) == 0) {
        auth_value += 6;
    }

    gchar **auth_data = g_strsplit(auth_value, ",", -1);
    for (guint i = 0; i < g_strv_length(auth_data); i++) {
        // Trim parameter string
        g_strstrip(auth_data[i]);

        // Try to get Call-ID from payload
        GMatchInfo *pmatch;
        g_regex_match(auth_param, auth_data[i], 0, &pmatch);
        if (g_match_info_matches(pmatch)) {
            const gchar *authhdrname = g_match_info_fetch_named(pmatch, "authhdrname");
            const gchar *authhdrvalue = g_match_info_fetch_named(pmatch, "authhdrvalue");

            if (g_strcmp0(authhdrname, "username") == 0) {
                self->username = authhdrvalue;
            } else if (g_strcmp0(authhdrname, "realm") == 0) {
                self->realm = authhdrvalue;
            } else if (g_strcmp0(authhdrname, "uri") == 0) {
                self->uri = authhdrvalue;
            } else if (g_strcmp0(authhdrname, "algorithm") == 0) {
                self->algorithm = authhdrvalue;
            } else if (g_strcmp0(authhdrname, "nonce") == 0) {
                self->nonce = authhdrvalue;
            } else if (g_strcmp0(authhdrname, "response") == 0) {
                self->response = authhdrvalue;
            }
        }
        g_match_info_free(pmatch);
    }

    // Set screen labels
    mvwprintw(win, 3, 11, self->method);
    mvwprintw(win, 4, 13, self->username);
    mvwprintw(win, 5, 10, self->realm);
    mvwprintw(win, 6, 14, self->algorithm);
    mvwprintw(win, 7, 15, self->response);
    mvwprintw(win, 8, 8, self->uri);

    // Set the message being check
    self->msg = msg;
}

void
auth_validate_win_free(SngWindow *window)
{
    g_object_unref(window);
}

static void
auth_validate_win_finalize(GObject *object)
{
    AuthValidateWindow *self = TUI_AUTH_VALIDATE(object);

    // Remove panel form and fields
    unpost_form(self->form);
    free_form(self->form);
    for (gint i = 0; i < FLD_AUTH_COUNT; i++)
        free_field(self->fields[i]);

    // Disable cursor position
    curs_set(0);

    // Chain-up parent finalize function
    G_OBJECT_CLASS(auth_validate_win_parent_class)->finalize(object);
}

SngWindow *
auth_validate_win_new()
{
    return g_object_new(
        WINDOW_TYPE_AUTH_VALIDATE,
        "height", 15,
        "width", 68,
        NULL
    );
}

static void
auth_validate_win_constructed(GObject *object)
{
    // Chain-up parent constructed
    G_OBJECT_CLASS(auth_validate_win_parent_class)->constructed(object);

    // Get parent window information
    AuthValidateWindow *self = TUI_AUTH_VALIDATE(object);
    SngWindow *parent = SNG_WINDOW(self);
    WINDOW *win = sng_window_get_ncurses_window(parent);
    PANEL *panel = sng_window_get_ncurses_panel(parent);

    gint height = sng_window_get_height(parent);
    gint width = sng_window_get_width(parent);

    // Initialize the fields    int total, displayed;
    self->fields[FLD_AUTH_PASS] = new_field(1, 50, 10, 13, 0, 0);
    self->fields[FLD_AUTH_CLOSE] = new_field(1, 9, height - 2, 27, 0, 0);
    self->fields[FLD_AUTH_COUNT] = NULL;

    // Set fields options
    field_opts_off(self->fields[FLD_AUTH_PASS], O_STATIC);
    field_opts_off(self->fields[FLD_AUTH_PASS], O_AUTOSKIP);
    set_max_field(self->fields[FLD_AUTH_PASS], 50);
    set_field_back(self->fields[FLD_AUTH_PASS], A_UNDERLINE);

    // Create the form and post it
    self->form = new_form(self->fields);
    set_form_sub(self->form, win);
    post_form(self->form);
    form_opts_off(self->form, O_BS_OVERLOAD);

    // Set Default field values
    char savepath[SETTING_MAX_LEN];
    sprintf(savepath, "%s", setting_get_value(SETTING_STORAGE_SAVEPATH));

    set_field_buffer(self->fields[FLD_AUTH_CLOSE], 0, "[ Close ]");

    // Set window boxes
    wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    // Window border
    title_foot_box(panel);

    // Header and footer lines
    mvwhline(win, height - 3, 1, ACS_HLINE, width - 1);
    mvwaddch(win, height - 3, 0, ACS_LTEE);
    mvwaddch(win, height - 3, width - 1, ACS_RTEE);
    wattroff(win, COLOR_PAIR(CP_BLUE_ON_DEF));

    wattron(win, COLOR_PAIR(CP_GREEN_ON_DEF));
    mvwprintw(win, 3, 3, "Method:");
    mvwprintw(win, 4, 3, "Username:");
    mvwprintw(win, 5, 3, "Realm:");
    mvwprintw(win, 6, 3, "Algorithm:");
    mvwprintw(win, 7, 3, "Response:");
    mvwprintw(win, 8, 3, "URI:");
    mvwprintw(win, 10, 3, "Password:");
    mvwprintw(win, 11, 3, "Calculated:");
    wattroff(win, COLOR_PAIR(CP_GREEN_ON_DEF));

    // Window title
    mvwprintw(win, 1, 20, "Authorization validator");

    // Set default cursor position
    set_current_field(self->form, self->fields[FLD_AUTH_PASS]);
    form_driver(self->form, REQ_END_LINE);
    curs_set(1);
}

static void
auth_validate_win_class_init(AuthValidateWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = auth_validate_win_constructed;
    object_class->finalize = auth_validate_win_finalize;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->draw = auth_validate_win_draw;
    widget_class->key_pressed = auth_validate_win_handle_key;

}

static void
auth_validate_win_init(AuthValidateWindow *self)
{
    // Initialize attributes
    sng_window_set_window_type(SNG_WINDOW(self), WINDOW_AUTH_VALIDATE);
}
