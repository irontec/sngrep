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
#include <gio/gio.h>
#include <unistd.h>
#ifdef  WITH_SND
#include <sndfile.h>
#endif
#include "codecs/codec.h"
#include "glib-extra/glib.h"
#include "setting.h"
#include "storage/filter.h"
#include "capture/capture_txt.h"
#include "tui/widgets/dialog.h"
#include "tui/windows/save_win.h"

G_DEFINE_TYPE(SaveWindow, save_win, SNG_TYPE_WINDOW)

SngWindow *
save_win_new()
{
    return g_object_new(
        SNG_TYPE_SAVE_WIN,
        "title", "Save Capture",
        "border", TRUE,
        "height", 15,
        "width", 68,
        NULL
    );
}

#ifdef WITH_SND

static void
save_win_stream_to_file(SaveWindow *save_window, GString *filename, GError **error)
{
    SF_INFO file_info = { 0 };
    file_info.samplerate = 8000;
    file_info.channels = 1;
    file_info.format = SF_FORMAT_WAV | SF_FORMAT_GSM610;

    g_autoptr(GByteArray) decoded = codec_stream_decode(save_window->stream, NULL, error);
    if (error != NULL) {
        return;
    }

    // Create a new wav file in requested path
    SNDFILE *file = sf_open(filename->str, SFM_WRITE, &file_info);
    if (file == NULL) {
        g_set_error(error,
                    SAVE_ERROR,
                    SAVE_ERROR_SND_OPEN,
                    "%s",
                    sf_strerror(file));
        return;
    }

    // Save all decoded bytes
    sf_write_short(file, (const gint16 *) decoded->data, decoded->len / 2);

    // Close wav file and free decoded data
    sf_close(file);

    sng_dialog_show_message(
        NULL,
        "%d bytes decoded into %s",
        g_byte_array_len(decoded) / 2,
        filename->str
    );
}

#endif


static void
save_win_save_worker(GTask *task, SaveWindow *save_window, gpointer task_data, G_GNUC_UNUSED GCancellable *cancellable)
{
    GPtrArray *calls = NULL;
    GPtrArray *packets = g_ptr_array_new();
    SngDialog *progress = SNG_DIALOG(task_data);

    // Get calls iterator
    switch (save_window->mode) {
        case SAVE_MODE_ALL:
        case SAVE_MODE_DISPLAYED:
            // Get calls iterator
            calls = storage_calls();
            break;
        case SAVE_MODE_SELECTED:
            // Save selected packets to file
            calls = save_window->group->calls;
            break;
        default:
            break;
    }

    // Update progress status
    sng_dialog_set_message(progress, "Preparing packets ...");

    gdouble total = 0;
    if (save_window->mode == SAVE_MODE_MESSAGE) {
        // Save selected message packet to pcap
        capture_output_write(save_window->output, save_window->msg->packet);
        total = 1;
    } else if (save_window->format == SAVE_FORMAT_TXT) {
        // Save selected packets to file
        for (guint i = 0; i < g_ptr_array_len(calls); i++) {
            Call *call = g_ptr_array_index(calls, i);
            total += call_msg_count(call);
            // Save SIP message content
            for (guint j = 0; j < g_ptr_array_len(call->msgs); j++) {
                Message *msg = g_ptr_array_index(call->msgs, j);
                capture_output_write(save_window->output, msg->packet);
            }
        }
    } else {
        // Count packages for progress bar
        for (guint i = 0; i < g_ptr_array_len(calls); i++) {
            Call *call = g_ptr_array_index(calls, i);

            if (save_window->mode == SAVE_MODE_DISPLAYED && !filter_check_call(call, NULL))
                continue;

            total += call_msg_count(call);

            if (save_window->format == SAVE_FORMAT_PCAP_RTP) {
                for (guint j = 0; j < g_ptr_array_len(call->streams); j++) {
                    Stream *stream = g_ptr_array_index(call->streams, j);
                    total += stream_get_count(stream);
                }
            }
        }

        // Save selected packets to file
        gdouble count = 0;
        for (guint i = 0; i < g_ptr_array_len(calls); i++) {
            Call *call = g_ptr_array_index(calls, i);

            if (save_window->mode == SAVE_MODE_DISPLAYED && !filter_check_call(call, NULL))
                continue;

            // Save SIP message content
            for (guint j = 0; j < g_ptr_array_len(call->msgs); j++) {
                Message *msg = g_ptr_array_index(call->msgs, j);
                // Update progress bar dialog
                sng_dialog_progress_set_fraction(progress, count++ / total);
                g_ptr_array_add(packets, msg->packet);
            }

            // Save RTP packets
            if (save_window->format == SAVE_FORMAT_PCAP_RTP) {
                for (guint j = 0; j < g_ptr_array_len(call->streams); j++) {
                    Stream *stream = g_ptr_array_index(call->streams, j);
                    sng_dialog_progress_set_fraction(progress, count++ / total);
                    g_ptr_array_add_array(packets, stream->packets);
                }
            }
        }

        sng_dialog_set_message(progress, "Sorting packets ...");
        g_ptr_array_sort(packets, (GCompareFunc) packet_time_sorter);

        sng_dialog_set_message(progress, "Saving packets ...");
        for (guint i = 0; i < g_ptr_array_len(packets); i++) {
            capture_output_write(save_window->output, g_ptr_array_index(packets, i));
            sng_dialog_progress_set_fraction(progress, (i + 1) / g_ptr_array_len(packets));
        }
    }

    // Return the number of packets saved
    g_task_return_int(task, total);

    // Hide progress dialog
    sng_widget_hide(SNG_WIDGET(progress));
}

