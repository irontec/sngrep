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
#ifdef WITH_NGREP
/**
 * @file exec.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in exec.h
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "option.h"
#include "ui_manager.h"

//! Forced stdbuf command line arguments
#define STDBUF_ARGS "-i0 -o0 -e0"
//! Forced ngrep command line arguments
#define NGREP_ARGS  "-qpt -W byline"

/****************************************************************************
 ** Current version of sngrep launches a thread that execs original ngrep
 ** binary. sngrep was born with the idea of parsing ngrep output.
 ** This could be changed with a bit of effort to a network capturing thread
 ** using libpcap functions, but we'll keep this way for now.
 **
 ** Also, take into account that as a parser, we expect ngrep header in an
 ** expecific format that is obtained using ngrep arguments -qpt which are
 ** forced by the exec process.
 **
 ** U DD/MM/YY hh:mm:ss.uuuuuu fff.fff.fff.fff:pppp -> fff.fff.fff.fff:pppp
 **
 ** If any other parameters are supplied to sngrep that changes this header
 ** (let's say -T), sngrep will fail at parsing any header :(
 **
 ****************************************************************************/
int
online_capture(void *pargv)
{
    char **argv = (char**) pargv;
    int argc = 1;
    char cmdline[512];
    FILE *fp;
    char stdout_line[2048] = "";
    char msg_header[256], msg_payload[20480];

    // Build the commald line to execute ngrep
    memset(cmdline, 0, sizeof(cmdline));
    sprintf(cmdline, "%s %s %s %s", STDBUF_BIN, STDBUF_ARGS, NGREP_BIN, NGREP_ARGS);
    // Add save to temporal file (if option enabled)
    if (!is_option_disabled("sngrep.tmpfile"))
        sprintf(cmdline + strlen(cmdline), " -O %s", get_option_value("sngrep.tmpfile"));

    while (argv[argc]){
        if (strchr(argv[argc], ' ')) {
            sprintf(cmdline + strlen(cmdline), " \"%s\"", argv[argc++]);
        } else {
            sprintf(cmdline + strlen(cmdline), " %s", argv[argc++]);
        }
    }

    // Open the command for reading.
    fp = popen(cmdline, "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        return 1;
    }

    // Get capture limit value
    int limit = get_option_int_value("capture.limit");

    // Read the output a line at a time - output it.
    while (fgets(stdout_line, 1024, fp) != NULL) {
        if (!strncmp(stdout_line, "\n", 1) && strlen(msg_header) && strlen(msg_payload)) {
            // Parse message
            struct sip_msg *msg;
            if ((msg = sip_load_message(msg_header, strdup((const char*) msg_payload)))) {
                // Update the ui
                ui_new_msg_refresh(msg);

                // Check if we have reach capture limit
                if (limit && sip_calls_count() >= limit) {
                    break;
                }

            }

            // Initialize structures
            strcpy(msg_header, "");
            strcpy(msg_payload, "");
        } else {
            if (!strncmp(stdout_line, "U ", 2)) {
                strcpy(msg_header, stdout_line);
            } else {
                if (strlen(msg_header)) {
                    strcat(msg_payload, stdout_line);
                }
            }
        }
    }

    // Close read pipe
    pclose(fp);
    return 0;
}

#ifndef WITH_LIBPCAP
int
load_from_file(const char* file)
{
    char cmdline[256];
    FILE *fp;
    char stdout_line[2048] = "";
    char msg_header[256], msg_payload[20480];

    // Build the commald line to execute ngrep
    sprintf(cmdline, "%s %s %s %s -I %s", STDBUF_BIN, STDBUF_ARGS, NGREP_BIN, NGREP_ARGS, file);

    // Open the command for reading.
    fp = popen(cmdline, "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        return 1;
    }

    // Read the output a line at a time - output it.
    while (fgets(stdout_line, 1024, fp) != NULL) {
        if (!strncmp(stdout_line, "\n", 1) && strlen(msg_header) && strlen(msg_payload)) {
            // Parse message
            sip_load_message(msg_header, strdup((const char*) msg_payload));
            // Initialize structures
            memset(msg_header, 0, 256);
            memset(msg_payload, 0, 20480);
        } else {
            if (!strncmp(stdout_line, "U ", 2)) {
                strcpy(msg_header, stdout_line);
            } else {
                if (strlen(msg_header)) {
                    strcat(msg_payload, stdout_line);
                }
            }
        }
    }

    // Close read pipe
    pclose(fp);
    return 0;
}
#endif
#endif
