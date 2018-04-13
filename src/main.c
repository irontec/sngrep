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
#include "option.h"
#include "capture/capture.h"
#include "capture/capture_hep.h"
#ifdef WITH_GNUTLS
#include "capture/capture_gnutls.h"
#endif
#ifdef WITH_OPENSSL
#include "capture/capture_openssl.h"
#endif
#include "curses/ui_manager.h"

void
print_version_info()
{
    g_print("%s - %s\n"
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
#ifdef USE_HEP
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
    gchar **input_files = NULL;
    gchar **input_devices = NULL;
    gchar *hep_listen = NULL;
    gchar *hep_send = NULL;
    gboolean no_interface = FALSE;
    gboolean quiet = FALSE;
    gboolean version = FALSE;
    gboolean config_dump = FALSE;
    gboolean no_config = FALSE;
    gchar *output_file = NULL;
    gchar *config_file = NULL;
    gchar *keyfile = NULL;
    SStorageSortOpts storage_sopts = {};
    SStorageMatchOpts storage_mopts = {};
    SStorageCaptureOpts storage_copts = {};
    CaptureManager *manager;
    CaptureInput *input;
    CaptureOutput *output;

    GOptionEntry main_entries[] = {
        { "version",'V', 0, G_OPTION_ARG_CALLBACK, &version,
          "Version information", NULL },
        { "device", 'd', 0, G_OPTION_ARG_STRING_ARRAY,  &input_devices,
          "Use this capture device instead of default", "DEVICE" },
        { "input",  'I', 0, G_OPTION_ARG_FILENAME_ARRAY, &input_files,
          "Read captured data from pcap file", "FILE" },
        { "output", 'O', 0, G_OPTION_ARG_FILENAME, &output_file,
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
#ifdef USE_HEP
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

    /***************************** Capture Inputs *****************************/
    // Main packet capture manager
    manager = capture_manager_new();

    // Handle capture file inputs
    if (input_files) {
        for (gint i = 0; input_files[i]; i++) {
            if ((input = capture_input_pcap_offline(input_files[i], &error))) {
                capture_manager_add_input(manager, input);
            } else {
                g_printerr("error: %s", error->message);
                return 1;
            }
        }
    }

    // Handle capture device inputs
    if (input_devices) {
        for (gint i = 0; input_devices[i]; i++) {
            if ((input = capture_input_pcap_online(input_devices[i], &error))) {
                capture_manager_add_input(manager, input);
            } else {
                g_printerr("error: %s", error->message);
                return 1;
            }
        }
    }

#ifdef USE_HEP
    // Hep settings
    if (hep_listen) {
        if ((input = capture_input_hep(hep_listen, &error))) {
            capture_manager_add_input(manager, input);
        } else {
            g_printerr("error: %s", error->message);
            return 1;
        }
    }

#endif

#if defined(WITH_GNUTLS) || defined(WITH_OPENSSL)
    if (keyfile) {
        // Check if we have a keyfile and is valid
        if (!tls_check_keyfile(keyfile, &error)) {
            g_printerr("error: %s\n", error->message);
            return 1;
        } else {
            // Set capture decrypt key file
            capture_manager_set_keyfile(manager, keyfile, &error);
        }
    }
#endif

    // Old legacy option to open pcaps without other arguments
    if (argc == 2 && g_file_test(argv[1], G_FILE_TEST_EXISTS)) {
        g_print("%s seems to be a file: Use -I flag or press any key to read its contents.\n", argv[1]);
        getchar();
        // Read as a PCAP file
        if ((input = capture_input_pcap_offline(argv[1], &error))) {
            capture_manager_add_input(manager, input);
            argc--;
        } else {
            g_printerr("error: %s\n", error->message);
            return 1;
        }
    }

    // If no capture file or device selected, use default capture device
    if (g_slist_length(manager->inputs) == 0) {
        gchar *default_device = (gchar *) setting_get_value(SETTING_CAPTURE_DEVICE);
        if ((input = capture_input_pcap_online(default_device, &error))) {
            capture_manager_add_input(manager, input);
        } else {
            g_printerr("error: %s\n", error->message);
            return 1;
        }
    }

    /***************************   Input Filtering  ***************************/
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
        if (!capture_manager_set_filter(manager, bpf_filter->str, NULL)) {
            // BPF Filter invalid, check including match_expr
            g_string_prepend(bpf_filter, match_expr);
            match_expr = NULL;

            // Check bpf filter is valid again
            if (!capture_manager_set_filter(manager, bpf_filter->str, &error)) {
                g_printerr("error: %s\n", error->message);
                return 1;
            }
        }

        // Set the payload match expression
        storage_mopts.mexpr = match_expr;
    }

    /***************************** Capture Outputs *****************************/
    // Handle capture file output
    if (output_file != NULL) {
        if ((output = capture_output_pcap(output_file, &error))) {
            capture_manager_add_output(manager, output);
        } else {
            g_printerr("error: %s\n", error->message);
            return 1;
        }
    }

#ifdef USE_HEP
    // Handle capture HEP output
    if (hep_send != NULL) {
        if ((output = capture_output_hep(hep_send, &error))) {
            capture_manager_add_output(manager, output);
        } else {
            g_printerr("error: %s\n", error->message);
            return 1;
        }
    }
#endif

    /*****************************  Main Logic  *****************************/
    // Initialize SIP Messages Storage
    if (!sip_init(storage_copts, storage_mopts, storage_sopts, &error)) {
        g_printerr("Failed to initialize storage: %s\n", error->message);
        return 1;
    };

    // Start capture threads
    capture_manager_start(manager);

    if (!no_interface) {
        // Initialize interface
        ncurses_init();
        // This is a blocking call.
        // Create the first panel and wait for user input
        ui_create_panel(PANEL_CALL_LIST);
        ui_wait_for_input();
    } else {
        setbuf(stdout, NULL);
        while(capture_is_running(manager)) {
            if (!quiet)
                g_print("\rDialog count: %d", sip_calls_count());
            g_usleep(500 * 1000);
        }
        if (!quiet)
            g_print("\rDialog count: %d\n", sip_calls_count());
    }

    // Capture deinit
    capture_manager_free(manager);

    // Deinitialize interface
    ncurses_deinit();

    // Deinitialize configuration options
    deinit_options();

    // Deallocate sip stored messages
    sip_deinit();

    // Leaving!
    return 0;
}
