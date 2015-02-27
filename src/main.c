/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013,2014 Ivan Alonso (Kaian)
 ** Copyright (C) 2013,2014 Irontec SL. All rights reserved.
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
#include "ui_manager.h"
#include "capture.h"
#ifdef WITH_OPENSSL
#include "capture_tls.h"
#endif

/**
 * @brief Usage function
 *
 * Print all available options (which are none actually)
 */
void
usage()
{
    printf("Usage: %s [-c][-IO pcap_dump] [-d dev] [-l limit]"
#ifdef WITH_OPENSSL
           " [-k keyfile]"
#endif
           " [<bpf filter>]\n\n"
           "    -h --help\t\t This usage\n"
           "    -v --version\t Version information\n"
           "    -d --device\t\t Use this capture device instead of default\n"
           "    -I --input\t\t Read captured data from pcap file\n"
           "    -O --output\t\t Write captured data to pcap file\n"
           "    -c --calls\t\t Only display dialogs starting with INVITE\n"
           "    -l --limit\t\t Set capture limit to N dialogs\n"
#ifdef WITH_OPENSSL
           "    -k  RSA private keyfile to decrypt captured packets\n"
#endif
           "\n",PACKAGE);
}

void
version()
{
    printf("%s - %s\n"
           "Copyright (C) 2013-2015 Irontec S.L.\n"
           "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n"
           "This is free software: you are free to change and redistribute it.\n"
           "There is NO WARRANTY, to the extent permitted by law.\n"

#ifdef WITH_OPENSSL
           " * Compiled with OpenSSL support.\n"
#endif
#ifdef WITH_UNICODE
           " * Compiled with Wide-character support.\n"
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
    int opt, idx, limit;
    const char *device, *infile, *outfile;
    char bpf[512];
    const char *keyfile;

    // Program otptions
    static struct option long_options[] = {
        { "help", no_argument, 0, 'h' },
        { "version", no_argument, 0, 'v' },
        { "device", required_argument, 0, 'd' },
        { "input", required_argument, 0, 'I' },
        { "output", required_argument, 0, 'O' },
        { "keyfile", required_argument, 0, 'k' },
        { "calls", no_argument, 0, 'c' },
        { "limit", no_argument, 0, 'l' },
    };

    // Initialize configuration options
    init_options();

    // Get initial values for configurable arguments
    device = get_option_value("capture.device");
    infile = get_option_value("capture.infile");
    outfile = get_option_value("capture.outfile");
    keyfile = get_option_value("capture.keyfile");
    limit = get_option_int_value("capture.limit");

    // Parse command line arguments
    opterr = 0;
    char *options = "hvd:I:O:pqtW:k:cl:";
    while ((opt = getopt_long(argc, argv, options, long_options, &idx)) != -1) {
        switch (opt) {
            case 'h':
                usage();
                return 0;
            case 'v':
                version();
                return 0;
            case 'd':
                device = optarg;
                break;
            case 'I':
                infile = optarg;
                break;
            case 'O':
                outfile = optarg;
                break;
            case 'l':
                if(!(limit = atoi(optarg))) {
                    fprintf(stderr, "Invalid limit value.\n");
                    return 0;
                }
                break;
            case 'k':
                keyfile = optarg;
                break;
            case 'c':
                set_option_value("sip.calls", "on");
                break;
            // Dark options for dummy ones
            case 'p':
            case 'q':
            case 't':
            case 'W':
                break;
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

#ifdef WITH_OPENSSL
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
    } else {
        // Build the bpf filter string
        memset(bpf, 0, sizeof(bpf));
        for (; optind < argc; optind++) {
            sprintf(bpf + strlen(bpf), "%s ", argv[optind]);
        }
    }

    // Initialize SIP Messages Storage
    sip_init(limit);

    // If we have an input file, load it
    if (infile) {
        // Try to load file
        if (capture_offline(infile, bpf, limit) != 0)
            return 1;
    } else {
        // Check if all capture data is valid
        if (capture_online(device, bpf, outfile, limit) != 0)
            return 1;
    }

    // Initialize interface
    init_interface();

    // Start a capture thread for Online mode
    if (capture_is_online()) {
        if (capture_launch_thread() != 0) {
            deinit_interface();
            fprintf(stderr, "Failed to launch capture thread.\n");
            return 1;
        }
    }

    // This is a blocking call.
    // Create the first panel and wait for user input
    wait_for_input(ui_create(ui_find_by_type(PANEL_CALL_LIST)));

    // Close pcap handler
    capture_close();

    // Deinitialize interface
    deinit_interface();

    // Deinitialize configuration options
    deinit_options();

    // Deallocate sip stored messages
    sip_calls_clear();

    // Leaving!
    return 0;
}
