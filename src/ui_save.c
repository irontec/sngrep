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
 * @file ui_save_pcap.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_save_pcap.c
 */

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <form.h>
#include <ctype.h>
#include "ui_save.h"
#include "setting.h"
#include "capture.h"
#include "filter.h"

/**
 * Ui Structure definition for Save panel
 */
ui_t ui_save = {
    .type = PANEL_SAVE,
    .panel = NULL,
    .create = save_create,
    .draw = save_draw,
    .handle_key = save_handle_key,
    .destroy = save_destroy
};

PANEL *
save_create()
{
    PANEL *panel;
    WINDOW *win;
    int height, width;
    save_info_t *info;
    char savepath[128];
    int total, displayed;

    // Pause the capture while saving
    capture_set_paused(1);

    // Calculate window dimensions
    height = 14;
    width = 68;

    // Cerate a new indow for the panel and form
    win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);

    // Create a new panel
    panel = new_panel(win);

    // Initialize save panel specific data
    info = malloc(sizeof(save_info_t));
    memset(info, 0, sizeof(save_info_t));

    // Store it into panel userptr
    set_panel_userptr(panel, (void*) info);

    // Initialize the fields
    info->fields[FLD_SAVE_PATH] = new_field(1, 52, 3, 13, 0, 0);
    info->fields[FLD_SAVE_FILE] = new_field(1, 47, 4, 13, 0, 0);
    info->fields[FLD_SAVE_ALL] = new_field(1, 1, 7, 4, 0, 0);
    info->fields[FLD_SAVE_SELECTED] = new_field(1, 1, 8, 4, 0, 0);
    info->fields[FLD_SAVE_DISPLAYED] = new_field(1, 1, 9, 4, 0, 0);
    info->fields[FLD_SAVE_PCAP] = new_field(1, 1, 7, 36, 0, 0);
    info->fields[FLD_SAVE_TXT] = new_field(1, 1, 8, 36, 0, 0);
    info->fields[FLD_SAVE_SAVE] = new_field(1, 10, height - 2, 20, 0, 0);
    info->fields[FLD_SAVE_CANCEL] = new_field(1, 10, height - 2, 40, 0, 0);
    info->fields[FLD_SAVE_COUNT] = NULL;

    // Set fields options
    field_opts_off(info->fields[FLD_SAVE_PATH], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_SAVE_FILE], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_SAVE_ALL], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_SAVE_SELECTED], O_AUTOSKIP);
    field_opts_off(info->fields[FLD_SAVE_DISPLAYED], O_AUTOSKIP);

    // Change background of input fields
    set_field_back(info->fields[FLD_SAVE_PATH], A_UNDERLINE);
    set_field_back(info->fields[FLD_SAVE_FILE], A_UNDERLINE);

    // Create the form and post it
    info->form = new_form(info->fields);
    set_form_sub(info->form, win);
    post_form(info->form);
    form_opts_off(info->form, O_BS_OVERLOAD);

    // Set Default field values
    sprintf(savepath, "%s", setting_get_value(SETTING_SAVEPATH));

    set_field_buffer(info->fields[FLD_SAVE_PATH], 0, savepath);
    set_field_buffer(info->fields[FLD_SAVE_SAVE], 0, "[  Save  ]");
    set_field_buffer(info->fields[FLD_SAVE_CANCEL], 0, "[ Cancel ]");

    // Set window boxes
    wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    // Window border
    title_foot_box(panel_window(panel));

    // Header and footer lines
    mvwhline(win, height - 3, 1, ACS_HLINE, width - 1);
    mvwaddch(win, height - 3, 0, ACS_LTEE);
    mvwaddch(win, height - 3, width - 1, ACS_RTEE);

    // Save mode box
    mvwaddch(win, 6, 2, ACS_ULCORNER);
    mvwhline(win, 6, 3, ACS_HLINE, 30);
    mvwaddch(win, 6, 32, ACS_URCORNER);
    mvwvline(win, 7, 2, ACS_VLINE, 3);
    mvwvline(win, 7, 32, ACS_VLINE, 3);
    mvwaddch(win, 10, 2, ACS_LLCORNER);
    mvwhline(win, 10, 3, ACS_HLINE, 30);
    mvwaddch(win, 10, 32, ACS_LRCORNER);

    // Save mode box
    mvwaddch(win, 6, 34, ACS_ULCORNER);
    mvwhline(win, 6, 35, ACS_HLINE, 30);
    mvwaddch(win, 6, 64, ACS_URCORNER);
    mvwvline(win, 7, 34, ACS_VLINE, 3);
    mvwvline(win, 7, 64, ACS_VLINE, 3);
    mvwaddch(win, 10, 34, ACS_LLCORNER);
    mvwhline(win, 10, 35, ACS_HLINE, 30);
    mvwaddch(win, 10, 64, ACS_LRCORNER);

    wattroff(win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Set screen labels
    mvwprintw(win, 1, 27, "Save capture");
    mvwprintw(win, 3, 3, "Path:");
    mvwprintw(win, 4, 3, "Filename:");
    wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    mvwprintw(win, 6, 4, " Dialogs ");
    mvwprintw(win, 6, 36, " Format ");
    wattroff(win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Set default cursor position
    set_current_field(info->form, info->fields[FLD_SAVE_FILE]);
    form_driver(info->form, REQ_END_LINE);
    curs_set(1);

    // Get filter stats
    sip_calls_stats(&total, &displayed);

    // Set default save modes
    info->savemode = (displayed == total) ? SAVE_ALL : SAVE_DISPLAYED;
    info->saveformat = SAVE_PCAP;

    return panel;
}

void
save_destroy(PANEL *panel)
{
    save_info_t *info;
    int i;

    // Get panel information
    if ((info = save_info(panel))) {
        // Remove panel form and fields
        unpost_form(info->form);
        free_form(info->form);
        for (i = 0; i < FLD_SAVE_COUNT; i++)
            free_field(info->fields[i]);

        // Remove panel window and custom info
        delwin(panel_window(panel));
        free(info);
    }

    // Resume capture
    capture_set_paused(0);

    // Disable cursor position
    curs_set(0);
}

save_info_t *
save_info(PANEL *panel)
{
    return (save_info_t*) panel_userptr(panel);
}

int
save_draw(PANEL *panel)
{
    int total, displayed;
    const char *field_value;

    // Get panel information
    save_info_t *info = save_info(panel);
    WINDOW *win = panel_window(panel);

    // Get filter stats
    sip_calls_stats(&total, &displayed);

    mvwprintw(win, 7, 3, "( ) all dialogs ");
    mvwprintw(win, 8, 3, "( ) selected dialogs [%d]", call_group_count(info->group));
    mvwprintw(win, 9, 3, "( ) filtered dialogs [%d]", displayed);

    mvwprintw(win, 7, 35, "( ) .pcap");
    mvwprintw(win, 8, 35, "( ) .txt");

    // Get filename field value.
    field_value = strtrim(field_buffer(info->fields[FLD_SAVE_FILE], 0));

    mvwprintw(win, 4, 60, "     ");
    if (strstr(field_value, ".pcap")) {
        info->saveformat = SAVE_PCAP;
    } else if (strstr(field_value, ".txt")) {
        info->saveformat = SAVE_TXT;
    } else {
        if (info->saveformat == SAVE_PCAP)
            mvwprintw(win, 4, 60, ".pcap");
        else
            mvwprintw(win, 4, 60, ".txt ");
    }

    set_field_buffer(info->fields[FLD_SAVE_ALL], 0, (info->savemode == SAVE_ALL) ? "*" : " ");
    set_field_buffer(info->fields[FLD_SAVE_SELECTED], 0,
                     (info->savemode == SAVE_SELECTED) ? "*" : " ");
    set_field_buffer(info->fields[FLD_SAVE_DISPLAYED], 0,
                     (info->savemode == SAVE_DISPLAYED) ? "*" : " ");
    set_field_buffer(info->fields[FLD_SAVE_PCAP], 0, (info->saveformat == SAVE_PCAP) ? "*" : " ");
    set_field_buffer(info->fields[FLD_SAVE_TXT], 0, (info->saveformat == SAVE_TXT) ? "*" : " ");

    set_current_field(info->form, current_field(info->form));
    form_driver(info->form, REQ_VALIDATION);

    return 0;
}

int
save_handle_key(PANEL *panel, int key)
{
    int field_idx;
    int action = -1;

    // Get panel information
    save_info_t *info = save_info(panel);

    // Get current field id
    field_idx = field_index(current_field(info->form));

    // Check actions for this key
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
                    case FLD_SAVE_PCAP:
                        info->saveformat = SAVE_PCAP;
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
                    return save_to_file(panel);
                }
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
    return (action == ERR) ? key : 0;
}

