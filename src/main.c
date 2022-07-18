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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <getopt.h>
#include "option.h"
#include "vector.h"
#include "capture.h"
#include "capture_eep.h"
#ifdef WITH_GNUTLS
#include "capture_gnutls.h"
#endif
#ifdef WITH_OPENSSL
#include "capture_openssl.h"
#endif
#include "curses/ui_manager.h"

/**
 * @brief Usage function
 *
 * Print all available options (which are none actually)
 */
void
usage()
{
    printf("Usage: %s [-hVcivNqrD] [-IO pcap_dump] [-d dev] [-l limit] [-B buffer]"
#if defined(WITH_GNUTLS) || defined(WITH_OPENSSL)
           " [-k keyfile]"
#endif
#ifdef USE_EEP
           " [-LHE capture_url]"
#endif
           " [<match expression>] [<bpf filter>]\n\n"
           "    -h --help\t\t This usage\n"
           "    -V --version\t Version information\n"
           "    -d --device\t\t Use this capture device instead of default\n"
           "    -I --input\t\t Read captured data from pcap file\n"
           "    -O --output\t\t Write captured data to pcap file\n"
           "    -B --buffer\t\t Set pcap buffer size in MB (default: 2)\n"
           "    -c --calls\t\t Only display dialogs starting with INVITE\n"
           "    -r --rtp\t\t Capture RTP packets payload\n"
           "    -l --limit\t\t Set capture limit to N dialogs\n"
           "    -i --icase\t\t Make <match expression> case insensitive\n"
           "    -v --invert\t\t Invert <match expression>\n"
           "    -N --no-interface\t Don't display sngrep interface, just capture\n"
           "    -q --quiet\t\t Don't print captured dialogs in no interface mode\n"
           "    -D --dump-config\t Print active configuration settings and exit\n"
           "    -f --config\t\t Read configuration from file\n"
           "    -F --no-config\t Do not read configuration from default config file\n"
           "    -R --rotate\t\t Rotate calls when capture limit have been reached\n"
#ifdef USE_EEP
           "    -H --eep-send\t Homer sipcapture url (udp:X.X.X.X:XXXX)\n"
           "    -L --eep-listen\t Listen for encapsulated packets (udp:X.X.X.X:XXXX)\n"
           "    -E --eep-parse\t Enable EEP parsing in captured packets\n"
#endif
#if defined(WITH_GNUTLS) || defined(WITH_OPENSSL)
           "    -k --keyfile\t RSA private keyfile to decrypt captured packets\n"
#endif
           "\n",PACKAGE);
}

void
version()
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
#ifdef WITH_PCRE
           " * Compiled with Perl Compatible regular expressions support.\n"
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
    int opt, idx, limit, only_calls, no_incomplete, pcap_buffer_size, i;
    const char *device, *outfile;
    char bpf[512];
#if defined(WITH_GNUTLS) || defined(WITH_OPENSSL)
    const char *keyfile;
