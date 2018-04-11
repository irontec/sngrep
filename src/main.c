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
 * @file main.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of initial functions used by sngrep
 */

#include "config.h"
#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "option.h"
#include "glib-utils.h"
#include "capture.h"
#include "capture_eep.h"
#ifdef WITH_GNUTLS
#include "capture_gnutls.h"
#endif
#ifdef WITH_OPENSSL
#include "capture_openssl.h"
#endif
#include "curses/ui_manager.h"

void
print_version_info()
{
    printf("%s - %s\n"
           "Copyright (C) 2013-2018 Irontec S.L.\n"
           "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n"
           "This is free software: you are free to change and redistribute it.\n"
           "There is NO WARRANTY, to the extent permitted by law.\n"

#ifdef WITH_GNUTLS
           " * Compiled with GnuTLS support.\n"
#endif
#ifdef WITH_OPENSSL
           " * Compiled with OpenSSL support.\n"
#endif
#ifdef WITH_UNICODE
           " * Compiled with Wide-character support.\n"
#endif
#ifdef USE_IPV6
           " * Compiled with IPv6 support.\n"
#endif
#ifdef USE_EEP
            " * Compiled with EEP/HEP support.\n"
#endif
           "\nWritten by Ivan Alonso [aka Kaian]\n",
           PACKAGE, VERSION);
}

/**
 * @brief Main function logic
 *
 * Parse command line options and start running threads
 */
