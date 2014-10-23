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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "config.h"
#include "option.h"
#include "ui_manager.h"
#include "capture.h"
#include "capture_tls.h"

/**
 * @brief Usage function
 *
 * Print all available options (which are none actually)
 */
void
usage()
{
    printf("Usage: %s [-IO pcap_dump] [-d dev] [-k keyfile] [<bpf filter>|<pcap_dump>]\n\n"
        "    -h  This usage\n"
        "    -v  Version information\n"
        "    -d  Use this capture device insted of default\n"
        "    -I  Read captured data from pcap file\n"
        "    -O  Write captured data to pcap file\n"
        "    -k  SSL keyfile to decrypt captured packets\n",
        PACKAGE);
}

void
version()
{
    printf("%s - %s\n"
        "Copyright (C) 2013,2014,2014 Irontec S.L.\n"
        "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\n\n"
        "Written by Ivan Alonso [aka Kaian]\n", PACKAGE, VERSION);
}

/**
 * @brief Main function logic
 *
 * Parse command line options and start running threads
 */
int
main(int argc, char* argv[])
{

    int ret = 0, opt;
    const char *infile;
    const char *keyfile;

    //! BPF arguments filter
    char bpf[512];

    //! capture thread attributes
    pthread_attr_t attr;
    //! capture thread
    pthread_t exec_t;

    // Initialize configuration options
    init_options();

    // Parse command line arguments
    opterr = 0;
    char *options = "hvd:I:O:pqtW:k:";
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
        case 'k':
            set_option_value("capture.keyfile", optarg);
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
                fprintf(stderr, "Unknown option character '\\x%x'.\n",optopt);
            }
            return 1;
        default:
            break;
        }
    }

    // Check if we have a keyfile and is valid
    if ((keyfile = get_option_value("capture.keyfile")) &&
        !tls_check_keyfile(keyfile)) {
        fprintf(stderr, "%s does not contain a valid RSA private key.\n", keyfile);
        return 1;
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

    // Close pcap handler
    capture_close();

    // Leaving!
    return ret;
}
