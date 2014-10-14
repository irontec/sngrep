/**************************************************************************
 **
 ** sngrep - SIP callflow viewer using ngrep
 **
 ** Copyright (C) 2013 Ivan Alonso (Kaian)
 ** Copyright (C) 2013 Irontec SL. All rights reserved.
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
 *
 * @todo This should be coded properly. We could use use -f flag argument to
 * load the pcap file (because ngrep has no -f flag) and assume any other
 * argument are ngrep arguments. Anyway, actual main code is awful.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "config.h"
#include "option.h"
#include "ui_manager.h"
#include "capture.h"

/**
 * @brief Usage function
 *
 * Print all available options (which are none actually)
 */
void
usage()
{
    printf("Usage: %s <-IO pcap_dump> <-d dev> [<bpf filter>|<pcap_dump>]\n\n"
        "    -h  This usage\n"
        "    -v  Version information\n"
        "    -I  Read captured data from pcap file\n"
        "    -O  Write captured data to pcap file\n\n",
        PACKAGE);
}

void
version()
{
    printf("%s - %s\n"
        "Copyright (C) 2013,124 Irontec S.L.\n"
        "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\n\n"
        "Written by Ivan Alonso [aka Kaian]\n", PACKAGE, VERSION);
}

/**
 * @brief Main function logic
 *
 * Parse command line options and start running threads
 *
 * @note There are no params actually... if you supply one
 *    param, I will assume we are running offline mode with
 *    a pcap file. Otherwise the args will be passed to ngrep
 *    without any type of validation.
 *
 */
int
main(int argc, char* argv[])
{

    int ret = 0, opt;
    const char *infile;

    //! BPF arguments filter
    char bpf[512];

    //! ngrep thread attributes
    pthread_attr_t attr;
    //! ngrep running thread
    pthread_t exec_t;

    // We need parameters!
    if (argc == 1) {
        usage();
        return 0;
    }

    // Initialize configuration options
    init_options();

    // Parse command line arguments
    opterr = 0;
    char *options = "hvd:I:O:";
    while ((opt = getopt (argc, argv, options)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            return 0;
        case 'v':
            version();
            return 0;
        case 'd':
            set_option_value("capture.device", optarg);
            break;
        case 'I':
            set_option_value("capture.infile", optarg);
            break;
        case 'O':
            set_option_value("capture.outfile", optarg);
            break;
        case '?':
            if (strchr(options, optopt)) {
                fprintf(stderr, "-%c option requires an argument.\n", optopt);
            } else if (isprint(optopt)) {
                fprintf(stderr, "Unknown option -%c.\n", optopt);
            } else {
                fprintf(stderr, "Unknown option character '\\x%x'.\n",optopt);
            }
            return 1;
        default:
            break;
        }
    }

    // Check if given argument is a file
    if (argc == 2 && (access(argv[1], F_OK) == 0)) {
        // Show offline mode in ui
        set_option_value("capture.infile", argv[1]);
    } else {
        // Build the bpf filter string
        memset(bpf, 0, sizeof(bpf));
        for (; optind < argc; optind++) {
            sprintf(bpf + strlen(bpf), "%s ", argv[optind]);
        }
        // Set the capture filter
        set_option_value("capture.filter", bpf);
    }

    // If we have an input file, load it
    if ((infile = get_option_value("capture.infile"))) {
        // Set mode to display on interface
        set_option_value("sngrep.mode", "Offline");
        // Try to load file
        if (capture_offline() != 0)
            return 1;
    } else {
        // Show online mode on interface
        set_option_value("sngrep.mode", "Online");

        // Check if all capture data is valid
        if (capture_online() != 0)
            return 1;

        // Launch capture thread
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&exec_t, &attr, (void *) capture_thread, NULL)) {
            fprintf(stderr, "Unable to create Capture Thread!\n");
            return 1;
        }
        pthread_attr_destroy(&attr);
    }

    // Initialize interface
    // This is a blocking call. Interface have user action loops.
    init_interface();

    // Leaving!
    return ret;
}
