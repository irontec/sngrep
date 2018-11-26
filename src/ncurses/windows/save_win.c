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
 * @file save_win.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in save_win.h
 */
#include "config.h"
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <form.h>
#include <ctype.h>
#include "glib-utils.h"
#include "save_win.h"
#include "setting.h"
#include "capture/capture_pcap.h"
#include "capture/capture_txt.h"
#include "filter.h"

/**
 * @brief Get custom information of given panel
 *
 * Return ncurses users pointer of the given panel into panel's
 * information structure pointer.
 *
 * @param window UI structure pointer
 * @return a pointer to info structure of given panel
 */
static SaveWinInfo *
save_info(Window *window)
{
    return (SaveWinInfo*) panel_userptr(window->panel);
}

/**
 * @brief Draw the Save panel
 *
 * This function will drawn the panel into the screen based on its stored
 * status
 *
 * @param window UI structure pointer
 * @return 0 if the panel has been drawn, -1 otherwise
 */
static int
save_draw(Window *window)
{
    char field_value[SETTING_MAX_LEN];

    // Get panel information
    SaveWinInfo *info = save_info(window);
    g_return_val_if_fail(info != NULL, -1);

    // Get filter stats
    sip_stats_t stats = storage_calls_stats();

    mvwprintw(window->win, 7, 3, "( ) all dialogs ");
    mvwprintw(window->win, 8, 3, "( ) selected dialogs [%d]", call_group_count(info->group));
    mvwprintw(window->win, 9, 3, "( ) filtered dialogs [%d]", stats.displayed);

    // Print 'current SIP message' field label if required
    if (info->msg != NULL)
        mvwprintw(window->win, 10, 3, "( ) current SIP message");

    mvwprintw(window->win, 7, 35, "( ) .pcap (SIP)");
    mvwprintw(window->win, 8, 35, "( ) .pcap (SIP + RTP)");
    mvwprintw(window->win, 9, 35, "( ) .txt");

    // Get filename field value.
    memset(field_value, 0, sizeof(field_value));
    strcpy(field_value, field_buffer(info->fields[FLD_SAVE_FILE], 0));
    g_strstrip(field_value);

    mvwprintw(window->win, 4, 60, "     ");
    if (strstr(field_value, ".pcap")) {
        info->saveformat = (setting_enabled(SETTING_CAPTURE_RTP))? SAVE_PCAP_RTP : SAVE_PCAP;
    } else if (strstr(field_value, ".txt")) {
        info->saveformat = SAVE_TXT;
    } else {
        if (info->saveformat == SAVE_PCAP || info->saveformat == SAVE_PCAP_RTP)
            mvwprintw(window->win, 4, 60, ".pcap");
        else
            mvwprintw(window->win, 4, 60, ".txt ");
    }

    set_field_buffer(info->fields[FLD_SAVE_ALL], 0, (info->savemode == SAVE_ALL) ? "*" : " ");
    set_field_buffer(info->fields[FLD_SAVE_SELECTED], 0,
                     (info->savemode == SAVE_SELECTED) ? "*" : " ");
    set_field_buffer(info->fields[FLD_SAVE_DISPLAYED], 0,
                     (info->savemode == SAVE_DISPLAYED) ? "*" : " ");
    set_field_buffer(info->fields[FLD_SAVE_MESSAGE], 0,
                     (info->savemode == SAVE_MESSAGE) ? "*" : " ");
    set_field_buffer(info->fields[FLD_SAVE_PCAP], 0, (info->saveformat == SAVE_PCAP) ? "*" : " ");
    set_field_buffer(info->fields[FLD_SAVE_PCAP_RTP], 0, (info->saveformat == SAVE_PCAP_RTP) ? "*" : " ");
    set_field_buffer(info->fields[FLD_SAVE_TXT], 0, (info->saveformat == SAVE_TXT) ? "*" : " ");

    // Show disabled options with makers
    if (!setting_enabled(SETTING_CAPTURE_RTP))
        set_field_buffer(info->fields[FLD_SAVE_PCAP_RTP], 0, "-");

    set_current_field(info->form, current_field(info->form));
    form_driver(info->form, REQ_VALIDATION);

    return 0;
}

