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
#include "tui/widgets/dialog.h"
#include "tui/windows/save_win.h"

G_DEFINE_TYPE(SaveWindow, save, SNG_TYPE_WINDOW)

SngWindow *
save_win_new()
{
    return g_object_new(
        WINDOW_TYPE_SAVE,
        "title", "Save Capture",
        "border", TRUE,
        "height", 15,
        "width", 68,
        NULL
    );
}

/**
 * @brief Callback function to save a single packet into a capture output
 */
static void
save_packet_cb(Packet *packet, CaptureOutput *output)
{
    capture_output_write(output, packet);
}

#if 0
#ifdef WITH_SND

static void
save_stream_to_file(SaveWindow *save_window)
{
    // Validate save file name
    g_autoptr(GString) filename = g_string_new(sng_entry_get_text(SNG_ENTRY(save_window->en_fname)));
    if (filename->len == 0) {
        dialog_run("Please enter a valid filename");
        return;
    }

    // Prepend save file path
    g_string_prepend(filename, G_DIR_SEPARATOR_S);
    g_string_prepend(filename, sng_entry_get_text(SNG_ENTRY(save_window->en_fpath)));

    // Add extension if not already present
    const gchar *fext = sng_label_get_text(SNG_LABEL(save_window->lb_fext));
    if (g_str_has_suffix(filename->str, fext)) {
        g_string_append(filename, fext);
    }

    if (g_file_test(filename->str, G_FILE_TEST_EXISTS) == 0) {
        if (dialog_confirm("Overwrite confirmation",
                           "Selected file already exits.\n Do you want to overwrite it?",
                           "Yes,No") != 0)
            return;
    }

    SF_INFO file_info = { 0 };
    file_info.samplerate = 8000;
    file_info.channels = 1;
    file_info.format = SF_FORMAT_WAV | SF_FORMAT_GSM610;

    GError *error = NULL;
    g_autoptr(GByteArray) decoded = codec_stream_decode(save_window->stream, NULL, &error);
    if (error != NULL) {
        dialog_run("error: %s", error->message);
        return;
    }

    // Create a new wav file in requested path
    SNDFILE *file = sf_open(filename->str, SFM_WRITE, &file_info);
    if (file == NULL) {
        dialog_run("error: %s", sf_strerror(file));
        return;
    }

    // Save all decoded bytes
    sf_write_short(file, (const gint16 *) decoded->data, decoded->len / 2);

    // Close wav file and free decoded data
    sf_close(file);

    dialog_run("%d bytes decoded into %s", g_byte_array_len(decoded) / 2, filename->str);
}