int
main(int argc, char* argv[])
{
    GError *error = NULL;
    gchar **file_inputs = NULL;
    gchar **devices = NULL;
    gchar *hep_listen = NULL;
    gchar *hep_send = NULL;
    gboolean no_interface = FALSE;
    gboolean quiet = FALSE;
    gboolean version = FALSE;
    gboolean config_dump = FALSE;
    gboolean no_config = FALSE;
    gchar *config_file = NULL;
    gchar *keyfile = NULL;
    SStorageSortOpts storage_sopts = {};
    SStorageMatchOpts storage_mopts = {};
    SStorageCaptureOpts storage_copts = {};
    GSequence *infiles = g_sequence_new(NULL);
    GSequence*indevices = g_sequence_new(NULL);

    GOptionEntry main_entries[] = {
        { "version",'V', 0, G_OPTION_ARG_NONE, &version,
          "Version information", NULL },
        { "device", 'd', 0, G_OPTION_ARG_STRING_ARRAY,  &devices,
          "Use this capture device instead of default", "DEVICE" },
        { "input",  'I', 0, G_OPTION_ARG_FILENAME_ARRAY, &file_inputs,
          "Read captured data from pcap file", "FILE" },
        { "output", 'O', 0, G_OPTION_ARG_FILENAME, storage_copts.outfile,
          "Write captured data to pcap file", "FILE" },
        { "calls",  'c', 0, G_OPTION_ARG_NONE, &storage_mopts.invite,
          "Only display dialogs starting with INVITE", NULL },
        { "rtp",    'r', 0, G_OPTION_ARG_NONE, &storage_copts.rtp,
          "Store captured RTP packets (enables saving RTP)", NULL },
        { "limit",  'l', 0, G_OPTION_ARG_INT, &storage_copts.limit,
          "Set capture limit to N dialogs", "N" },
        { "rotate", 'R', 0, G_OPTION_ARG_NONE, &storage_copts.rotate,
          "Rotate calls when capture limit have been reached", NULL },
        { "no-interface", 'N', 0, G_OPTION_ARG_NONE, &no_interface,
          "Don't display sngrep interface, just capture", NULL },
        { "quiet",  'q', 0, G_OPTION_ARG_NONE, &quiet,
          "Don't print captured dialogs in no interface mode", NULL },
        { "icase",  'i', 0, G_OPTION_ARG_NONE, &storage_mopts.micase,
          "Make <match expression> case insensitive", NULL },
        { "invert", 'v', 0, G_OPTION_ARG_NONE, &storage_mopts.minvert,
          "Invert <match expression>", NULL },
        { "dump-config", 'D', 0, G_OPTION_ARG_NONE, &config_dump,
          "Print active configuration settings and exit", NULL },
        { "config", 'f', 0, G_OPTION_ARG_FILENAME, &config_file,
          "Read configuration from FILE", "FILE" },
        { "no-config", 'F', 0, G_OPTION_ARG_NONE, &no_config,
          "Do not read configuration from default config file", NULL },
#if defined(WITH_GNUTLS) || defined(WITH_OPENSSL)
        { "keyfile", 'k', 0, G_OPTION_ARG_FILENAME, &keyfile,
            "RSA private keyfile to decrypt captured packets", "KEYFILE" },
#endif
#ifdef USE_EEP
        { "hep-listen", 'L', 0, G_OPTION_ARG_STRING, &hep_listen,
            "Listen for encapsulated HEP packets", "udp:X.X.X.X:XXXX" },
        { "hep-send", 'H', 0, G_OPTION_ARG_STRING, &hep_send,
            "Homer sipcapture URL", "udp:X.X.X.X:XXXX" },
#endif
        { NULL }
    };

    /************************** Command Line Parsing **************************/
    GOptionContext *context = g_option_context_new("[<match expression>] [<bpf filter>]");
    g_option_context_add_main_entries (context, main_entries, NULL);
    g_option_context_parse (context, &argc, &argv, &error);
    g_option_context_free (context);

    if (error != NULL) {
        g_printerr("Options parsing failed: %s\n", error->message);
        return 1;
    }

    // Parse command line arguments that have high priority
    if (version) {
        print_version_info();
        return 0;
    } else if (config_dump) {
        key_bindings_dump();
        settings_dump();
        return 0;
    }

    /***************************** Configuration *****************************/
    // Initialize configuration options
    init_options(no_config);

    // Override default configuration options if requested
    if (config_file)
        read_options(config_file);

    // Get initial values for configurable arguments
    if (!storage_mopts.invite)
        storage_mopts.invite = setting_enabled(SETTING_SIP_CALLS);
    if (!storage_mopts.complete)
        storage_mopts.complete = setting_enabled(SETTING_SIP_NOINCOMPLETE);
    if (!storage_copts.limit)
        storage_copts.limit = setting_get_intvalue(SETTING_CAPTURE_LIMIT);
    if (!storage_copts.rtp)
        storage_copts.rtp = setting_enabled(SETTING_CAPTURE_RTP);
    if (!storage_copts.rotate)
        storage_copts.rotate = setting_enabled(SETTING_CAPTURE_ROTATE);
    if (!storage_copts.outfile)
        storage_copts.outfile = g_strdup(setting_get_value(SETTING_CAPTURE_OUTFILE));

#if defined(WITH_GNUTLS) || defined(WITH_OPENSSL)
    if (!keyfile) keyfile = g_strdup(setting_get_value(SETTING_CAPTURE_KEYFILE));
#endif

    // Handle capture inputs
    if (devices) {
        for (int i = 0; devices[i]; i++) {
            g_sequence_append(indevices, devices[i]);
        }
    }

    if (file_inputs) {
        for (int i = 0; file_inputs[i]; i++) {
            g_sequence_append(infiles, file_inputs[i]);
        }
    }

#ifdef USE_EEP
    // Hep settings
    if (hep_listen) capture_eep_set_server_url(hep_listen);
    if (hep_send)  capture_eep_set_client_url(hep_send);
#endif

#if defined(WITH_GNUTLS) || defined(WITH_OPENSSL)
    // Set capture decrypt key file
    capture_set_keyfile(keyfile);
    // Check if we have a keyfile and is valid
    if (keyfile && !tls_check_keyfile(keyfile)) {
        fprintf(stderr, "%s does not contain a valid RSA private key.\n", keyfile);
        return 1;
    }
#endif

    // Check if given argument is a file
    if (argc == 2 && (access(argv[1], F_OK) == 0)) {
        // Old legacy option to open pcaps without other arguments
        printf("%s seems to be a file: You forgot -I flag?\n", argv[1]);
        return 0;
    }

    // If no device or files has been specified in command line, use default
    if (g_sequence_get_length(indevices) == 0 && g_sequence_get_length(infiles) == 0) {
        g_sequence_append(indevices, (gpointer) setting_get_value(SETTING_CAPTURE_DEVICE));
    }

    // Set capture options
    capture_init(storage_copts.limit, storage_copts.rtp, storage_copts.rotate);

#ifdef USE_EEP
    // Initialize EEP if enabled
    capture_eep_init();
#endif

    // If we have an input file, load it
    for (int i = 0; i < g_sequence_get_length(infiles); i++) {
        // Try to load file
        if (capture_offline(g_sequence_nth(infiles, i), storage_copts.outfile) != 0)
            return 1;
    }

    // If we have an input device, load it
    for (int i = 0; i < g_sequence_get_length(indevices); i++) {
        // Check if all capture data is valid
        if (capture_online(g_sequence_nth(indevices, i), storage_copts.outfile) != 0)
            return 1;
    }

    // Remove Input files vector
    g_sequence_free(infiles);
    g_sequence_free(indevices);

    // More arguments pending!
    if (argc > 1) {
        // Assume first pending argument is match expression
        guint argi = 1;
        gchar *match_expr = argv[argi++];

        // Try to build the bpf filter string with the rest
        GString *bpf_filter = g_string_new(NULL);
        while (argv[argi]) {
            g_string_append_printf(bpf_filter, " %s", argv[argi]);
            argi++;
        }

        // Check if this BPF filter is valid
        if (capture_set_bpf_filter(bpf_filter->str) != 0) {
            // BPF Filter invalid, check incluiding match_expr
            g_string_prepend(bpf_filter, match_expr);
            match_expr = NULL;

            // Check bpf filter is valid again
            if (capture_set_bpf_filter(bpf_filter->str) != 0) {
                fprintf(stderr, "Couldn't install filter %s: %s\n", bpf_filter->str, capture_last_error());
                return 1;
            }
        }

        // Set the payload match expression
        storage_mopts.mexpr = match_expr;
    }

    // Initialize SIP Messages Storage
    if (!sip_init(storage_copts, storage_mopts, storage_sopts, &error)) {
        g_printerr("Failed to initialize storage: %s\n", error->message);
        return 1;
    };

    // Start a capture thread
    if (capture_launch_thread() != 0) {
        ncurses_deinit();
        fprintf(stderr, "Failed to launch capture thread.\n");
        return 1;
    }

    if (!no_interface) {
        // Initialize interface
        ncurses_init();
        // This is a blocking call.
        // Create the first panel and wait for user input
        ui_create_panel(PANEL_CALL_LIST);
        ui_wait_for_input();
    } else {
        setbuf(stdout, NULL);
        while(capture_is_running()) {
            if (!quiet)
                printf("\rDialog count: %d", sip_calls_count());
            usleep(500 * 1000);
        }
        if (!quiet)
            printf("\rDialog count: %d\n", sip_calls_count());
    }

    // Capture deinit
    capture_deinit();

    // Deinitialize interface
    ncurses_deinit();

    // Deinitialize configuration options
    deinit_options();

    // Deallocate sip stored messages
    sip_deinit();

    // Leaving!
    return 0;
}
