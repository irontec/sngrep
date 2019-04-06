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
 * @file gasyncqueuesource.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to GSource for GAsyncQueue
 *
 * This GSource is implemented based on the Custom GSource tutorial available at
 * https://developer.gnome.org/gnome-devel-demos/stable/custom-gsource.c.html.en
 */
#include <glib.h>
#include <glib-object.h>
#include "gasyncqueuesource.h"

static gboolean
g_async_queue_source_prepare(GSource *source, G_GNUC_UNUSED gint *timeout)
{
    GAsyncQueueSource *g_async_queue_source = (GAsyncQueueSource *) source;
    return (g_async_queue_length(g_async_queue_source->queue) > 0);
}

static gboolean
g_async_queue_source_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
    GAsyncQueueSource *g_async_queue_source = (GAsyncQueueSource *) source;
    gpointer message;
    GAsyncQueueSourceFunc func = (GAsyncQueueSourceFunc) callback;

    /* Pop a message off the queue. */
    message = g_async_queue_try_pop(g_async_queue_source->queue);

    /* If there was no message, bail. */
    if (message == NULL) {
        /* Keep the source around to handle the next message. */
        return TRUE;
    }

    /* @func may be %NULL if no callback was specified.
     * If so, drop the message. */
    if (func == NULL) {
        if (g_async_queue_source->destroy != NULL) {
            g_async_queue_source->destroy(message);
        }

        /* Keep the source around to consume the next message. */
        return TRUE;
    }

    return func(message, user_data);
}

static void
g_async_queue_source_finalize(GSource *source)
{
    GAsyncQueueSource *g_async_queue_source = (GAsyncQueueSource *) source;
    g_async_queue_unref(g_async_queue_source->queue);
}

static gboolean
g_async_queue_source_closure_callback(gpointer message, gpointer user_data)
{
    GClosure *closure = user_data;
    GValue param_value = G_VALUE_INIT;
    GValue return_value = G_VALUE_INIT;
    gboolean retval;

    /* The invoked function is responsible for freeing @message. */
    g_value_init(&return_value, G_TYPE_BOOLEAN);
    g_value_init(&param_value, G_TYPE_POINTER);
    g_value_set_pointer(&param_value, message);

    g_closure_invoke(closure, &return_value, 1, &param_value, NULL);
    retval = g_value_get_boolean(&return_value);

    g_value_unset(&param_value);
    g_value_unset(&return_value);

    return retval;
}

static GSourceFuncs g_async_queue_source_funcs =
    {
        g_async_queue_source_prepare,
        NULL,  /* check */
        g_async_queue_source_dispatch,
        g_async_queue_source_finalize,
        (GSourceFunc) g_async_queue_source_closure_callback,
        NULL,
    };

GSource *
g_async_queue_source_new(GAsyncQueue *queue, GDestroyNotify destroy)
{
    g_return_val_if_fail (queue != NULL, NULL);

    GAsyncQueueSource *source = (GAsyncQueueSource *) g_source_new(
        &g_async_queue_source_funcs,
        sizeof(GAsyncQueueSource)
    );
    source->queue = g_async_queue_ref(queue);
    source->destroy = destroy;
    return (GSource *) source;
}