/**
 * @brief Callback function to save a single packet into a capture output
 */
static void
save_packet_cb(Packet *packet, CaptureOutput *output)
{
    output->write(output, packet);
}

/**
 * @brief Save form data to options
 *
 * Save capture packets to a file based on selected modes on screen
 * It will display an error or success dialog before exit
 *
 * @param window UI structure pointer
 * @returns 1 in case of error, 0 otherwise.
 */
static int
save_to_file(Window *window)
{
    char savepath[SETTING_MAX_LEN];
    char savefile[SETTING_MAX_LEN];
    char fullfile[SETTING_MAX_LEN*2];
    CaptureOutput *output = NULL;
    int cur = 0, total = 0;
    WINDOW *progress;
    GPtrArray *calls = NULL;
    GPtrArray *packets = g_ptr_array_new();
    GError *error = NULL;

    // Get panel information
    SaveWinInfo *info = save_info(window);

    // Get current path field value.
    memset(savepath, 0, sizeof(savepath));
    strcpy(savepath, field_buffer(info->fields[FLD_SAVE_PATH], 0));
    g_strstrip(savepath);
    if (strlen(savepath))
        strcat(savepath, "/");

    // Get current file field value.
    memset(savefile, 0, sizeof(savefile));
    strcpy(savefile, field_buffer(info->fields[FLD_SAVE_FILE], 0));
    g_strstrip(savefile);

    if (!strlen(savefile)) {
        dialog_run("Please enter a valid filename");
        return 1;
    }

    if (info->saveformat == SAVE_PCAP || info->saveformat == SAVE_PCAP_RTP) {
        if (!strstr(savefile, ".pcap"))
            strcat(savefile, ".pcap");
    } else {
        if (!strstr(savefile, ".txt"))
            strcat(savefile, ".txt");
    }

    // Absolute filename
    sprintf(fullfile, "%s%s", savepath, savefile);

    if (access(fullfile, R_OK) == 0) {
        if (dialog_confirm("Overwrite confirmation",
                           "Selected file already exits.\n Do you want to overwrite it?",
                           "Yes,No") != 0)
            return 1;
    }

    // Don't allow to save no packets!
    if (info->savemode == SAVE_SELECTED && call_group_msg_count(info->group) == 0) {
        dialog_run("Unable to save: No selected dialogs.");
        return 1;
    }

    if (info->saveformat == SAVE_PCAP || info->saveformat == SAVE_PCAP_RTP) {
        // Open PCAP ouptut file
        output = capture_output_pcap(fullfile, &error);
    } else {
        // Open TXT output file
        output = capture_output_txt(fullfile, &error);
    }

    // Output creation error checking
    if (error != NULL) {
        dialog_run("Error: %s", error->message);
        return 1;
    }

    // Get calls iterator
    switch (info->savemode) {
        case SAVE_ALL:
        case SAVE_DISPLAYED:
            // Get calls iterator
            calls = storage_calls();
            break;
        case SAVE_SELECTED:
            // Save selected packets to file
            calls = info->group->calls;
            break;
        default:
            break;
    }

    if (info->savemode == SAVE_MESSAGE) {
        // Save selected message packet to pcap
        output->write(output, info->msg->packet);
    } else if (info->saveformat == SAVE_TXT) {
        // Save selected packets to file
        for (guint i = 0; i < g_ptr_array_len(calls); i++) {
            Call *call = g_ptr_array_index(calls, i);
            // Save SIP message content
            for (guint j = 0; j < g_ptr_array_len(call->msgs); j++) {
                Message *msg = g_ptr_array_index(call->msgs, j);
                output->write(output, msg->packet);
            }
        }
    } else {
        // Count packages for progress bar
        for (guint i = 0; i < g_ptr_array_len(calls); i++) {
            Call *call = g_ptr_array_index(calls, i);

            if (info->savemode == SAVE_DISPLAYED && !filter_check_call(call, NULL))
                continue;

            total += call_msg_count(call);

            if (info->saveformat == SAVE_PCAP_RTP)
                total += g_sequence_get_length(call->rtp_packets);
        }

        progress = dialog_progress_run("Saving packets...");
        dialog_progress_set_value(progress, 0);

        // Save selected packets to file
        for (guint i = 0; i < g_ptr_array_len(calls); i++) {
            Call *call = g_ptr_array_index(calls, i);

            if (info->savemode == SAVE_DISPLAYED && !filter_check_call(call, NULL))
                continue;

            // Save SIP message content
            for (guint j = 0; j < g_ptr_array_len(call->msgs); j++) {
                Message * msg = g_ptr_array_index(call->msgs, j);
                // Update progress bar dialog
                dialog_progress_set_value(progress, (++cur * 100) / total);
                g_ptr_array_add(packets, msg->packet);
            }

            // Save RTP packets
            if (info->saveformat == SAVE_PCAP_RTP) {
                GSequenceIter *rtps = g_sequence_get_begin_iter(call->rtp_packets);
                for (;!g_sequence_iter_is_end(rtps); rtps = g_sequence_iter_next(rtps)) {
                    // Update progress bar dialog
                    dialog_progress_set_value(progress, (++cur * 100) / total);
                    g_ptr_array_add(packets, g_sequence_get(rtps));
                }
            }
        }

        g_ptr_array_sort(packets, (GCompareFunc) capture_packet_time_sorter);
        g_ptr_array_foreach(packets, (GFunc) save_packet_cb, output);

        dialog_progress_destroy(progress);
    }

    // Close saved file
    output->close(output);

    // Show success popup
    if (info->savemode == SAVE_MESSAGE) {
        dialog_run("Successfully saved selected SIP message to %s", savefile);
    } else {
        dialog_run("Successfully saved %d dialogs to %s", total, savefile);
    }

    return 0;
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
save_handle_key(Window *window, int key)
{
    // Get panel information
    SaveWinInfo *info = save_info(window);
    g_return_val_if_fail(info != NULL, KEY_NOT_HANDLED);

    // Get current field id
    gint field_idx = field_index(current_field(info->form));

    // Check actions for this key
    enum KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch (action) {
            case ACTION_PRINTABLE:
                if (field_idx == FLD_SAVE_PATH || field_idx == FLD_SAVE_FILE) {
                    form_driver(info->form, key);
                    break;
                }
                continue;
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
            case ACTION_SELECT:
                switch (field_idx) {
                    case FLD_SAVE_ALL:
                        info->savemode = SAVE_ALL;
                        break;
                    case FLD_SAVE_SELECTED:
                        info->savemode = SAVE_SELECTED;
                        break;
                    case FLD_SAVE_DISPLAYED:
                        info->savemode = SAVE_DISPLAYED;
                        break;
                    case FLD_SAVE_MESSAGE:
                        info->savemode = SAVE_MESSAGE;
                        break;
                    case FLD_SAVE_PCAP:
                        info->saveformat = SAVE_PCAP;
                        break;
                    case FLD_SAVE_PCAP_RTP:
                        info->saveformat = SAVE_PCAP_RTP;
                        break;
                    case FLD_SAVE_TXT:
                        info->saveformat = SAVE_TXT;
                        break;
                    case FLD_SAVE_FILE:
                        form_driver(info->form, key);
                        break;
                    default:
                        break;
                }
                break;
            case ACTION_CONFIRM:
                if (field_idx != FLD_SAVE_CANCEL) {
                    save_to_file(window);
                }
                return KEY_DESTROY;
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
    set_field_back(info->fields[FLD_SAVE_SAVE], A_NORMAL);
    set_field_back(info->fields[FLD_SAVE_CANCEL], A_NORMAL);
    curs_set(1);

    // Change current field background
    field_idx = field_index(current_field(info->form));
    if (field_idx == FLD_SAVE_SAVE || field_idx == FLD_SAVE_CANCEL) {
        set_field_back(info->fields[field_idx], A_REVERSE);
        curs_set(0);
    }

    // Return if this panel has handled or not the key
    return (action == ERR) ? KEY_NOT_HANDLED : KEY_HANDLED;
}

void
save_set_group(Window *window, CallGroup *group)
{
    // Get panel information
    SaveWinInfo *info = save_info(window);
    g_return_if_fail(info != NULL);
    g_return_if_fail(group != NULL);

    info->group = group;
    if (call_group_count(group)) {
        info->savemode = SAVE_SELECTED;
    }
}

void
save_set_msg(Window *window, Message *msg)
{
    // Get panel information
    SaveWinInfo *info = save_info(window);
    g_return_if_fail(info != NULL);
    g_return_if_fail(msg != NULL);

    info->msg = msg;

    // make 'current SIP message' field visible
    field_opts_on(info->fields[FLD_SAVE_MESSAGE], O_VISIBLE);
}

void
save_win_free(Window *window)
{
    SaveWinInfo *info;
    int i;

    // Get panel information
    if ((info = save_info(window))) {
        // Remove panel form and fields
        unpost_form(info->form);
        free_form(info->form);
        for (i = 0; i < FLD_SAVE_COUNT; i++)
            free_field(info->fields[i]);

        // Remove panel window and custom info
        g_free(info);
    }

    // Delete panel
    window_deinit(window);

    // Resume capture
    capture_manager_set_pause(capture_manager(), FALSE);

    // Disable cursor position
    curs_set(0);
}

Window *
save_win_new()
{
    Window *window = g_malloc0(sizeof(Window));
    window->type = WINDOW_SAVE;
    window->destroy = save_win_free;
    window->draw = save_draw;
    window->handle_key = save_handle_key;

    // Cerate a new indow for the panel and form
    window_init(window, 15, 68);

    // Pause the capture while saving
    capture_manager_set_pause(capture_manager(), TRUE);

    // Initialize save panel specific data
    SaveWinInfo *info = g_malloc0(sizeof(SaveWinInfo));
    set_panel_userptr(window->panel, (void*) info);

    // Initialize the fields    int total, displayed;
    info->fields[FLD_SAVE_PATH]         = new_field(1, 52, 3, 13, 0, 0);
    info->fields[FLD_SAVE_FILE]         = new_field(1, 47, 4, 13, 0, 0);
    info->fields[FLD_SAVE_ALL]          = new_field(1, 1, 7, 4, 0, 0);
    info->fields[FLD_SAVE_SELECTED]     = new_field(1, 1, 8, 4, 0, 0);
    info->fields[FLD_SAVE_DISPLAYED]    = new_field(1, 1, 9, 4, 0, 0);
    info->fields[FLD_SAVE_MESSAGE]      = new_field(1, 1, 10, 4, 0, 0);
    info->fields[FLD_SAVE_PCAP]         = new_field(1, 1, 7, 36, 0, 0);
    info->fields[FLD_SAVE_PCAP_RTP]     = new_field(1, 1, 8, 36, 0, 0);
    info->fields[FLD_SAVE_TXT]          = new_field(1, 1, 9, 36, 0, 0);
    info->fields[FLD_SAVE_SAVE]         = new_field(1, 10, window->height - 2, 20, 0, 0);
    info->fields[FLD_SAVE_CANCEL]       = new_field(1, 10, window->height - 2, 40, 0, 0);
    info->fields[FLD_SAVE_COUNT]        = NULL;

    // Set fields options
    field_opts_off(info->fields[FLD_SAVE_PATH], O_STATIC);
    field_opts_off(info->fields[FLD_SAVE_PATH], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_SAVE_FILE], O_STATIC);
    field_opts_off(info->fields[FLD_SAVE_FILE], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_SAVE_ALL], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_SAVE_SELECTED], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_SAVE_DISPLAYED], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_SAVE_MESSAGE], O_VISIBLE);

    // Limit max save path and file length
    set_max_field(info->fields[FLD_SAVE_PATH], SETTING_MAX_LEN);
    set_max_field(info->fields[FLD_SAVE_FILE], SETTING_MAX_LEN);

    // Change background of input fields
    set_field_back(info->fields[FLD_SAVE_PATH], A_UNDERLINE);
    set_field_back(info->fields[FLD_SAVE_FILE], A_UNDERLINE);

    // Disable Save RTP if RTP packets are not being captured
    if (!setting_enabled(SETTING_CAPTURE_RTP))
        field_opts_off(info->fields[FLD_SAVE_PCAP_RTP], O_ACTIVE);

    // Create the form and post it
    info->form = new_form(info->fields);
    set_form_sub(info->form, window->win);
    post_form(info->form);
    form_opts_off(info->form, O_BS_OVERLOAD);

    // Set Default field values
    char savepath[SETTING_MAX_LEN];
    sprintf(savepath, "%s", setting_get_value(SETTING_SAVEPATH));

    set_field_buffer(info->fields[FLD_SAVE_PATH], 0, savepath);
    set_field_buffer(info->fields[FLD_SAVE_SAVE], 0, "[  Save  ]");
    set_field_buffer(info->fields[FLD_SAVE_CANCEL], 0, "[ Cancel ]");

    // Set window boxes
    wattron(window->win, COLOR_PAIR(CP_BLUE_ON_DEF));
    // Window border
    title_foot_box(window->panel);

    // Header and footer lines
    mvwhline(window->win, window->height - 3, 1, ACS_HLINE, window->width - 1);
    mvwaddch(window->win, window->height - 3, 0, ACS_LTEE);
    mvwaddch(window->win, window->height - 3, window->width - 1, ACS_RTEE);

    // Save mode box
    mvwaddch(window->win, 6, 2, ACS_ULCORNER);
    mvwhline(window->win, 6, 3, ACS_HLINE, 30);
    mvwaddch(window->win, 6, 32, ACS_URCORNER);
    mvwvline(window->win, 7, 2, ACS_VLINE, 4);
    mvwvline(window->win, 7, 32, ACS_VLINE, 4);
    mvwaddch(window->win, 11, 2, ACS_LLCORNER);
    mvwhline(window->win, 11, 3, ACS_HLINE, 30);
    mvwaddch(window->win, 11, 32, ACS_LRCORNER);

    // Save mode box
    mvwaddch(window->win, 6, 34, ACS_ULCORNER);
    mvwhline(window->win, 6, 35, ACS_HLINE, 30);
    mvwaddch(window->win, 6, 64, ACS_URCORNER);
    mvwvline(window->win, 7, 34, ACS_VLINE, 3);
    mvwvline(window->win, 7, 64, ACS_VLINE, 3);
    mvwaddch(window->win, 10, 34, ACS_LLCORNER);
    mvwhline(window->win, 10, 35, ACS_HLINE, 30);
    mvwaddch(window->win, 10, 64, ACS_LRCORNER);

    wattroff(window->win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Set screen labels
    mvwprintw(window->win, 1, 27, "Save capture");
    mvwprintw(window->win, 3, 3, "Path:");
    mvwprintw(window->win, 4, 3, "Filename:");
    wattron(window->win, COLOR_PAIR(CP_BLUE_ON_DEF));
    mvwprintw(window->win, 6, 4, " Dialogs ");
    mvwprintw(window->win, 6, 36, " Format ");
    wattroff(window->win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Set default cursor position
    set_current_field(info->form, info->fields[FLD_SAVE_FILE]);
    form_driver(info->form, REQ_END_LINE);
    curs_set(1);

    // Get filter stats
    sip_stats_t stats = storage_calls_stats();

    // Set default save modes
    info->savemode = (stats.displayed == stats.total) ? SAVE_ALL : SAVE_DISPLAYED;
    info->saveformat = (setting_enabled(SETTING_CAPTURE_RTP))? SAVE_PCAP_RTP : SAVE_PCAP;

    return window;
}
