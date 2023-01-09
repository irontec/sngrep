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
 * @file test_input.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * Basic input injector for sngrep testing
 */
#include "config.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#ifndef TEST_MAX_DURATION
#define TEST_MAX_DURATION 60
#endif

#ifndef TEST_INITIAL_WAIT
#define TEST_INITIAL_WAIT 1000 * 1200
#endif

#ifndef TEST_KEY_DELAY
#define TEST_KEY_DELAY 3000
#endif

#ifndef TEST_PCAP_INPUT
#define TEST_PCAP_INPUT "aaa.pcap"
#endif

/* keys array needs to be of type "char" */
const char keys[];

int
main()
{
    int ppipe[2];
    int unused, ret = 0;
    int child;
    unused = pipe(ppipe);

    // Max test duration
    alarm(TEST_MAX_DURATION);

    if ((child = fork()) < 0) {
        fprintf(stderr, "Fatal: unable to fork test.\n");
        return 127;
    } else if (child == 0) {
        // Child process, run sngrep with test pcap
        dup2(ppipe[0], STDIN_FILENO);
        char *argv[] = { "../src/sngrep", "-I", TEST_PCAP_INPUT, 0 };
        execv(argv[0], argv);
    } else {
        // Parent process, send keys to child stdin
        usleep(TEST_INITIAL_WAIT);
        int i;
        for (i = 0; keys[i]; i++) {
            unused = write(ppipe[1], &keys[i], sizeof(char));
            usleep(TEST_KEY_DELAY);
        }
        unused = wait(&ret);
    }

    return ret;
}