void
save_set_group(PANEL *panel, sip_call_group_t *group)
{
    // Get panel information
    save_info_t *info = save_info(panel);
    info->group = group;
    if (call_group_count(group)) {
        info->savemode = SAVE_SELECTED;
    }
}

int
save_to_file(PANEL *panel)
{
    char savepath[256];
    char savefile[256];
    char fullfile[512];
    sip_call_t *call = NULL;
    sip_msg_t *msg = NULL;
    pcap_dumper_t *pd = NULL;
    FILE *f = NULL;
    int i;
    vector_iter_t calls, msgs;

    // Get panel information
    save_info_t *info = save_info(panel);

    // Get current path field value.
    memset(savepath, 0, sizeof(savepath));
    strcpy(savepath, field_buffer(info->fields[FLD_SAVE_PATH], 0));
    // Trim trailing spaces
    for (i = strlen(savepath) - 1; isspace(savepath[i]); i--)
        savepath[i] = '\0';
    if (strlen(savepath))
        strcat(savepath, "/");

    // Get current file field value.
    memset(savefile, 0, sizeof(savefile));
    strcpy(savefile, field_buffer(info->fields[FLD_SAVE_FILE], 0));
    // Trim trailing spaces
    for (i = strlen(savefile) - 1; isspace(savefile[i]); i--)
        savefile[i] = '\0';

    if (!strlen(savefile)) {
        dialog_run("Please enter a valid filename");
        return 1;
    }

    if (info->saveformat == SAVE_PCAP) {
        if (!strstr(savefile, ".pcap"))
            strcat(savefile, ".pcap");
    } else {
        if (!strstr(savefile, ".txt"))
            strcat(savefile, ".txt");
    }

    // Absolute filename
    sprintf(fullfile, "%s%s", savepath, savefile);

    if (access(fullfile, R_OK) == 0) {
        dialog_run("Error: file %s already exists.", fullfile);
        return 1;
    }

    // Don't allow to save no packets!
    if (info->savemode == SAVE_SELECTED && call_group_msg_count(info->group) == 0) {
        dialog_run("Unable to save: No selected dialogs.");
        return 1;
    }

    if (info->saveformat == SAVE_PCAP) {
        // Open dump file
        pd = dump_open(fullfile);
        if (access(fullfile, W_OK) != 0) {
            dialog_run(capture_last_error());
            return 1;
        }
    } else {
        // Open a text file
        if (!(f = fopen(fullfile, "w"))) {
            dialog_run("Error: %s", strerror(errno));
            return 0;
        }
    }

    // Get calls iterator
    switch (info->savemode) {
        case SAVE_ALL:
            // Get calls iterator
            calls = sip_calls_iterator();
            break;
        case SAVE_SELECTED:
            // Save selected packets to file
            calls = vector_iterator(info->group->calls);
            break;
        case SAVE_DISPLAYED:
            // Set filtering for this iterator
            calls = sip_calls_iterator();
            vector_iterator_set_filter(&calls, filter_check_call);
            break;
    }

    // Save selected packets to file
    while ((call = vector_iterator_next(&calls))) {
        msgs = vector_iterator(call->msgs);
        while ((msg = vector_iterator_next(&msgs))) {
            (info->saveformat == SAVE_PCAP) ? save_msg_pcap(pd, msg) : save_msg_txt(f, msg);
        }
    }

    // Close saved file
    if (info->saveformat == SAVE_PCAP) {
        dump_close(pd);
    } else {
        fclose(f);
    }

    // Show success popup
    dialog_run("Successfully saved %d dialogs to %s", vector_iterator_count(&calls), savefile);

    return 27;
}

void
save_msg_pcap(pcap_dumper_t *pd, sip_msg_t *msg)
{
    capture_packet_t *packet;
    vector_iter_t it = vector_iterator(msg->packets);

    while ((packet = vector_iterator_next(&it))) {
        dump_packet(pd, packet->header, packet->data);
    }
}

void
save_msg_txt(FILE *f, sip_msg_t *msg)
{
    fprintf(f, "%s %s %s -> %s\n%s\n\n", msg_get_attribute(msg, SIP_ATTR_DATE),
            msg_get_attribute(msg, SIP_ATTR_TIME), msg_get_attribute(msg, SIP_ATTR_SRC),
            msg_get_attribute(msg, SIP_ATTR_DST), msg_get_payload(msg));
}