#endif
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
static void
save_to_file(SaveWindow *save_window)
{
    CaptureOutput *output = NULL;
    gint total = 0;
    GPtrArray *calls = NULL;
    GPtrArray *packets = g_ptr_array_new();
    GError *error = NULL;

    // Validate save file name
    g_autoptr(GString) filename = g_string_new(sng_entry_get_text(SNG_ENTRY(save_window->en_fname)));
    if (filename->len == 0) {
        sng_dialog_show_message(NULL, " <cyan>Please enter a valid filename");
        return;
    }

    // Prepend save file path
    g_string_prepend(filename, G_DIR_SEPARATOR_S);
    g_string_prepend(filename, sng_entry_get_text(SNG_ENTRY(save_window->en_fpath)));

    // Add extension if not already present
    const gchar *fext = sng_label_get_text(SNG_LABEL(save_window->lb_fext));
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

    // Determine save mode from active radio button
    if (sng_radio_button_is_active(SNG_RADIO_BUTTON(save_window->rb_all))) {
        save_window->savemode = SAVE_ALL;
    } else if (sng_radio_button_is_active(SNG_RADIO_BUTTON(save_window->rb_displayed))) {
        save_window->savemode = SAVE_DISPLAYED;
    } else if (sng_radio_button_is_active(SNG_RADIO_BUTTON(save_window->rb_selected))) {
        save_window->savemode = SAVE_SELECTED;
    } else if (sng_radio_button_is_active(SNG_RADIO_BUTTON(save_window->rb_message))) {
        save_window->savemode = SAVE_MESSAGE;
    } else if (sng_radio_button_is_active(SNG_RADIO_BUTTON(save_window->rb_stream))) {
        save_window->savemode = SAVE_STREAM;
    } else {
        g_warning("No save mode selected in Save Window\n");
    }

    // Determine save format from active radio button
    if (sng_radio_button_is_active(SNG_RADIO_BUTTON(save_window->rb_pcap))) {
        save_window->saveformat = SAVE_PCAP;
    } else if (sng_radio_button_is_active(SNG_RADIO_BUTTON(save_window->rb_pcap_rtp))) {
        save_window->saveformat = SAVE_PCAP_RTP;
    } else if (sng_radio_button_is_active(SNG_RADIO_BUTTON(save_window->rb_txt))) {
        save_window->saveformat = SAVE_TXT;
    } else if (sng_radio_button_is_active(SNG_RADIO_BUTTON(save_window->rb_wav))) {
        save_window->saveformat = SAVE_WAV;
    } else {
        g_warning("No save format selected in Save Window\n");
    }

    if (save_window->saveformat == SAVE_PCAP || save_window->saveformat == SAVE_PCAP_RTP) {
        // Open PCAP output file
        output = capture_output_pcap(filename->str, &error);
    } else {
        // Open TXT output file
        output = capture_output_txt(filename->str, &error);
    }

    // Output creation error checking
    if (error != NULL) {
        sng_dialog_show_message(NULL, "Error: %s", error->message);
        return;
    }

    // Get calls iterator
    switch (save_window->savemode) {
        case SAVE_ALL:
        case SAVE_DISPLAYED:
            // Get calls iterator
            calls = storage_calls();
            break;
        case SAVE_SELECTED:
            // Save selected packets to file
            calls = save_window->group->calls;
            break;
        default:
            break;
    }

    if (save_window->savemode == SAVE_MESSAGE) {
        // Save selected message packet to pcap
        capture_output_write(output, save_window->msg->packet);
    } else if (save_window->saveformat == SAVE_TXT) {
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

            if (save_window->savemode == SAVE_DISPLAYED && !filter_check_call(call, NULL))
                continue;

            total += call_msg_count(call);

            if (save_window->saveformat == SAVE_PCAP_RTP) {
                for (guint j = 0; j < g_ptr_array_len(call->streams); j++) {
                    Stream *stream = g_ptr_array_index(call->streams, j);
                    total += stream_get_count(stream);
                }
            }
        }

//        progress = dialog_progress_run("Saving packets...");
//        dialog_progress_set_value(progress, 0);

        // Save selected packets to file
        for (guint i = 0; i < g_ptr_array_len(calls); i++) {
            Call *call = g_ptr_array_index(calls, i);

            if (save_window->savemode == SAVE_DISPLAYED && !filter_check_call(call, NULL))
                continue;

            // Save SIP message content
            for (guint j = 0; j < g_ptr_array_len(call->msgs); j++) {
                Message *msg = g_ptr_array_index(call->msgs, j);
                // Update progress bar dialog
//                dialog_progress_set_value(progress, (++cur * 100) / total);
                g_ptr_array_add(packets, msg->packet);
            }

            // Save RTP packets
            if (save_window->saveformat == SAVE_PCAP_RTP) {
                for (guint j = 0; j < g_ptr_array_len(call->streams); j++) {
                    Stream *stream = g_ptr_array_index(call->streams, j);
//                    dialog_progress_set_value(progress, (++cur * 100) / total);
                    g_ptr_array_add_array(packets, stream->packets);
                }
            }
        }

        g_ptr_array_sort(packets, (GCompareFunc) packet_time_sorter);
        g_ptr_array_foreach(packets, (GFunc) save_packet_cb, output);

//        dialog_progress_destroy(progress);
    }

    // Close saved file
    capture_output_close(output);

    // Show success popup
    if (save_window->savemode == SAVE_MESSAGE) {
        sng_dialog_show_message(
            "Save completed",
            "Successfully saved selected SIP message to %s",
            g_filename_display_basename(filename->str)
        );
    } else {
        sng_dialog_show_message(
            "Save completed",
            "Successfully saved %d packets to %s",
            total,
            g_filename_display_basename(filename->str)
        );
    }

    sng_widget_destroy(SNG_WIDGET(save_window));
}

