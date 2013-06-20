/**************************************************************************
 **
 **  sngrep - Ncurses ngrep interface for SIP
 **
 **   Copyright (C) 2013 Ivan Alonso (Kaian)
 **   Copyright (C) 2013 Irontec SL. All rights reserved.
 **
 **   This program is free software: you can redistribute it and/or modify
 **   it under the terms of the GNU General Public License as published by
 **   the Free Software Foundation, either version 3 of the License, or
 **   (at your option) any later version.
 **
 **   This program is distributed in the hope that it will be useful,
 **   but WITHOUT ANY WARRANTY; without even the implied warranty of
 **   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **   GNU General Public License for more details.
 **
 **   You should have received a copy of the GNU General Public License
 **   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 ****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "exec.h"
#include "curses.h"
#include "pcap.h"

/**
 * Usage function
 * Print all available options (which are none actually)
 *
 */
void usage(const char* progname)
{
    fprintf(stdout, "[%s] Copyright (C) 2013 Irontec S.L.\n\n", progname);
    fprintf(stdout, "Usage:\n");
    fprintf(stdout, "\t%s <file.pcap>\n", progname);
    fprintf(stdout, "\t%s <ngrep options>\n", progname);
    fprintf(stdout, "\tsee 'man ngrep' for available ngrep options\n\n");
    fprintf(stdout, "Note: some ngrep options are forced by %s\n", progname);
}

/**
 * Main function 
 * Parse command line options and start running threads
 *
 * @note There are no params actually... if you supply one 
 *    param, I will assume we are running offline mode with
 *    a pcap file. Otherwise the args will be passed to ngrep
 *    without any type of validation.
 *
 */
int main(int argc, char* argv[])
{

    int ret;
    struct ui_config config; /* ui configuration */
    pthread_attr_t attr; /* ngrep thread attributes */
    pthread_t exec_t; /* ngrep running thread */

    // Parse arguments.. I mean..
    if (argc < 2) {
        // No arguments!
        usage(argv[0]);
        return 1;
    } else if (argc == 2) {
        // Show ofline mode in ui
        config.online = 0;
        config.fname = argv[1];

        // Assume Offline mode with pcap file
        if (load_pcap_file(argv[1]) != 0) {
            fprintf(stderr, "Error loading data from pcap file %s\n", argv[1]);
            return 1;
        }
    } else {
        // Show online mode in ui
        config.online = 1;

        // Assume online mode, launch ngrep in a thread
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&exec_t, &attr, (void *) run_ngrep, argv)) {
            fprintf(stderr, "Unable to create Exec Thread!\n");
            return 1;
        }
        pthread_attr_destroy(&attr);
    }

    // Initialize interface
    // This is a blocking call. Interface have user action loops.
    ret = init_interface(config);
    if (ret == 127) {
        fprintf(stderr, "%s requires at least %d columns in terminal :(\n", argv[0], UI_MIN_COLS);
    }

    // Leaving!
    return ret;
}