#endif
    const char *match_expr;
    int match_insensitive = 0, match_invert = 0;
    int no_interface = 0, quiet = 0, rtp_capture = 0, rotate = 0, no_config = 0;
    vector_t *infiles = vector_create(0, 1);
    vector_t *indevices = vector_create(0, 1);
    char *token;

    // Program options
    static struct option long_options[] = {
        { "help", no_argument, 0, 'h' },
        { "version", no_argument, 0, 'V' },
        { "device", required_argument, 0, 'd' },
        { "input", required_argument, 0, 'I' },
        { "output", required_argument, 0, 'O' },
        { "buffer", required_argument, 0, 'B' },
#if defined(WITH_GNUTLS) || defined(WITH_OPENSSL)
        { "keyfile", required_argument, 0, 'k' },
#endif
        { "calls", no_argument, 0, 'c' },
        { "rtp", no_argument, 0, 'r' },
        { "limit", required_argument, 0, 'l' },
        { "icase", no_argument, 0, 'i' },
        { "invert", no_argument, 0, 'v' },
        { "no-interface", no_argument, 0, 'N' },
        { "dump-config", no_argument, 0, 'D' },
        { "rotate", no_argument, 0, 'R' },
        { "config", required_argument, 0, 'f' },
        { "no-config", no_argument, 0, 'F' },
#ifdef USE_EEP
        { "eep-listen", required_argument, 0, 'L' },
        { "eep-send", required_argument, 0, 'H' },
        { "eep-parse", required_argument, 0, 'E' },
#endif
        { "quiet", no_argument, 0, 'q' },
    };

    // Parse command line arguments that have high priority
    opterr = 0;
    char *options = "hVd:I:O:B:pqtW:k:crl:ivNqDL:H:ERf:F";
    while ((opt = getopt_long(argc, argv, options, long_options, &idx)) != -1) {
        switch (opt) {
            case 'h':
                usage();
                return 0;
            case 'V':
                version();
                return 0;
            case 'F':
                no_config = 1;
                break;
            default:
                break;
        }
	}

    // Initialize configuration options
    init_options(no_config);

    // Get initial values for configurable arguments
    device = setting_get_value(SETTING_CAPTURE_DEVICE);
    outfile = setting_get_value(SETTING_CAPTURE_OUTFILE);
    pcap_buffer_size = setting_get_intvalue(SETTING_CAPTURE_BUFFER);
#if defined(WITH_GNUTLS) || defined(WITH_OPENSSL)
    keyfile = setting_get_value(SETTING_CAPTURE_KEYFILE);
