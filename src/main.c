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
 * @file main.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of initial functions used by sngrep
 */

#include "config.h"
#include <malloc.h>
#include <glib.h>
#include "glib-extra/glib.h"
#include "setting.h"
#include "capture/capture.h"
#include "tui/tui.h"
#include "tui/dialog.h"
#ifdef USE_HEP
#include "capture/capture_hep.h"
#endif
#ifdef WITH_SSL
#include "packet/packet_tls.h"
#endif

void
print_version_info()
{
    g_print("%s - %s\n"
            "Copyright (C) 2013-2019 Irontec S.L.\n"
            "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n"
            "This is free software: you are free to change and redistribute it.\n"
            "There is NO WARRANTY, to the extent permitted by law.\n"

            #ifdef WITH_SSL
            " * Compiled with GnuTLS support\n"
            #endif
            #ifdef USE_IPV6
            " * Compiled with IPv6 support\n"
            #endif
            #ifdef USE_HEP
            " * Compiled with HEPv3 support\n"
            #endif
            #ifdef WITH_G729
            " * Compiled with G.729 support\n"
            #endif
            #ifdef WITH_SND
            " * Compiled with libsnd support\n"
            #endif
            #ifdef WITH_PULSE
            " * Compiled with pulseaudio support\n"
            #endif
            "\nWritten by Ivan Alonso [aka Kaian]\n",
            PACKAGE, VERSION);
}

static gboolean
print_storage_count(GMainLoop *loop)
{
    setbuf(stdout, NULL);

    g_print(
        "\rProgress: %d%%\tDialog count: %d",
        capture_manager_load_progress(capture_manager_get_instance()),
        storage_calls_count()
    );

    if (capture_is_running() == FALSE &&
        storage_pending_packets() == 0) {
        g_print("\r\n");
        g_main_loop_quit(loop);
    }
    return TRUE;
}

/**
 * @brief Main function logic
 *
 * Parse command line options and start running threads
 */
