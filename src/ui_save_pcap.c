/**************************************************************************
 **
 ** sngrep - SIP callflow viewer using ngrep
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
#include "ui_save_pcap.h"
#include "option.h"

PANEL *
save_create()
{
    PANEL *panel;
    WINDOW *win;
    int height, width;
    save_info_t *info;
    char savefile[128];

    // Calculate window dimensions
    height = 8;
    width = 90;

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
    info->fields[FLD_SAVE_FILE]     = new_field(1, 68, 3, 15, 0, 0);
    info->fields[FLD_SAVE_SAVE]     = new_field(1, 10, height - 2, 30, 0, 0);
    info->fields[FLD_SAVE_CANCEL]   = new_field(1, 10, height - 2, 50, 0, 0);
    info->fields[FLD_SAVE_COUNT]    = NULL;

    // Set fields options
    field_opts_off(info->fields[FLD_SAVE_FILE], O_AUTOSKIP);

    // Change background of input fields
    set_field_back(info->fields[FLD_SAVE_FILE], A_UNDERLINE);

    // Create the form and post it
    info->form = new_form(info->fields);
    set_form_sub(info->form, win);
    post_form(info->form);

    // Fields labels
    mvwprintw(win, 3, 3, "Save file:");

    // Set Default field values
    sprintf(savefile, "%s/sngrep-capture-%u.pcap",
        get_option_value("sngrep.savepath"), (unsigned)time(NULL));

    set_field_buffer(info->fields[FLD_SAVE_FILE], 0, savefile);
    set_field_buffer(info->fields[FLD_SAVE_SAVE],      0, "[  Save  ]");
    set_field_buffer(info->fields[FLD_SAVE_CANCEL],    0, "[ Cancel ]");

    // Set the window title and boxes
    mvwprintw(win, 1, 32, "Save capture to file");
    wattron(win, COLOR_PAIR(DETAIL_BORDER_COLOR));
    title_foot_box(panel_window(panel));
    mvwhline(win, height-3, 1, ACS_HLINE, width-1);
    mvwaddch(win, height-3, 0, ACS_LTEE);
    mvwaddch(win, height-3, width-1, ACS_RTEE);
    wattroff(win, COLOR_PAIR(DETAIL_BORDER_COLOR));

    // Set default cursor position
    set_current_field(info->form, info->fields[FLD_SAVE_FILE]);
    wmove(win, 3, 15);
    curs_set(1);

    return panel;
}

void
save_destroy(PANEL *panel)
{
    // Disable cursor position
    curs_set(0);
}

int
save_handle_key(PANEL *panel, int key)
{
    int field_idx;
    char field_value[48];

    // Get panel information
    save_info_t *info = (save_info_t*) panel_userptr(panel);

    // Get current field id
    field_idx = field_index(current_field(info->form));

    // Get current field value.
    // We trim spaces with sscanf because and empty field is stored as
    // space characters
    memset(field_value, 0, sizeof(field_value));
    sscanf(field_buffer(current_field(info->form), 0), "%[^ ]", field_value);

    switch(key)  {
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
    case KEY_BACKSPACE:
        if (strlen(field_value) > 0)
            form_driver(info->form, REQ_DEL_PREV);
        break;
    case 10:    /* KEY_ENTER */
        if (field_idx != FLD_SAVE_CANCEL) {
            if (!strcasecmp(field_value, "")) {
                save_error_message(panel, "Invalid filename");
                return 0;
            }
            return save_to_file(panel);
        }
        return 27;
    default:
        // If this is a normal character on input field, print it
        switch(field_idx) {
            case FLD_SAVE_FILE:
                form_driver(info->form, key);
                break;
        }
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

    return 0;
}

void
save_error_message(PANEL *panel, const char *message)
{
    WINDOW *win = panel_window(panel);
    mvwprintw(win, 4, 3, "Error: %s", message);
    wmove(win, 3, 15);
}

int
save_to_file(PANEL *panel)
{
    char field_value[48];
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    int saved_errno;

    // Get panel information
    save_info_t *info = (save_info_t*) panel_userptr(panel);

    // Get current field value.
    // We trim spaces with sscanf because and empty field is stored as
    // space characters
    memset(field_value, 0, sizeof(field_value));
    sscanf(field_buffer(info->fields[FLD_SAVE_FILE], 0), "%[^ ]", field_value);

    fd_from = open(get_option_value("sngrep.tmpfile"), O_RDONLY);
    if (fd_from < 0) {
        save_error_message(panel, "Unable top open tempfile");
        return 0;
    }

    fd_to = open(field_value, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd_to < 0) {
        save_error_message(panel, "Unable to open save file for writting");
        goto out_error;
    }

    while (nread = read(fd_from, buf, sizeof buf), nread > 0)
    {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0) {
                nread -= nwritten;
                out_ptr += nwritten;
            } else if (errno != EINTR) {
                save_error_message(panel, "Write to file interrupted");
                goto out_error;
            }
        } while (nread > 0);
    }

    if (nread == 0) {
        if (close(fd_to) < 0) {
            fd_to = -1;
            goto out_error;
        }
        close(fd_from);
        return 27;
    }

 out_error:
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0)
        close(fd_to);

    errno = saved_errno;
    return 0;

}

