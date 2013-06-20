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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sip.h"
#include "curses.h"

#define STDBUF_ARGS "-i0 -o0 -e0"
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
int run_ngrep(void *pargv)
{
    char **argv = (char**) pargv;
    int argc = 1;
    char cmdline[256];
    FILE *fp;
    char stdout_line[2048] = "";
    char msg_header[256], msg_payload[20480];

    // Build the commald line to execute ngrep
    sprintf(cmdline, "%s %s %s %s", STDBUF_BIN, STDBUF_ARGS, NGREP_BIN, NGREP_ARGS);
    while (argv[argc])
        sprintf(cmdline, "%s %s", cmdline, argv[argc++]);

    // Open the command for reading. 
    fp = popen(cmdline, "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        return 1;
    }

    /* Read the output a line at a time - output it. */
    while (fgets(stdout_line, 1024, fp) != NULL) {
        if (!strncmp(stdout_line, "\n", 1) && strlen(msg_header) && strlen(msg_payload)) {
            // Parse message 
            struct sip_msg *msg;
            if ((msg = sip_parse_message(msg_header, strdup((const char*) msg_payload)))) {
                // Update the ui
                refresh_call_ui(msg->call->callid);
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

    /* close */
    pclose(fp);
    return 0;
}

