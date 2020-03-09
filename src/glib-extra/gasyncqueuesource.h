/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2019 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2019 Irontec SL. All rights reserved.
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
 * @file gasyncqueuesource.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to GSource for GAsyncQueue
 *
 * This GSource is implemented based on the Custom GSource tutorial available at
 * https://developer.gnome.org/gnome-devel-demos/stable/custom-gsource.c.html.en
 */

#ifndef SNGREP_GASYNCQUEUESOURCE_H
#define SNGREP_GASYNCQUEUESOURCE_H

#include <glib.h>


typedef struct _GAsyncQueueSource GAsyncQueueSource;

typedef gboolean (*GAsyncQueueSourceFunc)(gpointer message, gpointer user_data);


struct _GAsyncQueueSource
{
    GSource parent;
    GAsyncQueue *queue;
    GDestroyNotify destroy;
};

GSource *
g_async_queue_source_new(GAsyncQueue *queue, GDestroyNotify destroy);


#endif //SNGREP_GASYNCQUEUESOURCE_H
