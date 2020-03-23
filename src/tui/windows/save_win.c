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
 * @file save_win.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in save_win.h
 */
#include "config.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <unistd.h>
#include <form.h>
#ifdef  WITH_SND
#include <sndfile.h>
#endif
#include "codecs/codec.h"
#include "glib-extra/glib.h"
#include "setting.h"
#include "storage/filter.h"
#include "capture/capture_txt.h"
#include "tui/dialog.h"
#include "tui/windows/save_win.h"

G_DEFINE_TYPE(SaveWindow, save, TUI_TYPE_WINDOW)

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

    SaveWindow *self = TUI_SAVE(window);
    WINDOW *win = window_get_ncurses_window(window);

    // Get filter stats
    StorageStats stats = storage_calls_stats();
    // Get Storage options
    StorageCaptureOpts storageCaptureOpts = storage_capture_options();

    mvwprintw(win, 7, 3, "( ) all dialogs ");
    if (self->group != NULL) {
        mvwprintw(win, 8, 3, "( ) selected dialogs [%d]", call_group_count(self->group));
        mvwprintw(win, 9, 3, "( ) filtered dialogs [%d]", stats.displayed);
    }

    // Print 'current SIP message' field label if required
    if (self->msg != NULL)
        mvwprintw(win, 10, 3, "( ) current SIP message");

    if (self->stream != NULL) {
        mvwprintw(win, 7, 3, "( ) current stream");
        mvwprintw(win, 7, 35, "( ) .wav");
    } else {
        mvwprintw(win, 7, 35, "( ) .pcap (SIP)");
        mvwprintw(win, 8, 35, "( ) .pcap (SIP + RTP)");
        mvwprintw(win, 9, 35, "( ) .txt");
    }

    // Get filename field value.
    memset(field_value, 0, sizeof(field_value));
    strcpy(field_value, field_buffer(self->fields[FLD_SAVE_FILE], 0));
    g_strstrip(field_value);

    mvwprintw(win, 4, 60, "     ");
    if (strstr(field_value, ".pcap")) {
        self->saveformat = (storageCaptureOpts.rtp) ? SAVE_PCAP_RTP : SAVE_PCAP;
    } else if (strstr(field_value, ".txt")) {
        self->saveformat = SAVE_TXT;
    } else {
        if (self->saveformat == SAVE_PCAP || self->saveformat == SAVE_PCAP_RTP)
            mvwprintw(win, 4, 60, ".pcap");
        else if (self->saveformat == SAVE_WAV)
            mvwprintw(win, 4, 60, ".wav");
        else
            mvwprintw(win, 4, 60, ".txt ");
    }

    set_field_buffer(self->fields[FLD_SAVE_ALL], 0,
                     (self->savemode == SAVE_ALL) ? "*" : " ");
    set_field_buffer(self->fields[FLD_SAVE_SELECTED], 0,
                     (self->savemode == SAVE_SELECTED) ? "*" : " ");
    set_field_buffer(self->fields[FLD_SAVE_DISPLAYED], 0,
                     (self->savemode == SAVE_DISPLAYED) ? "*" : " ");
    set_field_buffer(self->fields[FLD_SAVE_MESSAGE], 0,
                     (self->savemode == SAVE_MESSAGE) ? "*" : " ");
    set_field_buffer(self->fields[FLD_SAVE_STREAM], 0,
                     (self->savemode == SAVE_STREAM) ? "*" : " ");
    set_field_buffer(self->fields[FLD_SAVE_PCAP], 0,
                     (self->saveformat == SAVE_PCAP) ? "*" : " ");
    set_field_buffer(self->fields[FLD_SAVE_PCAP_RTP], 0,
                     (self->saveformat == SAVE_PCAP_RTP) ? "*" : " ");
    set_field_buffer(self->fields[FLD_SAVE_TXT], 0,
                     (self->saveformat == SAVE_TXT) ? "*" : " ");
    set_field_buffer(self->fields[FLD_SAVE_WAV], 0,
                     (self->saveformat == SAVE_WAV) ? "*" : " ");

    // Show disabled options with makers
    if (self->group != NULL && !storageCaptureOpts.rtp) {
        set_field_buffer(self->fields[FLD_SAVE_PCAP_RTP], 0, "-");
    }

    set_current_field(self->form, current_field(self->form));
    form_driver(self->form, REQ_VALIDATION);

    return 0;
}

