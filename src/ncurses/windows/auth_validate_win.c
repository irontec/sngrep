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
#include "capture/dissectors/packet_sip.h"
#include "ncurses/dialog.h"
#include "auth_validate_win.h"

/**
 * @brief Get custom information of given panel
 *
 * Return ncurses users pointer of the given panel into panel's
 * information structure pointer.
 *
 * @param window UI structure pointer
 * @return a pointer to info structure of given panel
 */
static AuthValidateWinInfo *
auth_validate_info(Window *window)
{
    return (AuthValidateWinInfo *) panel_userptr(window->panel);
}

/**
 * @brief Draw the Auth validator panel
 *
 * This function will drawn the panel into the screen based on its stored
 * status
 *
 * @param window UI structure pointer
 * @return 0 if the panel has been drawn, -1 otherwise
 */
static int
auth_validate_draw(Window *window)
{
    // Get panel information
    AuthValidateWinInfo *info = auth_validate_info(window);
    g_return_val_if_fail(info != NULL, -1);

    // No message with Autorization header
    if (info->msg == NULL) {
        dialog_run("No Authorization header found in current dialog.");
        return -1;
    }

    if (info->algorithm == NULL || g_strcmp0(info->algorithm, "MD5") != 0) {
        dialog_run("Unsupported auth validation algorithm.");
        return -1;
    }

    // Set calculated color depending on expected result
    if (info->calculated != NULL) {
        if (g_strcmp0(info->response, info->calculated) == 0) {
            wattron(window->win, COLOR_PAIR(CP_GREEN_ON_DEF));
        } else {
            wattron(window->win, COLOR_PAIR(CP_RED_ON_DEF));
        }

        // Set current calculated value
        mvwprintw(window->win, 11, 15, "%s", info->calculated);
        wattroff(window->win, COLOR_PAIR(CP_GREEN_ON_DEF));
        wattroff(window->win, COLOR_PAIR(CP_RED_ON_DEF));
    }

    set_current_field(info->form, current_field(info->form));
    form_driver(info->form, REQ_VALIDATION);

    return 0;
}

static void
auth_validate_calulate(Window *window)
{
    // Get panel information
    AuthValidateWinInfo *info = auth_validate_info(window);
    g_return_if_fail(info != NULL);

    char password[100];
    memset(password, 0, sizeof(password));
    strcpy(password, field_buffer(info->fields[FLD_AUTH_PASS], 0));
    g_strchomp(password);

    g_autofree gchar *str1 = g_strdup_printf("%s:%s:%s", info->username, info->realm, password);
    g_return_if_fail(str1 != NULL);
    g_autofree gchar *md51 = g_compute_checksum_for_data(G_CHECKSUM_MD5, (guchar *) str1, (gsize) strlen(str1));
    g_return_if_fail(md51 != NULL);
    g_autofree gchar *str2 = g_strdup_printf("%s:%s", info->method, info->uri);
    g_return_if_fail(str2 != NULL);
    g_autofree gchar *md52 = g_compute_checksum_for_data(G_CHECKSUM_MD5, (guchar *) str2, (gsize) strlen(str2));
    g_return_if_fail(md52 != NULL);
    g_autofree gchar *str3 = g_strdup_printf("%s:%s:%s", md51, info->nonce, md52);
    g_return_if_fail(str3 != NULL);

    info->calculated = g_compute_checksum_for_data(G_CHECKSUM_MD5, (guchar *) str3, (gsize) strlen(str3));;
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
static int
auth_validate_handle_key(Window *window, int key)
{
    // Get panel information
    AuthValidateWinInfo *info = auth_validate_info(window);
    g_return_val_if_fail(info != NULL, KEY_NOT_HANDLED);

    // Get current field id
    gint field_idx = field_index(current_field(info->form));

    // Check actions for this key
    enum KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch (action) {
            case ACTION_PRINTABLE:
                form_driver(info->form, key);
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
            case ACTION_DELETE:
                form_driver(info->form, REQ_DEL_CHAR);
                break;
            case ACTION_BACKSPACE:
                form_driver(info->form, REQ_DEL_PREV);
                break;
            case ACTION_CLEAR:
                form_driver(info->form, REQ_CLR_FIELD);
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
    form_driver(info->form, REQ_VALIDATION);

    // Change background and cursor of "button fields"
    set_field_back(info->fields[FLD_AUTH_CLOSE], A_NORMAL);
    curs_set(1);

    // Change current field background
    field_idx = field_index(current_field(info->form));
    if (field_idx == FLD_AUTH_CLOSE) {
        set_field_back(info->fields[field_idx], A_REVERSE);
        curs_set(0);
    }

    // Calculate hash based on current password
    if (field_idx == FLD_AUTH_PASS) {
        auth_validate_calulate(window);
    }

    // Return if this panel has handled or not the key
    return (action == ERR) ? KEY_NOT_HANDLED : KEY_HANDLED;
}

void
auth_validate_set_group(Window *window, CallGroup *group)
{
    // Get panel information
    AuthValidateWinInfo *info = auth_validate_info(window);
    g_return_if_fail(info != NULL);
    g_return_if_fail(group != NULL);

    Message *msg = NULL;
    while ((msg = call_group_get_next_msg(group, msg)) != NULL) {
        if (!msg_is_request(msg))
            continue;

        if (msg->request.auth != NULL) {
            auth_validate_set_msg(window, msg);
            break;
        }
    }
}

void
auth_validate_set_msg(Window *window, Message *msg)
{
    // Get panel information
    AuthValidateWinInfo *info = auth_validate_info(window);
    g_return_if_fail(info != NULL);
    g_return_if_fail(msg != NULL);

    // Authorization is only checked in request messages
    if (!msg_is_request(msg))
        return;

    info->method = msg->request.method;

    GRegex *auth_param = g_regex_new(
        "^(?P<authhdrname>\\w+)=\"?(?P<authhdrvalue>[^\"]+)\"?",
        G_REGEX_OPTIMIZE | G_REGEX_CASELESS, G_REGEX_MATCH_NEWLINE_CR, NULL);

    gchar *auth_value = g_strdup(msg->request.auth);
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
                info->username = authhdrvalue;
            } else if (g_strcmp0(authhdrname, "realm") == 0) {
                info->realm = authhdrvalue;
            } else if (g_strcmp0(authhdrname, "uri") == 0) {
                info->uri = authhdrvalue;
            } else if (g_strcmp0(authhdrname, "algorithm") == 0) {
                info->algorithm = authhdrvalue;
            } else if (g_strcmp0(authhdrname, "nonce") == 0) {
                info->nonce = authhdrvalue;
            } else if (g_strcmp0(authhdrname, "response") == 0) {
                info->response = authhdrvalue;
            }
        }
        g_match_info_free(pmatch);
    }

    // Set screen labels
    mvwprintw(window->win, 3, 11, info->method);
    mvwprintw(window->win, 4, 13, info->username);
    mvwprintw(window->win, 5, 10, info->realm);
    mvwprintw(window->win, 6, 14, info->algorithm);
    mvwprintw(window->win, 7, 15, info->response);
    mvwprintw(window->win, 8, 8, info->uri);

    // Set the message being check
    info->msg = msg;
}

