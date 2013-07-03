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
 * @file exec.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage ngrep background execution
 *
 * This file contains the functions to spawn an ngrep process and read its
 * output. This process is done in a detached thread that will request UI
 * updates when new messages are parsed.
 *
 */
#ifndef __SNGREP_EXEC_H
#define __SNGREP_EXEC_H

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
/**
 * @brief Exec ngrep in background
 *
 * This function is used as worker thread for spawning a ngrep process and
 * read its output. This is only used in Online captures. All sngrep arguments
 * will be passed to sngrep directly.
 *
 * @param pargs Argument list for ngrep
 * @return 0 on spawn success, 1 otherwise
 */
extern int
online_capture(void *pargs);

#ifndef WITH_LIBPCAP
/**
 * @brief Loads ngrep data from PCAP file
 *
 * This spawns ngrep using -I parameter to read the contents of a PCAP file
 * and write its output so sngrep can parse it. This can be used instead of
 * libpcap parsing but will be WAAAAAAY slower.
 *
 * @param file Full path to pcap file
 * @return 0 on read success, 1 otherwise
 */
extern int
load_from_file(const char* file);
#endif
#endif