/**
 * @brief Callback function to save a single packet into a capture output
 */
static void
save_packet_cb(Packet *packet, CaptureOutput *output)
{
    capture_output_write(output, packet);
}

#ifdef WITH_SND

static gboolean
save_stream_to_file(Window *window)
{
    // Get panel information
    SaveWindow *self = TUI_SAVE(window);
    g_return_val_if_fail(self != NULL, false);

    // Get current path field value.
    g_autofree gchar *save_path = g_malloc0(SETTING_MAX_LEN);
    g_strlcpy(save_path, field_buffer(self->fields[FLD_SAVE_PATH], 0), SETTING_MAX_LEN - 10);
    g_strstrip(save_path);
    if (strlen(save_path) != 0)
        strcat(save_path, G_DIR_SEPARATOR_S);

    // Get current file field value.
    g_autofree gchar *save_file = g_malloc0(SETTING_MAX_LEN);
    g_strlcpy(save_file, field_buffer(self->fields[FLD_SAVE_FILE], 0), SETTING_MAX_LEN - 10);
    g_strstrip(save_file);

    if (!strlen(save_file)) {
        dialog_run("Please enter a valid filename");
        return FALSE;
    }

    if (!strstr(save_file, ".wav"))
        strcat(save_file, ".wav");

    // Absolute filename
    g_autofree gchar *full_file = g_strdup_printf("%s%s", save_path, save_file);
    if (g_access(full_file, R_OK) == 0) {
        if (dialog_confirm("Overwrite confirmation",
                           "Selected file already exits.\n Do you want to overwrite it?",
                           "Yes,No") != 0)
            return false;
    }

    SF_INFO file_info = { 0 };
    file_info.samplerate = 8000;
    file_info.channels = 1;
    file_info.format = SF_FORMAT_WAV | SF_FORMAT_GSM610;

    GError *error = NULL;
    g_autoptr(GByteArray) decoded = codec_stream_decode(self->stream, NULL, &error);
    if (error != NULL) {
        dialog_run("error: %s", error->message);
        return FALSE;
    }

    // Create a new wav file in requested path
    SNDFILE *file = sf_open(full_file, SFM_WRITE, &file_info);
    if (file == NULL) {
        dialog_run("error: %s", sf_strerror(file));
        return FALSE;
    }

    // Save all decoded bytes
    sf_write_short(file, (const gint16 *) decoded->data, decoded->len / 2);

    // Close wav file and free decoded data
    sf_close(file);

    dialog_run("%d bytes decoded into %s", g_byte_array_len(decoded) / 2, full_file);
    return TRUE;
}