static void
save_win_save_finished(GObject *object, GAsyncResult *res, gpointer user_data)
{
    SaveWindow *save_window = SNG_SAVE_WIN(object);
    g_return_if_fail(save_window != NULL);

    GString *filename = user_data;
    g_return_if_fail(filename != NULL);

    // Show success popup
    if (save_window->mode == SAVE_MODE_MESSAGE) {
        sng_dialog_show_message(
            "Save completed",
            "Successfully saved selected SIP message to %s",
            g_filename_display_basename(capture_output_sink(save_window->output))
        );
    } else {
        sng_dialog_show_message(
            "Save completed",
            "Successfully saved %d packets to %s",
            g_task_propagate_int(G_TASK(res), NULL),
            g_filename_display_basename(capture_output_sink(save_window->output))
        );
    }

    // Close saved file
    capture_output_close(save_window->output);

    // Destroy save window
    sng_widget_destroy(SNG_WIDGET(save_window));
}

static void
save_win_dialogs_to_file(SaveWindow *save_win, GString *filename, GError **error)
{
    GError *output_error = NULL;

    if (save_win->format == SAVE_FORMAT_PCAP || save_win->format == SAVE_FORMAT_PCAP_RTP) {
        // Open PCAP output file
        save_win->output = capture_output_pcap(filename->str, &output_error);
    } else {
        // Open TXT output file
        save_win->output = capture_output_txt(filename->str, &output_error);
    }

    // Output creation error checking
    if (output_error != NULL) {
        g_propagate_error(error, output_error);
        return;
    }

    SngWidget *progress = sng_dialog_new(
        SNG_DIALOG_PROGRESS,
        SNG_BUTTONS_CANCEL,
        "Saving packets",
        "Preparing packets ..."
    );
    sng_window_update(SNG_WINDOW(progress));

    GTask *save_task = g_task_new(save_win, NULL, save_win_save_finished, filename);
    g_task_set_task_data(save_task, progress, (GDestroyNotify) sng_widget_destroy);
    g_task_set_return_on_cancel(save_task, TRUE);
    g_task_run_in_thread(save_task, (GTaskThreadFunc) save_win_save_worker);
    g_object_unref(save_task);
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
static void
save_win_activated(SaveWindow *save_win)
{
    GError *error = NULL;

    // Validate save file name
    g_autoptr(GString) filename = g_string_new(sng_entry_get_text(SNG_ENTRY(save_win->en_fname)));
    if (filename->len == 0) {
        sng_dialog_show_message(NULL, " <cyan>Please enter a valid filename");
        return;
    }

    // Prepend save file path
    g_string_prepend(filename, G_DIR_SEPARATOR_S);
    g_string_prepend(filename, sng_entry_get_text(SNG_ENTRY(save_win->en_fpath)));

    // Add extension if not already present
    const gchar *fext = sng_label_get_text(SNG_LABEL(save_win->lb_fext));
    if (!g_str_has_suffix(filename->str, fext)) {
        g_string_append(filename, fext);
    }

    if (g_file_test(filename->str, G_FILE_TEST_EXISTS)) {
        gboolean overwrite = sng_dialog_confirm(
            "Overwrite confirmation",
            "<cyan> Selected file already exits.\n Do you want to overwrite it?"
        );
        if (!overwrite) {
            return;
        }
    }

    switch (save_win->mode) {
        case SAVE_MODE_ALL:
        case SAVE_MODE_DISPLAYED:
        case SAVE_MODE_SELECTED:
        case SAVE_MODE_MESSAGE:
            save_win_dialogs_to_file(save_win, filename, &error);
            break;
        case SAVE_MODE_STREAM:
#ifdef WITH_SND
            save_win_stream_to_file(save_win, filename, &error);
#endif
            break;
    }

    // Print save errors
    if (error != NULL) {
        sng_dialog_show_message(NULL, "Error: %s", error->message);
        return;
    }
}

void
save_win_set_group(SaveWindow *save_window, CallGroup *group)
{
    // Set dialogs group
    save_window->group = group;

    // Show group options
    sng_widget_show(save_window->rb_all);
    sng_widget_show(save_window->rb_pcap);
    sng_widget_show(save_window->rb_txt);

    // Show Displayed option if no dialog is being filtered
    StorageStats stats = storage_calls_stats();
    if (stats.displayed == stats.total) {
        // Mark All dialogs as default selected option
        sng_button_activate(SNG_BUTTON(save_window->rb_all));
    } else {
        g_autoptr(GString) displayed = g_string_new("Displayed dialogs");
        g_string_append_printf(displayed, " [%d]", stats.displayed);
        sng_label_set_text(SNG_LABEL(save_window->rb_displayed), displayed->str);
        sng_widget_show(save_window->rb_displayed);
        // Mark displayed dialogs as default selected option
        sng_button_activate(SNG_BUTTON(save_window->rb_displayed));
    }

    // Show Selected option if there are dialogs selected
    if (call_group_count(group) != 0) {
        g_autoptr(GString) selected = g_string_new("Selected dialogs");
        g_string_append_printf(selected, " [%d]", call_group_count(group));
        sng_label_set_text(SNG_LABEL(save_window->rb_selected), selected->str);
        sng_widget_show(save_window->rb_selected);
        // Mark Selected dialogs as default selected option
        sng_button_activate(SNG_BUTTON(save_window->rb_selected));
    }

    // Activate format based on storage options
    StorageCaptureOpts storageCaptureOpts = storage_capture_options();
    if (storageCaptureOpts.rtp) {
        sng_widget_show(save_window->rb_pcap_rtp);
        sng_button_activate(SNG_BUTTON(save_window->rb_pcap_rtp));
    } else {
        sng_button_activate(SNG_BUTTON(save_window->rb_pcap));
    }
}

void
save_win_set_message(SaveWindow *save_window, Message *msg)
{
    // Set save group
    save_window->msg = msg;

    // Hide all non-message settings
    sng_widget_show(save_window->rb_message);
    sng_widget_show(save_window->rb_pcap);
    sng_widget_show(save_window->rb_txt);

    // Select save current message
    sng_button_activate(SNG_BUTTON(save_window->rb_message));
}

void
save_win_set_stream(SaveWindow *save_window, Stream *stream)
{
    // Set save group
    save_window->stream = stream;

    // Display the dialog select box
    sng_widget_show(save_window->rb_stream);
    sng_widget_show(save_window->rb_wav);

    // Enable PCAP formats
    sng_widget_show(SNG_WIDGET(save_window->rb_wav));
}

static void
save_win_update_mode(SngWidget *selected, SaveWindow *save_win)
{
    // Determine save mode from active radio button
    if (selected == save_win->rb_all) {
        save_win->mode = SAVE_MODE_ALL;
    } else if (selected == save_win->rb_displayed) {
        save_win->mode = SAVE_MODE_DISPLAYED;
    } else if (selected == save_win->rb_selected) {
        save_win->mode = SAVE_MODE_SELECTED;
    } else if (selected == save_win->rb_message) {
        save_win->mode = SAVE_MODE_MESSAGE;
    } else if (selected == save_win->rb_stream) {
        save_win->mode = SAVE_MODE_STREAM;
    } else {
        g_warning("No save mode selected in Save Window\n");
    }
}

static void
save_win_update_format(SngWidget *selected, SaveWindow *save_win)
{
    if (selected == save_win->rb_pcap) {
        save_win->format = SAVE_FORMAT_PCAP;
        sng_label_set_text(SNG_LABEL(save_win->lb_fext), ".pcap");
    } else if (selected == save_win->rb_pcap_rtp) {
        save_win->format = SAVE_FORMAT_PCAP_RTP;
        sng_label_set_text(SNG_LABEL(save_win->lb_fext), ".pcap");
    } else if (selected == save_win->rb_txt) {
        save_win->format = SAVE_FORMAT_TXT;
        sng_label_set_text(SNG_LABEL(save_win->lb_fext), ".txt");
    } else if (selected == save_win->rb_wav) {
        save_win->format = SAVE_FORMAT_WAV;
        sng_label_set_text(SNG_LABEL(save_win->lb_fext), ".wav");
    } else {
        g_warning("No save format selected in Save Window\n");
    }
}

static void
save_win_finalize(GObject *object)
{
    // Unpause the capture while saving
    capture_manager_set_pause(capture_manager_get_instance(), FALSE);

    // Chain-up parent finalize
    G_OBJECT_CLASS(save_win_parent_class)->finalize(object);
}

static void
save_win_constructed(GObject *object)
{
    // Pause the capture while saving
    capture_manager_set_pause(capture_manager_get_instance(), TRUE);

    SaveWindow *save_window = SNG_SAVE_WIN(object);

    // Save Path entry
    SngWidget *box_path = sng_box_new_full(SNG_ORIENTATION_HORIZONTAL, 1, 2);
    sng_widget_set_height(box_path, 1);
    sng_box_pack_start(SNG_BOX(box_path), sng_label_new("Path:    "));
    save_window->en_fpath = sng_entry_new(setting_get_value(SETTING_STORAGE_SAVEPATH));
    sng_container_add(SNG_CONTAINER(box_path), save_window->en_fpath);
    sng_box_pack_start(SNG_BOX(save_window), box_path);
    g_signal_connect_swapped(save_window->en_fpath, "activate",
                             G_CALLBACK(save_win_activated), save_window);

    // Filename entry
    SngWidget *box_fname = sng_box_new_full(SNG_ORIENTATION_HORIZONTAL, 1, 2);
    sng_widget_set_height(box_fname, 1);
    sng_box_pack_start(SNG_BOX(box_fname), sng_label_new("Filename:"));
    save_window->en_fname = sng_entry_new(NULL);
    sng_container_add(SNG_CONTAINER(box_fname), save_window->en_fname);
    save_window->lb_fext = sng_label_new(".pcap");
    sng_box_pack_start(SNG_BOX(box_fname), save_window->lb_fext);
    sng_box_pack_start(SNG_BOX(save_window), box_fname);
    g_signal_connect_swapped(save_window->en_fname, "activate",
                             G_CALLBACK(save_win_activated), save_window);
    g_signal_connect_swapped(save_window->en_fname, "action::cancel",
                             G_CALLBACK(sng_widget_destroy), SNG_WIDGET(save_window));

    // Dialog box frame
    save_window->box_dialogs = sng_box_new(SNG_ORIENTATION_VERTICAL);
    sng_box_set_border(SNG_BOX(save_window->box_dialogs), TRUE);
    sng_box_set_label(SNG_BOX(save_window->box_dialogs), "Dialogs");

    // Dialog select radio group
    GSList *rb_group_dialogs = NULL;

    // All Dialogs Radio Button
    save_window->rb_all = sng_radio_button_new("All dialogs");
    sng_widget_hide(save_window->rb_all);
    sng_box_pack_start(SNG_BOX(save_window->box_dialogs), save_window->rb_all);
    rb_group_dialogs = sng_radio_button_group_add(rb_group_dialogs, SNG_RADIO_BUTTON(save_window->rb_all));
    g_signal_connect(save_window->rb_all, "activate",
                     G_CALLBACK(save_win_update_mode), save_window);

    // Selected Dialogs Radio Button
    save_window->rb_selected = sng_radio_button_new("Selected dialogs");
    sng_widget_hide(save_window->rb_selected);
    rb_group_dialogs = sng_radio_button_group_add(rb_group_dialogs, SNG_RADIO_BUTTON(save_window->rb_selected));
    sng_box_pack_start(SNG_BOX(save_window->box_dialogs), save_window->rb_selected);
    g_signal_connect(save_window->rb_selected, "activate",
                     G_CALLBACK(save_win_update_mode), save_window);

    // Displayed Dialogs Radio Button
    save_window->rb_displayed = sng_radio_button_new("Selected dialogs");
    sng_widget_hide(save_window->rb_displayed);
    sng_radio_button_group_add(rb_group_dialogs, SNG_RADIO_BUTTON(save_window->rb_displayed));
    sng_box_pack_start(SNG_BOX(save_window->box_dialogs), save_window->rb_displayed);
    g_signal_connect(save_window->rb_displayed, "activate",
                     G_CALLBACK(save_win_update_mode), save_window);

    // Current Message Radio Button
    save_window->rb_message = sng_radio_button_new("Current SIP Message");
    sng_widget_hide(save_window->rb_message);
    sng_radio_button_group_add(rb_group_dialogs, SNG_RADIO_BUTTON(save_window->rb_message));
    sng_box_pack_start(SNG_BOX(save_window->box_dialogs), save_window->rb_message);
    g_signal_connect(save_window->rb_message, "activate",
                     G_CALLBACK(save_win_update_mode), save_window);

    // Current Message Radio Button
    save_window->rb_stream = sng_radio_button_new("Current Stream");
    sng_widget_hide(save_window->rb_stream);
    sng_radio_button_group_add(rb_group_dialogs, SNG_RADIO_BUTTON(save_window->rb_stream));
    sng_box_pack_start(SNG_BOX(save_window->box_dialogs), save_window->rb_stream);
    g_signal_connect(save_window->rb_stream, "activate",
                     G_CALLBACK(save_win_update_mode), save_window);


    // Format box frame
    save_window->box_format = sng_box_new(SNG_ORIENTATION_VERTICAL);
    sng_box_set_border(SNG_BOX(save_window->box_format), TRUE);
    sng_box_set_label(SNG_BOX(save_window->box_format), "Format");

    // Format select radio group
    GSList *rb_group_format = NULL;

    // PCAP Radio Button
    save_window->rb_pcap = sng_radio_button_new(".pcap (SIP)");
    sng_widget_hide(save_window->rb_pcap);
    sng_box_pack_start(SNG_BOX(save_window->box_format), save_window->rb_pcap);
    rb_group_format = sng_radio_button_group_add(rb_group_format, SNG_RADIO_BUTTON(save_window->rb_pcap));
    g_signal_connect(save_window->rb_pcap, "activate",
                     G_CALLBACK(save_win_update_format), save_window);

    // PCAP with RTP Radio Button
    save_window->rb_pcap_rtp = sng_radio_button_new(".pcap (SIP + RTP)");
    sng_widget_hide(save_window->rb_pcap_rtp);
    rb_group_format = sng_radio_button_group_add(rb_group_format, SNG_RADIO_BUTTON(save_window->rb_pcap_rtp));
    sng_box_pack_start(SNG_BOX(save_window->box_format), save_window->rb_pcap_rtp);
    g_signal_connect(save_window->rb_pcap_rtp, "activate",
                     G_CALLBACK(save_win_update_format), save_window);

    // TXT Radio Button
    save_window->rb_txt = sng_radio_button_new(".txt");
    sng_widget_hide(save_window->rb_txt);
    sng_radio_button_group_add(rb_group_format, SNG_RADIO_BUTTON(save_window->rb_txt));
    sng_box_pack_start(SNG_BOX(save_window->box_format), save_window->rb_txt);
    g_signal_connect(save_window->rb_txt, "activate",
                     G_CALLBACK(save_win_update_format), save_window);

    // WAV Radio Button
    save_window->rb_wav = sng_radio_button_new(".wav");
    sng_widget_hide(save_window->rb_wav);
    sng_radio_button_group_add(rb_group_format, SNG_RADIO_BUTTON(save_window->rb_wav));
    sng_box_pack_start(SNG_BOX(save_window->box_format), save_window->rb_wav);
    g_signal_connect(save_window->rb_wav, "activate",
                     G_CALLBACK(save_win_update_format), save_window);


    SngWidget *bn_save = sng_button_new();
    sng_label_set_text(SNG_LABEL(bn_save), "[   Save   ]");
    sng_window_add_button(SNG_WINDOW(save_window), SNG_BUTTON(bn_save));
    g_signal_connect_swapped(bn_save, "activate",
                             G_CALLBACK(save_win_activated), save_window);

    SngWidget *bn_no = sng_button_new();
    sng_label_set_text(SNG_LABEL(bn_no), "[   Cancel   ]");
    sng_window_add_button(SNG_WINDOW(save_window), SNG_BUTTON(bn_no));
    g_signal_connect_swapped(bn_no, "activate",
                             G_CALLBACK(sng_widget_destroy), save_window);

    // Layout box for save options
    SngWidget *box_options = sng_box_new_full(SNG_ORIENTATION_HORIZONTAL, 1, 0);
    sng_box_set_padding_full(SNG_BOX(box_options), 1, 0, 1, 1);
    sng_container_add(SNG_CONTAINER(box_options), save_window->box_dialogs);
    sng_container_add(SNG_CONTAINER(box_options), save_window->box_format);
    sng_container_add(SNG_CONTAINER(save_window), box_options);

    // Set Filename entry as default widget
    sng_window_set_default_focus(SNG_WINDOW(save_window), save_window->en_fname);

    // Chain-up parent constructed
    G_OBJECT_CLASS(save_win_parent_class)->constructed(object);
}

static void
save_win_init(G_GNUC_UNUSED SaveWindow *self)
{
}

static void
save_win_class_init(SaveWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = save_win_constructed;
    object_class->finalize = save_win_finalize;
}