void
save_set_group(SaveWindow *save_window, CallGroup *group)
{
    // Set dialogs group
    save_window->group = group;

    g_autoptr(GString) selected = g_string_new("selected dialogs");
    g_string_append_printf(selected, " [%d]", call_group_count(group));
    sng_label_set_text(SNG_LABEL(save_window->rb_selected), selected->str);

    // If there are selected calls enable selected radio button
    if (call_group_count(group) != 0) {
        sng_button_activate(SNG_BUTTON(save_window->rb_selected));
    }
}

void
save_set_message(SaveWindow *save_window, Message *msg)
{
    // Set save group
    save_window->msg = msg;

    // Select save current message
    sng_button_activate(SNG_BUTTON(save_window->rb_message));
}

void
save_set_stream(SaveWindow *save_window, Stream *stream)
{
    // Set save group
    save_window->stream = stream;

    // Display the dialog select box
    sng_widget_show(SNG_WIDGET(save_window->rb_stream));

    // Enable PCAP formats
    sng_widget_show(SNG_WIDGET(save_window->rb_wav));
}

static void
save_constructed_file_widgets(SaveWindow *save_window)
{
    // Save Path entry
    SngWidget *box_path = sng_box_new_full(SNG_ORIENTATION_HORIZONTAL, 1, 2);
    sng_widget_set_height(box_path, 1);
    sng_box_pack_start(SNG_BOX(box_path), sng_label_new("Path:    "));
    save_window->en_fpath = sng_entry_new(setting_get_value(SETTING_STORAGE_SAVEPATH));
    sng_container_add(SNG_CONTAINER(box_path), save_window->en_fpath);
    sng_box_pack_start(SNG_BOX(save_window), box_path);
    g_signal_connect_swapped(save_window->en_fpath, "activate",
                             G_CALLBACK(save_to_file), save_window);

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
                             G_CALLBACK(save_to_file), save_window);

    // Set Filename entry as default widget
    sng_window_set_default_focus(SNG_WINDOW(save_window), save_window->en_fname);
}

static void
save_constructed_dialog_widgets(SaveWindow *save_window)
{
    GSList *rb_group = NULL;

    // Dialog box frame
    save_window->box_dialogs = sng_box_new(SNG_ORIENTATION_VERTICAL);
    sng_box_set_border(SNG_BOX(save_window->box_dialogs), TRUE);
    sng_box_set_label(SNG_BOX(save_window->box_dialogs), "Dialogs");

    // All Dialogs Radio Button
    save_window->rb_all = sng_radio_button_new("All dialogs");
    sng_box_pack_start(SNG_BOX(save_window->box_dialogs), save_window->rb_all);
    rb_group = sng_radio_button_group_add(rb_group, SNG_RADIO_BUTTON(save_window->rb_all));

    // Selected Dialogs Radio Button
    save_window->rb_selected = sng_radio_button_new("Selected dialogs");
    rb_group = sng_radio_button_group_add(rb_group, SNG_RADIO_BUTTON(save_window->rb_selected));
    sng_box_pack_start(SNG_BOX(save_window->box_dialogs), save_window->rb_selected);

    // Displayed Dialogs Radio Button
    StorageStats stats = storage_calls_stats();
    g_autoptr(GString) displayed = g_string_new("Displayed dialogs");
    g_string_append_printf(displayed, " [%d]", stats.displayed);
    save_window->rb_displayed = sng_radio_button_new(displayed->str);
    sng_radio_button_group_add(rb_group, SNG_RADIO_BUTTON(save_window->rb_displayed));
    sng_box_pack_start(SNG_BOX(save_window->box_dialogs), save_window->rb_displayed);

    // Current Message Radio Button
    save_window->rb_message = sng_radio_button_new("Current SIP Message");
    sng_radio_button_group_add(rb_group, SNG_RADIO_BUTTON(save_window->rb_message));
    sng_box_pack_start(SNG_BOX(save_window->box_dialogs), save_window->rb_message);

    // Current Message Radio Button
    save_window->rb_stream = sng_radio_button_new("Current Stream");
    sng_radio_button_group_add(rb_group, SNG_RADIO_BUTTON(save_window->rb_stream));
    sng_box_pack_start(SNG_BOX(save_window->box_dialogs), save_window->rb_stream);

    // Activate dialog mode based on storage stats
    if (stats.displayed == stats.total) {
        sng_button_activate(SNG_BUTTON(save_window->rb_all));
    } else {
        sng_button_activate(SNG_BUTTON(save_window->rb_displayed));
    }
}