#endif

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
    CaptureOutput *output = NULL;
    int cur = 0, total = 0;
    WINDOW *progress;
    GPtrArray *calls = NULL;
    GPtrArray *packets = g_ptr_array_new();
    GError *error = NULL;

    // Get panel information
    SaveWindow *self = TUI_SAVE(window);
    g_return_val_if_fail(self != NULL, false);

    // Get current path field value.
    g_autofree gchar *save_path = g_malloc0(SETTING_MAX_LEN);
    g_strlcpy(save_path, field_buffer(self->fields[FLD_SAVE_PATH], 0), SETTING_MAX_LEN - 10);
    g_strstrip(save_path);
    if (strlen(save_path) != 0)
        strcat(save_path, G_DIR_SEPARATOR_S);

    // Get current file field value.
    g_autofree gchar *save_file = g_malloc0(SETTING_MAX_LEN);
    g_strlcpy(save_file, field_buffer(self->fields[FLD_SAVE_FILE], 0), SETTING_MAX_LEN - 10);
    g_strstrip(save_file);

    if (!strlen(save_file)) {
        dialog_run("Please enter a valid filename");
        return 1;
    }

    if (self->saveformat == SAVE_PCAP || self->saveformat == SAVE_PCAP_RTP) {
        if (!strstr(save_file, ".pcap"))
            strcat(save_file, ".pcap");
    } else {
        if (!strstr(save_file, ".txt"))
            strcat(save_file, ".txt");
    }

    // Absolute filename
    g_autofree gchar *full_file = g_strdup_printf("%s%s", save_path, save_file);
    if (access(full_file, R_OK) == 0) {
        if (dialog_confirm("Overwrite confirmation",
                           "Selected file already exits.\n Do you want to overwrite it?",
                           "Yes,No") != 0)
            return 1;
    }

    // Don't allow to save no packets!
    if (self->savemode == SAVE_SELECTED && call_group_msg_count(self->group) == 0) {
        dialog_run("Unable to save: No selected dialogs.");
        return 1;
    }

    if (self->saveformat == SAVE_PCAP || self->saveformat == SAVE_PCAP_RTP) {
        // Open PCAP output file
        output = capture_output_pcap(full_file, &error);
    } else {
        // Open TXT output file
        output = capture_output_txt(full_file, &error);
    }

    // Output creation error checking
    if (error != NULL) {
        dialog_run("Error: %s", error->message);
        return 1;
    }

    // Get calls iterator
    switch (self->savemode) {
        case SAVE_ALL:
        case SAVE_DISPLAYED:
            // Get calls iterator
            calls = storage_calls();
            break;
        case SAVE_SELECTED:
            // Save selected packets to file
            calls = self->group->calls;
            break;
        default:
            break;
    }

    if (self->savemode == SAVE_MESSAGE) {
        // Save selected message packet to pcap
        capture_output_write(output, self->msg->packet);
    } else if (self->saveformat == SAVE_TXT) {
        // Save selected packets to file
        for (guint i = 0; i < g_ptr_array_len(calls); i++) {
            Call *call = g_ptr_array_index(calls, i);
            // Save SIP message content
            for (guint j = 0; j < g_ptr_array_len(call->msgs); j++) {
                Message *msg = g_ptr_array_index(call->msgs, j);
                capture_output_write(output, msg->packet);
            }
        }
    } else {
        // Count packages for progress bar
        for (guint i = 0; i < g_ptr_array_len(calls); i++) {
            Call *call = g_ptr_array_index(calls, i);

            if (self->savemode == SAVE_DISPLAYED && !filter_check_call(call, NULL))
                continue;

            total += call_msg_count(call);

            if (self->saveformat == SAVE_PCAP_RTP) {
                for (guint j = 0; j < g_ptr_array_len(call->streams); j++) {
                    Stream *stream = g_ptr_array_index(call->streams, j);
                    total += stream_get_count(stream);
                }
            }
        }

        progress = dialog_progress_run("Saving packets...");
        dialog_progress_set_value(progress, 0);

        // Save selected packets to file
        for (guint i = 0; i < g_ptr_array_len(calls); i++) {
            Call *call = g_ptr_array_index(calls, i);

            if (self->savemode == SAVE_DISPLAYED && !filter_check_call(call, NULL))
                continue;

            // Save SIP message content
            for (guint j = 0; j < g_ptr_array_len(call->msgs); j++) {
                Message *msg = g_ptr_array_index(call->msgs, j);
                // Update progress bar dialog
                dialog_progress_set_value(progress, (++cur * 100) / total);
                g_ptr_array_add(packets, msg->packet);
            }

            // Save RTP packets
            if (self->saveformat == SAVE_PCAP_RTP) {
                for (guint j = 0; j < g_ptr_array_len(call->streams); j++) {
                    Stream *stream = g_ptr_array_index(call->streams, j);
                    dialog_progress_set_value(progress, (++cur * 100) / total);
                    g_ptr_array_add_array(packets, stream->packets);
                }
            }
        }

        g_ptr_array_sort(packets, (GCompareFunc) packet_time_sorter);
        g_ptr_array_foreach(packets, (GFunc) save_packet_cb, output);

        dialog_progress_destroy(progress);
    }

    // Close saved file
    capture_output_close(output);

    // Show success popup
    if (self->savemode == SAVE_MESSAGE) {
        dialog_run("Successfully saved selected SIP message to %s", save_file);
    } else {
        dialog_run("Successfully saved %d dialogs to %s", total, save_file);
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
    SaveWindow *self = TUI_SAVE(window);
    g_return_val_if_fail(self != NULL, KEY_NOT_HANDLED);

    // Get current field id
    gint field_idx = field_index(current_field(self->form));

    // Check actions for this key
    enum KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch (action) {
            case ACTION_PRINTABLE:
                if (field_idx == FLD_SAVE_PATH || field_idx == FLD_SAVE_FILE) {
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
            case ACTION_DELETE:
                form_driver(self->form, REQ_DEL_CHAR);
                break;
            case ACTION_BACKSPACE:
                form_driver(self->form, REQ_DEL_PREV);
                break;
            case ACTION_CLEAR:
                form_driver(self->form, REQ_CLR_FIELD);
                break;
            case ACTION_SELECT:
                switch (field_idx) {
                    case FLD_SAVE_ALL:
                        self->savemode = SAVE_ALL;
                        break;
                    case FLD_SAVE_SELECTED:
                        self->savemode = SAVE_SELECTED;
                        break;
                    case FLD_SAVE_DISPLAYED:
                        self->savemode = SAVE_DISPLAYED;
                        break;
                    case FLD_SAVE_MESSAGE:
                        self->savemode = SAVE_MESSAGE;
                        break;
                    case FLD_SAVE_PCAP:
                        self->saveformat = SAVE_PCAP;
                        break;
                    case FLD_SAVE_PCAP_RTP:
                        self->saveformat = SAVE_PCAP_RTP;
                        break;
                    case FLD_SAVE_TXT:
                        self->saveformat = SAVE_TXT;
                        break;
                    case FLD_SAVE_WAV:
                        self->saveformat = SAVE_WAV;
                        break;
                    case FLD_SAVE_FILE:
                        form_driver(self->form, key);
                        break;
                    default:
                        break;
                }
                break;
            case ACTION_CONFIRM:
                if (field_idx != FLD_SAVE_CANCEL) {
                    if (self->savemode == SAVE_STREAM) {
#ifdef WITH_SND
                        save_stream_to_file(window);
#endif
                    } else {
                        save_to_file(window);
                    }
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
    form_driver(self->form, REQ_VALIDATION);

    // Change background and cursor of "button fields"
    set_field_back(self->fields[FLD_SAVE_SAVE], A_NORMAL);
    set_field_back(self->fields[FLD_SAVE_CANCEL], A_NORMAL);
    curs_set(1);

    // Change current field background
    field_idx = field_index(current_field(self->form));
    if (field_idx == FLD_SAVE_SAVE || field_idx == FLD_SAVE_CANCEL) {
        set_field_back(self->fields[field_idx], A_REVERSE);
        curs_set(0);
    }

    // Return if this panel has handled or not the key
    return (action == ERR) ? KEY_NOT_HANDLED : KEY_HANDLED;
}

void
save_set_group(Window *window, CallGroup *group)
{
    // Get panel information
    SaveWindow *self = TUI_SAVE(window);
    g_return_if_fail(self != NULL);
    g_return_if_fail(group != NULL);

    self->group = group;
    if (call_group_count(group)) {
        self->savemode = SAVE_SELECTED;
    }

    field_opts_on(self->fields[FLD_SAVE_SELECTED], O_ACTIVE | O_VISIBLE);
    field_opts_on(self->fields[FLD_SAVE_DISPLAYED], O_ACTIVE | O_VISIBLE);
    field_opts_on(self->fields[FLD_SAVE_ALL], O_ACTIVE | O_VISIBLE);
    field_opts_on(self->fields[FLD_SAVE_PCAP], O_ACTIVE | O_VISIBLE);
    field_opts_on(self->fields[FLD_SAVE_TXT], O_ACTIVE | O_VISIBLE);
    field_opts_on(self->fields[FLD_SAVE_PCAP_RTP], O_VISIBLE);

    // Enable RTP if RTP packets are being captured
    StorageCaptureOpts storageCaptureOpts = storage_capture_options();
    if (storageCaptureOpts.rtp)
        field_opts_on(self->fields[FLD_SAVE_PCAP_RTP], O_ACTIVE);
}

void
save_set_msg(Window *window, Message *msg)
{
    // Get panel information
    SaveWindow *self = TUI_SAVE(window);
    g_return_if_fail(self != NULL);
    g_return_if_fail(msg != NULL);

    self->msg = msg;

    // make 'current SIP message' field visible
    field_opts_on(self->fields[FLD_SAVE_MESSAGE], O_ACTIVE | O_VISIBLE);
}

void
save_set_stream(Window *window, Stream *stream)
{
    // Get panel information
    SaveWindow *self = TUI_SAVE(window);
    g_return_if_fail(self != NULL);
    g_return_if_fail(stream != NULL);

    self->stream = stream;
    self->savemode = SAVE_STREAM;
    self->saveformat = SAVE_WAV;

    // make 'current stream' field visible
    field_opts_on(self->fields[FLD_SAVE_STREAM], O_ACTIVE | O_VISIBLE);
    field_opts_on(self->fields[FLD_SAVE_WAV], O_ACTIVE | O_VISIBLE);
    field_opts_off(self->fields[FLD_SAVE_ALL], O_ACTIVE | O_VISIBLE);
}


void
save_win_free(Window *window)
{
    g_object_unref(window);
}

Window *
save_win_new()
{
    return g_object_new(
        WINDOW_TYPE_SAVE,
        "height", 15,
        "width", 68,
        NULL
    );
}

static void
save_finalize(GObject *object)
{
    SaveWindow *self = TUI_SAVE(object);

    // Remove panel form and fields
    unpost_form(self->form);
    free_form(self->form);
    for (gint i = 0; i < FLD_SAVE_COUNT; i++)
        free_field(self->fields[i]);

    // Resume capture
    capture_manager_set_pause(capture_manager_get_instance(), FALSE);

    // Disable cursor position
    curs_set(0);

    // Chain-up parent finalize
    G_OBJECT_CLASS(save_parent_class)->finalize(object);
}

static void
save_constructed(GObject *object)
{
    // Chain-up parent constructed
    G_OBJECT_CLASS(save_parent_class)->constructed(object);

    // Get parent window information
    SaveWindow *self = TUI_SAVE(object);
    Window *parent = TUI_WINDOW(self);
    WINDOW *win = window_get_ncurses_window(parent);
    PANEL *panel = window_get_ncurses_panel(parent);

    gint height = window_get_height(parent);
    gint width = window_get_width(parent);

    // Pause the capture while saving
    capture_manager_set_pause(capture_manager_get_instance(), TRUE);

    // Initialize the fields    int total, displayed;
    self->fields[FLD_SAVE_PATH] = new_field(1, 52, 3, 13, 0, 0);
    self->fields[FLD_SAVE_FILE] = new_field(1, 47, 4, 13, 0, 0);
    self->fields[FLD_SAVE_ALL] = new_field(1, 1, 7, 4, 0, 0);
    self->fields[FLD_SAVE_SELECTED] = new_field(1, 1, 8, 4, 0, 0);
    self->fields[FLD_SAVE_DISPLAYED] = new_field(1, 1, 9, 4, 0, 0);
    self->fields[FLD_SAVE_MESSAGE] = new_field(1, 1, 10, 4, 0, 0);
    self->fields[FLD_SAVE_STREAM] = new_field(1, 1, 7, 4, 0, 0);
    self->fields[FLD_SAVE_PCAP] = new_field(1, 1, 7, 36, 0, 0);
    self->fields[FLD_SAVE_PCAP_RTP] = new_field(1, 1, 8, 36, 0, 0);
    self->fields[FLD_SAVE_TXT] = new_field(1, 1, 9, 36, 0, 0);
    self->fields[FLD_SAVE_WAV] = new_field(1, 1, 7, 36, 0, 0);
    self->fields[FLD_SAVE_SAVE] = new_field(1, 10, height - 2, 20, 0, 0);
    self->fields[FLD_SAVE_CANCEL] = new_field(1, 10, height - 2, 40, 0, 0);
    self->fields[FLD_SAVE_COUNT] = NULL;

    // Set fields options
    field_opts_off(self->fields[FLD_SAVE_PATH], O_STATIC);
    field_opts_off(self->fields[FLD_SAVE_PATH], O_AUTOSKIP);
    field_opts_off(self->fields[FLD_SAVE_FILE], O_STATIC);
    field_opts_off(self->fields[FLD_SAVE_FILE], O_AUTOSKIP);
    field_opts_off(self->fields[FLD_SAVE_ALL], O_AUTOSKIP);
    field_opts_off(self->fields[FLD_SAVE_SELECTED], O_AUTOSKIP);
    field_opts_off(self->fields[FLD_SAVE_DISPLAYED], O_AUTOSKIP);
    field_opts_off(self->fields[FLD_SAVE_DISPLAYED], O_ACTIVE);
    field_opts_off(self->fields[FLD_SAVE_SELECTED], O_ACTIVE);
    field_opts_off(self->fields[FLD_SAVE_MESSAGE], O_ACTIVE);
    field_opts_off(self->fields[FLD_SAVE_STREAM], O_ACTIVE);
    field_opts_off(self->fields[FLD_SAVE_STREAM], O_VISIBLE);
    field_opts_off(self->fields[FLD_SAVE_PCAP], O_ACTIVE);
    field_opts_off(self->fields[FLD_SAVE_PCAP_RTP], O_ACTIVE);
    field_opts_off(self->fields[FLD_SAVE_TXT], O_ACTIVE);
    field_opts_off(self->fields[FLD_SAVE_WAV], O_ACTIVE);
    field_opts_off(self->fields[FLD_SAVE_WAV], O_VISIBLE);

    // Limit max save path and file length
    set_max_field(self->fields[FLD_SAVE_PATH], SETTING_MAX_LEN);
    set_max_field(self->fields[FLD_SAVE_FILE], SETTING_MAX_LEN);

    // Change background of input fields
    set_field_back(self->fields[FLD_SAVE_PATH], A_UNDERLINE);
    set_field_back(self->fields[FLD_SAVE_FILE], A_UNDERLINE);

    // Create the form and post it
    self->form = new_form(self->fields);
    set_form_sub(self->form, win);
    post_form(self->form);
    form_opts_off(self->form, O_BS_OVERLOAD);

    // Set Default field values
    char save_path[SETTING_MAX_LEN];
    sprintf(save_path, "%s", setting_get_value(SETTING_SAVEPATH));

    set_field_buffer(self->fields[FLD_SAVE_PATH], 0, save_path);
    set_field_buffer(self->fields[FLD_SAVE_SAVE], 0, "[  Save  ]");
    set_field_buffer(self->fields[FLD_SAVE_CANCEL], 0, "[ Cancel ]");

    // Set window boxes
    wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    // Window border
    title_foot_box(panel);

    // Header and footer lines
    mvwhline(win, height - 3, 1, ACS_HLINE, width - 1);
    mvwaddch(win, height - 3, 0, ACS_LTEE);
    mvwaddch(win, height - 3, width - 1, ACS_RTEE);

    // Save mode box
    mvwaddch(win, 6, 2, ACS_ULCORNER);
    mvwhline(win, 6, 3, ACS_HLINE, 30);
    mvwaddch(win, 6, 32, ACS_URCORNER);
    mvwvline(win, 7, 2, ACS_VLINE, 4);
    mvwvline(win, 7, 32, ACS_VLINE, 4);
    mvwaddch(win, 11, 2, ACS_LLCORNER);
    mvwhline(win, 11, 3, ACS_HLINE, 30);
    mvwaddch(win, 11, 32, ACS_LRCORNER);

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
    set_current_field(self->form, self->fields[FLD_SAVE_FILE]);
    form_driver(self->form, REQ_END_LINE);
    curs_set(1);

    // Get filter stats
    StorageStats stats = storage_calls_stats();

    // Set default save modes
    StorageCaptureOpts storageCaptureOpts = storage_capture_options();
    self->savemode = (stats.displayed == stats.total) ? SAVE_ALL : SAVE_DISPLAYED;
    self->saveformat = (storageCaptureOpts.rtp) ? SAVE_PCAP_RTP : SAVE_PCAP;
}

static void
save_class_init(SaveWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = save_constructed;
    object_class->finalize = save_finalize;

    WindowClass *window_class = TUI_WINDOW_CLASS(klass);
    window_class->draw = save_draw;
    window_class->handle_key = save_handle_key;

}

static void
save_init(SaveWindow *self)
{
    // Initialize attributes
    window_set_window_type(TUI_WINDOW(self), WINDOW_SAVE);
}