#endif
    limit = setting_get_intvalue(SETTING_CAPTURE_LIMIT);
    only_calls = setting_enabled(SETTING_SIP_CALLS);
    no_incomplete = setting_enabled(SETTING_SIP_NOINCOMPLETE);
    rtp_capture = setting_enabled(SETTING_CAPTURE_RTP);
    rotate = setting_enabled(SETTING_CAPTURE_ROTATE);

    // Parse the rest of command line arguments
    opterr = 0;
    optind = 1;  /* reset getopt index */
    while ((opt = getopt_long(argc, argv, options, long_options, &idx)) != -1) {
        switch (opt) {
            case 'h': /* handled before with higher priority options */
                break;
            case 'V': /* handled before with higher priority options */
                break;
            case 'd':
                token = strtok(optarg, ",");
                while (token) {
                    vector_append(indevices, token);
                    token = strtok(NULL, ",");
                }
                break;
            case 'I':
                vector_append(infiles, optarg);
                break;
            case 'O':
                outfile = optarg;
                break;
            case 'B':
                if(!(pcap_buffer_size = atoi(optarg))) {
                    fprintf(stderr, "Invalid buffer size.\n");
                    return 0;
                }
                if(!(pcap_buffer_size > 0 && pcap_buffer_size <= 2048)) {
                    fprintf(stderr, "Buffer size not in range (0 < b <= 2048).\n");
                    return 0;
                }
                break;
            case 'l':
                if(!(limit = atoi(optarg))) {
                    fprintf(stderr, "Invalid limit value.\n");
                    return 0;
                }
                break;
            case 'k':
#if defined(WITH_GNUTLS) || defined(WITH_OPENSSL)
                keyfile = optarg;
                break;
#else
                fprintf(stderr, "sngrep is not compiled with SSL support.");
                exit(1);
#endif
            case 'c':
                only_calls = 1;
                setting_set_value(SETTING_SIP_CALLS, SETTING_ON);
                break;
            case 'r':
                rtp_capture = 1;
                setting_set_value(SETTING_CAPTURE_RTP, SETTING_ON);
                break;
            case 'i':
                match_insensitive++;
                break;
            case 'v':
                match_invert++;
                break;
            case 'N':
                no_interface = 1;
                setting_set_value(SETTING_CAPTURE_STORAGE, "none");
                break;
            case 'q':
                quiet = 1;
                break;
            case 'D':
                key_bindings_dump();
                settings_dump();
                return 0;
            case 'f':
                read_options(optarg);
                break;
            case 'F':  /* handled before with higher priority options */
                break;
            case 'R':
                rotate = 1;
                setting_set_value(SETTING_CAPTURE_ROTATE, SETTING_ON);
                break;
                // Dark options for dummy ones
            case 'p':
            case 't':
            case 'W':
                break;
            case 'L':
#ifdef USE_EEP
                capture_eep_set_server_url(optarg);
                break;
#else
                fprintf(stderr, "sngrep is not compiled with HEP/EEP support.");
                exit(1);
#endif
            case 'H':
#ifdef USE_EEP
                capture_eep_set_client_url(optarg);
                break;
#else
                fprintf(stderr, "sngrep is not compiled with HEP/EEP support.");
                exit(1);
#endif
            case 'E':
#ifdef USE_EEP
                setting_set_value(SETTING_CAPTURE_EEP, SETTING_ON);
                break;
#else
                fprintf(stderr, "sngrep is not compiled with HEP/EEP support.");
                exit(1);
#endif
            case '?':
                if (strchr(options, optopt)) {
                    fprintf(stderr, "-%c option requires an argument.\n", optopt);
                } else if (isprint(optopt)) {
                    fprintf(stderr, "Unknown option -%c.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
                }
                return 1;
            default:
                break;
        }
    }

    setup_sigterm_handler();

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

    // Initialize SIP Messages Storage
    sip_init(limit, only_calls, no_incomplete);

    // Set capture options
    capture_init(limit, rtp_capture, rotate, pcap_buffer_size);

#ifdef USE_EEP
    // Initialize EEP if enabled
    capture_eep_init();
#endif

    // If no device or files has been specified in command line, use default
    if (capture_sources_count() == 0 &&
        vector_count(indevices) == 0 && vector_count(infiles) == 0) {
        token = strdup(device);
        token = strtok(token, ",");
        while (token) {
            vector_append(indevices, token);
            token = strtok(NULL, ",");
        }
        sng_free(token);
    }

    // If we have an input file, load it
    for (i = 0; i < vector_count(infiles); i++) {
        // Try to load file
        if (capture_offline(vector_item(infiles, i)) != 0)
            return 1;
    }

    // If we have an input device, load it
    for (i = 0; i < vector_count(indevices); i++) {
        // Check if all capture data is valid
        if (capture_online(vector_item(indevices, i)) != 0)
            return 1;
    }

    if (outfile)
    {
        ino_t dump_inode;
        pcap_dumper_t *dumper = dump_open(outfile, &dump_inode);
        capture_set_dumper(dumper, dump_inode);
    }

    // Remove Input files vector
    vector_destroy(infiles);

    // More arguments pending!
    if (argv[optind]) {
        // Assume first pending argument is  match expression
        match_expr = argv[optind++];

        // Try to build the bpf filter string with the rest
        memset(bpf, 0, sizeof(bpf));
        for (i = optind; i < argc; i++)
            sprintf(bpf + strlen(bpf), "%s ", argv[i]);

        // Check if this BPF filter is valid
        if (capture_set_bpf_filter(bpf) != 0) {
            // BPF Filter invalid, check incluiding match_expr
            match_expr = 0;    // There is no need to parse all payload at this point


            // Build the bpf filter string
            memset(bpf, 0, sizeof(bpf));
            for (i = optind - 1; i < argc; i++)
                sprintf(bpf + strlen(bpf), "%s ", argv[i]);

            // Check bpf filter is valid again
            if (capture_set_bpf_filter(bpf) != 0) {
                fprintf(stderr, "Couldn't install filter %s: %s\n", bpf, capture_last_error());
                return 1;
            }
        }

        // Set the capture filter
        if (match_expr)
            if (sip_set_match_expression(match_expr, match_insensitive, match_invert)) {
                fprintf(stderr, "Unable to parse expression %s\n", match_expr);
                return 1;
            }
    }

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
        while(capture_is_running() && !was_sigterm_received()) {
            if (!quiet)
                printf("\rDialog count: %d", sip_calls_count_unrotated());
            usleep(500 * 1000);
        }
        if (!quiet)
            printf("\rDialog count: %d\n", sip_calls_count_unrotated());
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
