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
 * @file capture.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in capture.h
 *
 */

#include "config.h"
#include <glib.h>
#include "setting.h"
#include "capture.h"

static CaptureManager *manager;

CaptureManager *
capture_manager_new()
{
    manager = g_malloc0(sizeof(CaptureManager));

    manager->queue = g_async_queue_new();
    manager->paused = FALSE;

#if defined(WITH_GNUTLS) || defined(WITH_OPENSSL)
    // Parse TLS Server setting
    manager->tlsserver = address_from_str(setting_get_value(SETTING_CAPTURE_TLSSERVER));
#endif

    g_rec_mutex_init(&manager->lock);

    return manager;
}

void
capture_manager_free(CaptureManager *manager)
{
    if (capture_is_running(manager))
        capture_manager_stop(manager);

    g_rec_mutex_clear(&manager->lock);
    g_free(manager);
}

CaptureManager *
capture_manager()
{
    return manager;
}

void
capture_manager_start(CaptureManager *manager)
{
    // Start all captures threads
    for (GSList *le = manager->inputs; le != NULL; le = le->next) {
        CaptureInput *input = le->data;
        input->running = TRUE;
        input->thread = g_thread_new(NULL, (void *) input->start, input);
    }
}

void
capture_manager_stop(CaptureManager *manager)
{
    // Stop all capture inputs
    for (GSList *le = manager->inputs; le != NULL; le = le->next) {
        CaptureInput *input = le->data;
        if (input->stop) {
            input->stop(input);
        }
        g_thread_join(input->thread);
    }

    // Close all capture outputs
    for (GSList *le = manager->outputs; le != NULL; le = le->next) {
        CaptureOutput *output = le->data;
        if (output->close) {
            output->close(output);
        }
    }
}

gboolean
capture_manager_set_filter(CaptureManager *manager, gchar *filter, GError **error)
{
    // Store capture filter
    manager->filter = filter;

    // Apply fitler to all capture inputs
    for (GSList *le = manager->inputs; le != NULL; le = le->next) {
        CaptureInput *input = le->data;
        if (input->filter) {
            if (!input->filter(input, filter, error)) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

const gchar *
capture_manager_filter(CaptureManager *manager)
{
    return manager->filter;
}

void
capture_manager_set_keyfile(CaptureManager *manager, gchar *keyfile, GError **error)
{
    manager->keyfile = keyfile;
}

void
capture_manager_add_input(CaptureManager *manager, CaptureInput *input)
{
    input->manager = manager;
    manager->inputs = g_slist_append(manager->inputs, input);
}

void
capture_manager_add_output(CaptureManager *manager, CaptureOutput *output)
{
    output->manager = manager;
    manager->outputs = g_slist_append(manager->outputs, output);
}

void
capture_lock(CaptureManager *manager)
{
    // Avoid parsing more packet
    g_rec_mutex_lock(&manager->lock);
}

void
capture_unlock(CaptureManager *manager)
{
    // Allow parsing more packets
    g_rec_mutex_unlock(&manager->lock);
}

void
capture_manager_output_packet(CaptureManager *manager, Packet *packet)
{
    for (GSList *l = manager->outputs; l != NULL; l = l->next) {
        CaptureOutput *output = l->data;
        if (output->write) {
            output->write(output, packet);
        }
    }
}

gboolean
capture_is_running(CaptureManager *manager)
{
    // Check if all capture inputs are running
    for (GSList *l = manager->inputs; l != NULL; l = l->next) {
        CaptureInput *input = l->data;
        if (input->running == FALSE) {
            return FALSE;
        }
    }
    return TRUE;
}

const gchar *
capture_status_desc(CaptureManager *manager)
{
    int online = 0, offline = 0, loading = 0;

    for (GSList *l = manager->inputs; l != NULL; l = l->next) {
        CaptureInput *input = l->data;

        if (input->mode == CAPTURE_MODE_OFFLINE) {
            offline++;
            if (input->running) {
                loading++;
            }
        } else {
            online++;
        }
    }

#ifdef USE_HEP
    // HEP Listen mode is always considered online
    if (FALSE /* @todo capture_hep_listen_port() */) {
        online++;
    }
#endif

    if (manager->paused) {
        if (online > 0 && offline == 0) {
            return "Online (Paused)";
        } else if (online == 0 && offline > 0) {
            return "Offline (Paused)";
        } else {
            return "Mixed (Paused)";
        }
    } else if (loading > 0) {
        if (online > 0 && offline == 0) {
            return "Online (Loading)";
        } else if (online == 0 && offline > 0) {
            return "Offline (Loading)";
        } else {
            return "Mixed (Loading)";
        }
    } else {
        if (online > 0 && offline == 0) {
            return "Online";
        } else if (online == 0 && offline > 0) {
            return "Offline";
        } else {
            return "Mixed";
        }
    }
}

const gchar*
capture_keyfile(CaptureManager *manager)
{
    return manager->keyfile;
}

gboolean
capture_is_online(CaptureManager *manager)
{
    for (GSList *l = manager->inputs; l != NULL; l = l->next) {
        CaptureInput *input = l->data;

        if (input->mode == CAPTURE_MODE_OFFLINE)
            return FALSE;
    }

    return TRUE;
}

Address
capture_tls_server(CaptureManager *manager)
{
    return manager->tlsserver;
}

guint
capture_sources_count(CaptureManager *manager)
{
    return g_slist_length(manager->inputs);
}