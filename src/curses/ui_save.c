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

void
save_create(ui_t *ui)
{
    save_info_t *info;
    char savepath[MAX_SETTING_LEN];

    // Pause the capture while saving
    capture_set_paused(1);

    // Cerate a new indow for the panel and form
    ui_panel_create(ui, 15, 68);

    // Initialize save panel specific data
    info = sng_malloc(sizeof(save_info_t));

    // Store it into panel userptr
    set_panel_userptr(ui->panel, (void*) info);

    // Initialize the fields    int total, displayed;

    info->fields[FLD_SAVE_PATH] = new_field(1, 52, 3, 13, 0, 0);
    info->fields[FLD_SAVE_FILE] = new_field(1, 47, 4, 13, 0, 0);
    info->fields[FLD_SAVE_ALL] = new_field(1, 1, 7, 4, 0, 0);
    info->fields[FLD_SAVE_SELECTED] = new_field(1, 1, 8, 4, 0, 0);
    info->fields[FLD_SAVE_DISPLAYED] = new_field(1, 1, 9, 4, 0, 0);
    info->fields[FLD_SAVE_MESSAGE] = new_field(1, 1, 10, 4, 0, 0);
    info->fields[FLD_SAVE_PCAP] = new_field(1, 1, 7, 36, 0, 0);
    info->fields[FLD_SAVE_PCAP_RTP] = new_field(1, 1, 8, 36, 0, 0);
    info->fields[FLD_SAVE_TXT] = new_field(1, 1, 9, 36, 0, 0);
    info->fields[FLD_SAVE_SAVE] = new_field(1, 10, ui->height - 2, 20, 0, 0);
    info->fields[FLD_SAVE_CANCEL] = new_field(1, 10, ui->height - 2, 40, 0, 0);
    info->fields[FLD_SAVE_COUNT] = NULL;

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
    set_max_field(info->fields[FLD_SAVE_PATH], MAX_SETTING_LEN);
    set_max_field(info->fields[FLD_SAVE_FILE], MAX_SETTING_LEN);

    // Change background of input fields
    set_field_back(info->fields[FLD_SAVE_PATH], A_UNDERLINE);
    set_field_back(info->fields[FLD_SAVE_FILE], A_UNDERLINE);

    // Disable Save RTP if RTP packets are not being captured
    if (!setting_enabled(SETTING_CAPTURE_RTP))
        field_opts_off(info->fields[FLD_SAVE_PCAP_RTP], O_ACTIVE);

    // Create the form and post it
    info->form = new_form(info->fields);
    set_form_sub(info->form, ui->win);
    post_form(info->form);
    form_opts_off(info->form, O_BS_OVERLOAD);

    // Set Default field values
    sprintf(savepath, "%s", setting_get_value(SETTING_SAVEPATH));

    set_field_buffer(info->fields[FLD_SAVE_PATH], 0, savepath);
    set_field_buffer(info->fields[FLD_SAVE_SAVE], 0, "[  Save  ]");
    set_field_buffer(info->fields[FLD_SAVE_CANCEL], 0, "[ Cancel ]");

    // Set window boxes
    wattron(ui->win, COLOR_PAIR(CP_BLUE_ON_DEF));
    // Window border
    title_foot_box(ui->panel);

    // Header and footer lines
    mvwhline(ui->win, ui->height - 3, 1, ACS_HLINE, ui->width - 1);
    mvwaddch(ui->win, ui->height - 3, 0, ACS_LTEE);
    mvwaddch(ui->win, ui->height - 3, ui->width - 1, ACS_RTEE);

    // Save mode box
    mvwaddch(ui->win, 6, 2, ACS_ULCORNER);
    mvwhline(ui->win, 6, 3, ACS_HLINE, 30);
    mvwaddch(ui->win, 6, 32, ACS_URCORNER);
    mvwvline(ui->win, 7, 2, ACS_VLINE, 4);
    mvwvline(ui->win, 7, 32, ACS_VLINE, 4);
    mvwaddch(ui->win, 11, 2, ACS_LLCORNER);
    mvwhline(ui->win, 11, 3, ACS_HLINE, 30);
    mvwaddch(ui->win, 11, 32, ACS_LRCORNER);

    // Save mode box
    mvwaddch(ui->win, 6, 34, ACS_ULCORNER);
    mvwhline(ui->win, 6, 35, ACS_HLINE, 30);
    mvwaddch(ui->win, 6, 64, ACS_URCORNER);
    mvwvline(ui->win, 7, 34, ACS_VLINE, 3);
    mvwvline(ui->win, 7, 64, ACS_VLINE, 3);
    mvwaddch(ui->win, 10, 34, ACS_LLCORNER);
    mvwhline(ui->win, 10, 35, ACS_HLINE, 30);
    mvwaddch(ui->win, 10, 64, ACS_LRCORNER);

    wattroff(ui->win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Set screen labels
    mvwprintw(ui->win, 1, 27, "Save capture");
    mvwprintw(ui->win, 3, 3, "Path:");
    mvwprintw(ui->win, 4, 3, "Filename:");
    wattron(ui->win, COLOR_PAIR(CP_BLUE_ON_DEF));
    mvwprintw(ui->win, 6, 4, " Dialogs ");
    mvwprintw(ui->win, 6, 36, " Format ");
    wattroff(ui->win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Set default cursor position
    set_current_field(info->form, info->fields[FLD_SAVE_FILE]);
    form_driver(info->form, REQ_END_LINE);
    curs_set(1);

    // Get filter stats
    sip_stats_t stats = sip_calls_stats();

    // Set default save modes
    info->savemode = (stats.displayed == stats.total) ? SAVE_ALL : SAVE_DISPLAYED;
    info->saveformat = (setting_enabled(SETTING_CAPTURE_RTP))? SAVE_PCAP_RTP : SAVE_PCAP;

}

void
save_destroy(ui_t *ui)
{
    save_info_t *info;
    int i;

    // Get panel information
    if ((info = save_info(ui))) {
        // Remove panel form and fields
        unpost_form(info->form);
        free_form(info->form);
        for (i = 0; i < FLD_SAVE_COUNT; i++)
            free_field(info->fields[i]);

        // Remove panel window and custom info
        sng_free(info);
    }

    // Delete panel
    ui_panel_destroy(ui);

    // Resume capture
    capture_set_paused(0);

    // Disable cursor position
    curs_set(0);
}

save_info_t *
save_info(ui_t *ui)
{
    return (save_info_t*) panel_userptr(ui->panel);
}

int
save_draw(ui_t *ui)
{
    char field_value[MAX_SETTING_LEN];

    // Get panel information
    save_info_t *info = save_info(ui);

    // Get filter stats
    sip_stats_t stats = sip_calls_stats();

    mvwprintw(ui->win, 7, 3, "( ) all dialogs ");
    mvwprintw(ui->win, 8, 3, "( ) selected dialogs [%d]", call_group_count(info->group));
    mvwprintw(ui->win, 9, 3, "( ) filtered dialogs [%d]", stats.displayed);

    // Print 'current SIP message' field label if required
    if (info->msg != NULL)
        mvwprintw(ui->win, 10, 3, "( ) current SIP message");

    mvwprintw(ui->win, 7, 35, "( ) .pcap (SIP)");
    mvwprintw(ui->win, 8, 35, "( ) .pcap (SIP + RTP)");
    mvwprintw(ui->win, 9, 35, "( ) .txt");

    // Get filename field value.
    memset(field_value, 0, sizeof(field_value));
    strcpy(field_value, field_buffer(info->fields[FLD_SAVE_FILE], 0));
    strtrim(field_value);

    mvwprintw(ui->win, 4, 60, "     ");
    if (strstr(field_value, ".pcap")) {
        info->saveformat = (setting_enabled(SETTING_CAPTURE_RTP))? SAVE_PCAP_RTP : SAVE_PCAP;
    } else if (strstr(field_value, ".txt")) {
        info->saveformat = SAVE_TXT;
    } else {
        if (info->saveformat == SAVE_PCAP || info->saveformat == SAVE_PCAP_RTP)
            mvwprintw(ui->win, 4, 60, ".pcap");
        else
            mvwprintw(ui->win, 4, 60, ".txt ");
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

int
save_handle_key(ui_t *ui, int key)
{
    int field_idx;
    int action = -1;

    // Get panel information
    save_info_t *info = save_info(ui);

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
                    save_to_file(ui);
                }
                ui_destroy(ui);
                return KEY_HANDLED;
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
save_set_group(ui_t *ui, sip_call_group_t *group)
{
    // Get panel information
    save_info_t *info = save_info(ui);
    info->group = group;
    if (call_group_count(group)) {
        info->savemode = SAVE_SELECTED;
    }
}

void
save_set_msg(ui_t *ui, sip_msg_t *msg)
{
    // Get panel information
    save_info_t *info = save_info(ui);
    info->msg = msg;

    // make 'current SIP message' field visible
    field_opts_on(info->fields[FLD_SAVE_MESSAGE], O_VISIBLE);
}

int
save_to_file(ui_t *ui)
{
    char savepath[MAX_SETTING_LEN];
    char savefile[MAX_SETTING_LEN];
    char fullfile[MAX_SETTING_LEN*2];
    sip_call_t *call = NULL;
    sip_msg_t *msg = NULL;
    pcap_dumper_t *pd = NULL;
    FILE *f = NULL;
    int cur = 0, total = 0;
    WINDOW *progress;
    vector_iter_t calls, msgs, rtps, packets;
    packet_t *packet;
    vector_t *sorted;

    // Get panel information
    save_info_t *info = save_info(ui);

    // Get current path field value.
    memset(savepath, 0, sizeof(savepath));
    strcpy(savepath, field_buffer(info->fields[FLD_SAVE_PATH], 0));
    strtrim(savepath);
    if (strlen(savepath))
        strcat(savepath, "/");

    // Get current file field value.
    memset(savefile, 0, sizeof(savefile));
    strcpy(savefile, field_buffer(info->fields[FLD_SAVE_FILE], 0));
    strtrim(savefile);

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
        if (dialog_confirm("Overwrite confirmation", "Selected file already exists.\n Do you want to overwrite it?", "Yes,No") != 0)
            return 1;
    }

    // Don't allow to save no packets!
    if (info->savemode == SAVE_SELECTED && call_group_msg_count(info->group) == 0) {
        dialog_run("Unable to save: No selected dialogs.");
        return 1;
    }

    if (info->saveformat == SAVE_PCAP || info->saveformat == SAVE_PCAP_RTP) {
        // Open dump file
        pd = dump_open(fullfile, NULL);
        if (access(fullfile, W_OK) != 0) {
            dialog_run("%s", capture_last_error());
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
        default:
            break;
    }

    if (info->savemode == SAVE_MESSAGE) {
        if (info->saveformat == SAVE_TXT) {
            // Save selected message to file
            save_msg_txt(f, info->msg);
        } else {
            // Save selected message packet to pcap
            dump_packet(pd, info->msg->packet);
        }
    } else if (info->saveformat == SAVE_TXT) {
        // Save selected packets to file
        while ((call = vector_iterator_next(&calls))) {
            msgs = vector_iterator(call->msgs);
            // Save SIP message content
            while ((msg = vector_iterator_next(&msgs))) {
                save_msg_txt(f, msg);
            }
        }
    } else {
        // Store all messages in a time sorted vector
        sorted = vector_create(100, 50);
        vector_set_sorter(sorted, capture_packet_time_sorter);

        // Count packages for progress bar
        while ((call = vector_iterator_next(&calls))) {
            total += vector_count(call->msgs);
            if (info->saveformat == SAVE_PCAP_RTP)
                total += vector_count(call->rtp_packets);
        }
        vector_iterator_reset(&calls);

        progress = dialog_progress_run("Saving packets...");
        dialog_progress_set_value(progress, 0);

        // Save selected packets to file
        while ((call = vector_iterator_next(&calls))) {
            msgs = vector_iterator(call->msgs);
            // Save SIP message content
            while ((msg = vector_iterator_next(&msgs))) {
                // Update progress bar dialog
                dialog_progress_set_value(progress, (++cur * 100) / total);
                vector_append(sorted, msg->packet);
            }

            // Save RTP packets
            if (info->saveformat == SAVE_PCAP_RTP) {
                rtps = vector_iterator(call->rtp_packets);
                while ((packet = vector_iterator_next(&rtps))) {
                    // Update progress bar dialog
                    dialog_progress_set_value(progress, (++cur * 100) / total);
                    vector_append(sorted, packet);
                }
            }
        }

        // Save sorted packets
        packets = vector_iterator(sorted);
        while ((packet = vector_iterator_next(&packets))) {
            dump_packet(pd, packet);
        }

        dialog_progress_destroy(progress);
    }

    // Close saved file
    if (info->saveformat == SAVE_PCAP || info->saveformat == SAVE_PCAP_RTP) {
        dump_close(pd);
    } else {
        fclose(f);
    }

    // Show success popup
    if (info->savemode == SAVE_MESSAGE) {
      dialog_run("Successfully saved selected SIP message to %s", savefile);
    } else {
      dialog_run("Successfully saved %d dialogs to %s", vector_iterator_count(&calls), savefile);
    }

    return 0;
}

void
save_msg_txt(FILE *f, sip_msg_t *msg)
{
    char date[20], time[20], src[80], dst[80];

    fprintf(f, "%s %s %s -> %s\n%s\n\n",
            msg_get_attribute(msg, SIP_ATTR_DATE, date),
            msg_get_attribute(msg, SIP_ATTR_TIME, time),
            msg_get_attribute(msg, SIP_ATTR_SRC, src),
            msg_get_attribute(msg, SIP_ATTR_DST, dst),
            msg_get_payload(msg));
}