void
auth_validate_win_free(Window *window)
{
    AuthValidateWinInfo *info;
    int i;

    // Get panel information
    if ((info = auth_validate_info(window))) {
        // Remove panel form and fields
        unpost_form(info->form);
        free_form(info->form);
        for (i = 0; i < FLD_AUTH_COUNT; i++)
            free_field(info->fields[i]);

        // Remove panel window and custom info
        g_free(info);
    }

    // Delete panel
    window_deinit(window);

    // Disable cursor position
    curs_set(0);
}

Window *
auth_validate_win_new()
{
    Window *window = g_malloc0(sizeof(Window));
    window->type = WINDOW_SAVE;
    window->destroy = auth_validate_win_free;
    window->draw = auth_validate_draw;
    window->handle_key = auth_validate_handle_key;

    // Cerate a new indow for the panel and form
    window_init(window, 15, 68);

    // Initialize save panel specific data
    AuthValidateWinInfo *info = g_malloc0(sizeof(AuthValidateWinInfo));
    set_panel_userptr(window->panel, (void *) info);

    // Initialize the fields    int total, displayed;
    info->fields[FLD_AUTH_PASS] = new_field(1, 50, 10, 13, 0, 0);
    info->fields[FLD_AUTH_CLOSE] = new_field(1, 9, window->height - 2, 27, 0, 0);
    info->fields[FLD_AUTH_COUNT] = NULL;

    // Set fields options
    field_opts_off(info->fields[FLD_AUTH_PASS], O_STATIC);
    field_opts_off(info->fields[FLD_AUTH_PASS], O_AUTOSKIP);
    set_max_field(info->fields[FLD_AUTH_PASS], 50);
    set_field_back(info->fields[FLD_AUTH_PASS], A_UNDERLINE);

    // Create the form and post it
    info->form = new_form(info->fields);
    set_form_sub(info->form, window->win);
    post_form(info->form);
    form_opts_off(info->form, O_BS_OVERLOAD);

    // Set Default field values
    char savepath[SETTING_MAX_LEN];
    sprintf(savepath, "%s", setting_get_value(SETTING_SAVEPATH));

    set_field_buffer(info->fields[FLD_AUTH_CLOSE], 0, "[ Close ]");

    // Set window boxes
    wattron(window->win, COLOR_PAIR(CP_BLUE_ON_DEF));
    // Window border
    title_foot_box(window->panel);

    // Header and footer lines
    mvwhline(window->win, window->height - 3, 1, ACS_HLINE, window->width - 1);
    mvwaddch(window->win, window->height - 3, 0, ACS_LTEE);
    mvwaddch(window->win, window->height - 3, window->width - 1, ACS_RTEE);
    wattroff(window->win, COLOR_PAIR(CP_BLUE_ON_DEF));

    wattron(window->win, COLOR_PAIR(CP_GREEN_ON_DEF));
    mvwprintw(window->win, 3, 3, "Method:");
    mvwprintw(window->win, 4, 3, "Username:");
    mvwprintw(window->win, 5, 3, "Realm:");
    mvwprintw(window->win, 6, 3, "Algorithm:");
    mvwprintw(window->win, 7, 3, "Response:");
    mvwprintw(window->win, 8, 3, "URI:");
    mvwprintw(window->win, 10, 3, "Password:");
    mvwprintw(window->win, 11, 3, "Calculated:");
    wattroff(window->win, COLOR_PAIR(CP_GREEN_ON_DEF));

    // Window title
    mvwprintw(window->win, 1, 20, "Authorization validator");

    // Set default cursor position
    set_current_field(info->form, info->fields[FLD_AUTH_PASS]);
    form_driver(info->form, REQ_END_LINE);
    curs_set(1);

    return window;
}
