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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

#ifndef TEST_BINARY
#define TEST_BINARY "../sngrep"
#endif

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
int ppipe[2];
int ret = 0;
int child;

void
failure(int signal)
{
    fprintf(stderr, "Test failed.\n");
    kill(child, SIGINT);
    exit(signal);
}

int
main()
{
    pipe(ppipe);

    // Max test duration
    alarm(TEST_MAX_DURATION);

    if ((child = fork()) < 0) {
        fprintf(stderr, "Fatal: unable to fork test.\n");
        return 127;
    } else if (child == 0) {
        // Child process, run sngrep with test pcap
        dup2(ppipe[0], STDIN_FILENO);
        char *argv[] = { TEST_BINARY, "-I", TEST_PCAP_INPUT, 0 };
        if (execv(argv[0], argv) == -1) {
            fprintf(stderr, "Failed to start sngrep: %s\n", strerror(errno));
        }
    } else {
        signal(SIGALRM, failure);
        // Parent process, send keys to child stdin
        usleep(TEST_INITIAL_WAIT);
        for (int i = 0; keys[i]; i++) {
            write(ppipe[1], &keys[i], sizeof(char));
            usleep(TEST_KEY_DELAY);
        }
        wait(&ret);
    }

    return ret;
}