int
main(int argc, char *argv[])
{
    GError *error = NULL;
    gchar **input_files = NULL;
    gchar **input_devices = NULL;
#ifdef USE_HEP
    gchar *hep_listen = NULL;
    gchar *hep_send = NULL;
#endif
    gboolean no_interface = FALSE;
    gboolean quiet = FALSE;
    gboolean version = FALSE;
    gboolean config_dump = FALSE;
    gchar *memory_limit = NULL;
#ifdef WITH_SSL
    gchar *keyfile = NULL;
#endif
    StorageOpts storage_opts;
    SettingOpts setting_opts;
    CaptureInput *input;
    CaptureOutput *output;

    /****************************** Initialization ****************************/
    memset(&storage_opts, 0, sizeof(StorageOpts));
    memset(&setting_opts, 0, sizeof(SettingOpts));

    /************************ Command Line Arguments **************************/
    GOptionEntry main_entries[] = {
        { "version", 'V', 0, G_OPTION_ARG_NONE, &version,
          "Version information", NULL },
        { "device", 'd', 0, G_OPTION_ARG_STRING_ARRAY, &input_devices,
          "Use this capture device instead of default", "DEVICE" },
        { "input", 'I', 0, G_OPTION_ARG_FILENAME_ARRAY, &input_files,
          "Read captured data from pcap file", "FILE" },
        { "output", 'O', 0, G_OPTION_ARG_FILENAME, &storage_opts.capture.outfile,
          "Write captured data to pcap file", "FILE" },
        { "calls", 'c', 0, G_OPTION_ARG_NONE, &storage_opts.match.invite,
          "Only display dialogs starting with INVITE", NULL },
        { "rtp", 'r', 0, G_OPTION_ARG_NONE, &storage_opts.capture.rtp,
          "Store captured RTP packets (enables saving RTP)", NULL },
        { "memory-limit", 'm', 0, G_OPTION_ARG_STRING, &memory_limit,
          "Set memory limit in bytes (0 to disable)", "N" },
        { "limit", 'l', 0, G_OPTION_ARG_INT, &storage_opts.capture.limit,
          "Set capture limit to N dialogs", "N" },
        { "rotate", 'R', 0, G_OPTION_ARG_NONE, &storage_opts.capture.rotate,
          "Rotate calls when capture limit have been reached", NULL },
        { "no-interface", 'N', 0, G_OPTION_ARG_NONE, &no_interface,
          "Don't display sngrep interface, just capture", NULL },
        { "quiet", 'q', 0, G_OPTION_ARG_NONE, &quiet,
          "Don't print captured dialogs in no interface mode", NULL },
        { "icase", 'i', 0, G_OPTION_ARG_NONE, &storage_opts.match.micase,
          "Make <match expression> case insensitive", NULL },
        { "invert", 'v', 0, G_OPTION_ARG_NONE, &storage_opts.match.minvert,
          "Invert <match expression>", NULL },
        { "dump-config", 'D', 0, G_OPTION_ARG_NONE, &config_dump,
          "Print active configuration settings and exit", NULL },
        { "config", 'f', 0, G_OPTION_ARG_FILENAME, &setting_opts.file,
          "Read configuration from FILE", "FILE" },
        { "no-config", 'F', 0, G_OPTION_ARG_NONE, &setting_opts.use_defaults,
          "Do not read configuration from default config file", NULL },
#ifdef WITH_SSL
        { "keyfile", 'k', 0, G_OPTION_ARG_FILENAME, &keyfile,
          "RSA private keyfile to decrypt captured packets", "KEYFILE" },
#endif
#ifdef USE_HEP
        { "hep-listen", 'L', 0, G_OPTION_ARG_STRING, &hep_listen,
          "Listen for encapsulated HEP packets", "udp:X.X.X.X:XXXX" },
        { "hep-send", 'H', 0, G_OPTION_ARG_STRING, &hep_send,
          "Homer sipcapture URL", "udp:X.X.X.X:XXXX" },
#endif
        { G_OPTION_SENTINEL }
    };

    /************************** Command Line Parsing **************************/
    GOptionContext *context = g_option_context_new("[<match expression>] [<bpf filter>]");
    g_option_context_add_main_entries(context, main_entries, NULL);
    g_option_context_parse(context, &argc, &argv, &error);
    g_option_context_free(context);

    if (error != NULL) {
        g_printerr("Options parsing failed: %s\n", error->message);
        return 1;
    }

    /***************************** Configuration *****************************/
    // Initialize core attributes
    attribute_init();

    // Initialize configuration options
    settings_init(setting_opts);

    // Get initial values for configurable arguments
    if (!storage_opts.match.invite)
        storage_opts.match.invite = setting_enabled(SETTING_STORAGE_CALLS);
    if (!storage_opts.match.complete)
        storage_opts.match.complete = setting_enabled(SETTING_STORAGE_COMPLETE_DLG);
    if (!storage_opts.capture.limit)
        storage_opts.capture.limit = (guint) setting_get_intvalue(SETTING_CAPTURE_LIMIT);
    if (!storage_opts.capture.rtp)
        storage_opts.capture.rtp = setting_enabled(SETTING_STORAGE_RTP);
    if (!storage_opts.capture.rotate)
        storage_opts.capture.rotate = setting_enabled(SETTING_STORAGE_ROTATE);
    if (!storage_opts.capture.outfile)
        storage_opts.capture.outfile = g_strdup(setting_get_value(SETTING_CAPTURE_PCAP_OUTFILE));
    if (!storage_opts.capture.mode) {
        storage_opts.capture.mode = setting_get_enum(SETTING_STORAGE_MODE);
    }

    // Parse memory limit size string
    if (memory_limit == NULL) {
        storage_opts.capture.memory_limit = g_format_size_to_bytes(setting_get_value(SETTING_STORAGE_MEMORY_LIMIT));
    } else {
        storage_opts.capture.memory_limit = g_format_size_to_bytes(memory_limit);
    }

    // Disable frame storage if no interface is displayed
    if (no_interface) {
        storage_opts.capture.mode = STORAGE_MODE_NONE;
    }

#ifdef WITH_SSL
    if (keyfile == NULL) {
        keyfile = g_strdup(setting_get_value(SETTING_PACKET_TLS_KEYFILE));
    }
    if (keyfile != NULL) {
        // Automatically enable TCP and TLS protocols when using private key
        setting_set_value(SETTING_PACKET_TCP, SETTING_ON);
        setting_set_value(SETTING_PACKET_TLS, SETTING_ON);
    }
#endif

    // Parse command line arguments that have high priority
    if (version) {
        print_version_info();
        return 0;
    } else if (config_dump) {
        settings_dump();
        attribute_dump();
        key_bindings_dump();
        return 0;
    }

    /***************************** Capture Inputs *****************************/
    // Main packet capture manager
    CaptureManager *capture = capture_manager_new();

    // Handle capture input from files
    g_autoptr(GList) files = NULL;
    for (guint i = 0; input_files && i < g_strv_length(input_files); i++) {
        if (g_file_test(input_files[i], G_FILE_TEST_EXISTS)) {
            // An actual file, just append to the list
            files = g_list_append(files, input_files[i]);
            continue;
        }

        // Handle input files glob matching
        gchar *dirname = g_path_get_dirname(input_files[i]);
        gchar *glob = g_path_get_basename(input_files[i]);

        // Support home dir shell shortcuts
        if (*dirname == '~') {
            dirname = g_strdup_printf("%s/%s", g_get_home_dir(), dirname + 1);
        }

        // Open directory for reading files
        GDir *dir = g_dir_open(dirname, 0, &error);
        if (error != NULL) {
            g_printerr("error: %s\n", error->message);
            return 1;
        }

        // Check each file against given pattern
        const gchar *fname = NULL;
        GPatternSpec *match = g_pattern_spec_new(glob);
        while ((fname = g_dir_read_name(dir)) != NULL) {
            if (g_pattern_match_string(match, fname)) {
                files = g_list_append(files, g_strdup_printf(
                    "%s%c%s",
                    dirname, G_DIR_SEPARATOR, fname));
            }
        }

        if (files == NULL) {
            g_printerr("error: no file matched expression %s\n", glob);
            return 1;
        }

        g_free(dirname);
        g_free(glob);
        g_pattern_spec_free(match);
    }

    // Old legacy option to open pcaps without other arguments
    if (argc == 2 && g_file_test(argv[1], G_FILE_TEST_EXISTS)) {
        // Read as a PCAP file
        files = g_list_append(files, argv[1]);
        argc--;
    }

    // Handle capture file inputs
    for (GList *l = files; l != NULL; l = l->next) {
        if ((input = capture_input_pcap_offline(l->data, &error))) {
            capture_manager_add_input(capture, input);
        } else {
            g_printerr("error: %s\n", error->message);
            return 1;
        }
    }

    // Handle capture device inputs
    for (guint i = 0; input_devices && i < g_strv_length(input_devices); i++) {
        if ((input = capture_input_pcap_online(input_devices[i], &error))) {
            capture_manager_add_input(capture, input);
        } else {
            g_printerr("error: %s\n", error->message);
            return 1;
        }
    }

#ifdef USE_HEP
    // Hep settings
    if (hep_listen) {
        // Enable HEP packet
        setting_set_value(SETTING_PACKET_HEP, SETTING_ON);

        // Create a HEP server input
        if ((input = capture_input_hep(hep_listen, &error))) {
            capture_manager_add_input(capture, input);
        } else {
            g_printerr("error: %s\n", error->message);
            return 1;
        }
    }
#endif

#ifdef WITH_SSL
    if (keyfile) {
        // Check if we have a keyfile and is valid
        if (!packet_tls_privkey_check(keyfile, &error)) {
            g_printerr("error: %s\n", error->message);
            return 1;
        } else {
            // Set capture decrypt key file
            capture_manager_set_keyfile(capture, keyfile, &error);
        }
    }
#endif

    // If no capture file or device selected, use default capture device
    if (g_slist_length(capture->inputs) == 0) {
        gchar *default_device = (gchar *) setting_get_value(SETTING_CAPTURE_PCAP_DEVICE);
        if ((input = capture_input_pcap_online(default_device, &error))) {
            capture_manager_add_input(capture, input);
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
        g_autoptr(GString) bpf_filter = g_string_new(NULL);
        while (argv[argi]) {
            g_string_append_printf(bpf_filter, " %s", argv[argi]);
            argi++;
        }

        // Check if this BPF filter is valid
        if (!capture_manager_set_filter(capture, bpf_filter->str, NULL)) {
            // BPF Filter invalid, check including match_expr
            g_string_prepend(bpf_filter, match_expr);
            match_expr = NULL;

            // Check bpf filter is valid again
            if (!capture_manager_set_filter(capture, bpf_filter->str, &error)) {
                g_printerr("error: %s\n", error->message);
                return 1;
            }
        }

        // Set the payload match expression
        storage_opts.match.mexpr = match_expr;
    }

    /***************************** Capture Outputs *****************************/
    // Handle capture file output
    if (storage_opts.capture.outfile != NULL) {
        if ((output = capture_output_pcap(storage_opts.capture.outfile, &error))) {
            capture_manager_add_output(capture, output);
        } else {
            g_printerr("error: %s\n", error->message);
            return 1;
        }
    }

#ifdef USE_HEP
    // Handle capture HEP output
    if (hep_send != NULL) {
        if ((output = capture_output_hep(hep_send, &error))) {
            capture_manager_add_output(capture, output);
        } else {
            g_printerr("error: %s\n", error->message);
            return 1;
        }
    }
#endif

    /*****************************  Main Logic  *****************************/
    // Create main loop for default context
    GMainLoop *main_loop = g_main_loop_new(NULL, FALSE);

    // Initialize SIP Messages Storage
    Storage *storage = storage_new(storage_opts, &error);
    if (error != NULL) {
        g_printerr("Failed to initialize storage: %s\n", error->message);
        return 1;
    }

    // Start capture threads
    capture_manager_start(capture);

    // Check interface mode
    if (!no_interface) {
        // Initialize interface
        if (!tui_init(main_loop, &error)) {
            g_printerr("error: %s\n", error->message);
            return 1;
        }
    } else {
        if (!quiet) {
            g_timeout_add(500, (GSourceFunc) print_storage_count, main_loop);
        }
    }

    /************************* Application Main Loop  *************************/
    g_main_loop_run(main_loop);

    // Capture stop
    capture_manager_stop(capture);

    // Deallocate sip stored messages
    storage_free(storage);

    // Capture deinit
    capture_manager_free(capture);

    // Deinitialize interface
    tui_deinit();

    // Deinitialize configuration options
    settings_deinit();

    // Leaving!
    return 0;
}