static void
save_constructed_format_widgets(SaveWindow *save_window)
{
    GSList *rb_group = NULL;

    // Format box frame
    save_window->box_format = sng_box_new(SNG_ORIENTATION_VERTICAL);
    sng_box_set_border(SNG_BOX(save_window->box_format), TRUE);
    sng_box_set_label(SNG_BOX(save_window->box_format), "Format");

    // PCAP Radio Button
    save_window->rb_pcap = sng_radio_button_new(".pcap (SIP)");
    sng_box_pack_start(SNG_BOX(save_window->box_format), save_window->rb_pcap);
    rb_group = sng_radio_button_group_add(rb_group, SNG_RADIO_BUTTON(save_window->rb_pcap));

    // PCAP with RTP Radio Button
    save_window->rb_pcap_rtp = sng_radio_button_new(".pcap (SIP + RTP)");
    rb_group = sng_radio_button_group_add(rb_group, SNG_RADIO_BUTTON(save_window->rb_pcap_rtp));
    sng_box_pack_start(SNG_BOX(save_window->box_format), save_window->rb_pcap_rtp);

    // TXT Radio Button
    save_window->rb_txt = sng_radio_button_new(".txt");
    sng_radio_button_group_add(rb_group, SNG_RADIO_BUTTON(save_window->rb_txt));
    sng_box_pack_start(SNG_BOX(save_window->box_format), save_window->rb_txt);

    // WAV Radio Button
    save_window->rb_wav = sng_radio_button_new(".wav");
    sng_radio_button_group_add(rb_group, SNG_RADIO_BUTTON(save_window->rb_wav));
    sng_box_pack_start(SNG_BOX(save_window->box_format), save_window->rb_wav);

    // Activate format based on storage options
    StorageCaptureOpts storageCaptureOpts = storage_capture_options();
    if (storageCaptureOpts.rtp) {
        sng_button_activate(SNG_BUTTON(save_window->rb_pcap_rtp));
    } else {
        sng_button_activate(SNG_BUTTON(save_window->rb_pcap));
    }
}

static void
save_constructed_options_widgets(SaveWindow *save_window)
{
    SngWidget *box_options = sng_box_new_full(SNG_ORIENTATION_HORIZONTAL, 1, 0);
    sng_box_set_padding_full(SNG_BOX(box_options), 1, 0, 1, 1);

    // Add Dialog selection options
    save_constructed_dialog_widgets(save_window);
    sng_container_add(SNG_CONTAINER(box_options), save_window->box_dialogs);

    // Add Format selection options
    save_constructed_format_widgets(save_window);
    sng_container_add(SNG_CONTAINER(box_options), save_window->box_format);

    // Add boxes to the window
    sng_container_add(SNG_CONTAINER(save_window), box_options);
}

static void
save_constructed(GObject *object)
{
    // Pause the capture while saving
    capture_manager_set_pause(capture_manager_get_instance(), TRUE);

    SaveWindow *save_window = TUI_SAVE(object);
    // File path and name
    save_constructed_file_widgets(save_window);
    // Dialog select options
    save_constructed_options_widgets(save_window);

    SngWidget *bn_save = sng_button_new();
    sng_label_set_text(SNG_LABEL(bn_save), "[   Save   ]");
    sng_window_add_button(SNG_WINDOW(save_window), SNG_BUTTON(bn_save));
    g_signal_connect_swapped(bn_save, "activate",
                             G_CALLBACK(save_to_file), save_window);

    SngWidget *bn_no = sng_button_new();
    sng_label_set_text(SNG_LABEL(bn_no), "[   Cancel   ]");
    sng_window_add_button(SNG_WINDOW(save_window), SNG_BUTTON(bn_no));
    g_signal_connect_swapped(bn_no, "activate",
                             G_CALLBACK(sng_widget_destroy), save_window);

    // Chain-up parent constructed
    G_OBJECT_CLASS(save_parent_class)->constructed(object);
}

static void
save_init(G_GNUC_UNUSED SaveWindow *self)
{
}

static void
save_class_init(SaveWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = save_constructed;
}
