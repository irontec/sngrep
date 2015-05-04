/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2015 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2015 Irontec SL. All rights reserved.
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
 * @file media.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage call media
 *
 */

#ifndef __SNGREP_MEDIA_H_
#define __SNGREP_MEDIA_H_

#include "config.h"

typedef struct media_sess media_t;

struct media_sess
{
    //! Local port
    int lport;
    //! Remote port
    int rport;
    //! Remote address
    char *raddress;
    //! Number of packets transmited
    int txcnt;
    //! Number of packets received
    int rvcnt;

    //! Next media in same call
    struct media_sess *next;
};

const char *
media_create(int lport, int rport, const char *raddress)
{

    return 0;
}



#endif /* __SNGREP_MEDIA_H_ */
