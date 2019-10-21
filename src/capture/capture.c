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
 * @file capture.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in capture.h
 *
 */

#include "config.h"
#include <glib.h>
#include "glib/gasyncqueuesource.h"
#include "setting.h"
#include "storage/storage.h"
#include "capture.h"

static CaptureManager *manager = NULL;

CaptureManager *
capture_manager_new()
{
    manager = g_malloc0(sizeof(CaptureManager));

    manager->paused = FALSE;

#ifdef WITH_SSL
    // Parse TLS Server setting
    manager->tlsserver = address_from_str(setting_get_value(SETTING_CAPTURE_TLSSERVER));
#endif

    manager->loop = g_main_loop_new(
        g_main_context_new(),
        FALSE
    );

    return manager;
}

void
capture_manager_free(CaptureManager *manager)
{
    // Stop all capture inputs
    for (GSList *le = manager->inputs; le != NULL; le = le->next) {
        capture_input_unref(le->data);
    }

    // Close all capture outputs
    for (GSList *le = manager->outputs; le != NULL; le = le->next) {
        capture_output_unref(le->data);
    }

    g_slist_free(manager->inputs);
    g_slist_free(manager->outputs);
    g_free(manager->filter);
    g_main_loop_unref(manager->loop);
    g_free(manager);
}

CaptureManager *
capture_manager_get_instance()
{
    return manager;
}

static gpointer
capture_manager_thread(CaptureManager *manager)
{
    g_main_loop_run(manager->loop);
    return NULL;
}

void
capture_manager_start(CaptureManager *manager)
{
    manager->thread = g_thread_new(NULL, (GThreadFunc) capture_manager_thread, manager);
}

void
capture_manager_stop(CaptureManager *manager)
{
    // Close all capture inputs
    for (GSList *le = manager->inputs; le != NULL; le = le->next) {
        capture_input_stop(le->data);
    }

    // Close all capture outputs
    for (GSList *le = manager->outputs; le != NULL; le = le->next) {
        capture_output_close(le->data);
    }

    // Stop manager thread
    g_main_loop_quit(manager->loop);
    g_thread_join(manager->thread);
}

gboolean
capture_manager_set_filter(CaptureManager *manager, gchar *filter, GError **error)
{
    // Apply fitler to all capture inputs
    for (GSList *le = manager->inputs; le != NULL; le = le->next) {
        if (capture_input_filter(le->data, filter, error) == 0) {
            manager->filter = NULL;
            return FALSE;
        }
    }

    // Store capture filter
    manager->filter = g_strdup(filter);
    return TRUE;
}

const gchar *
capture_manager_filter(CaptureManager *manager)
{
    return manager->filter;
}

void
capture_manager_set_keyfile(CaptureManager *manager, const gchar *keyfile, G_GNUC_UNUSED GError **error)
{
    manager->keyfile = keyfile;
}

void
capture_manager_add_input(CaptureManager *manager, CaptureInput *input)
{
    capture_input_set_manager(input, manager);

    g_source_attach(
        capture_input_source(input),
        g_main_loop_get_context(manager->loop)
    );

    manager->inputs = g_slist_append(manager->inputs, input);
}

void
capture_manager_add_output(CaptureManager *manager, CaptureOutput *output)
{
    capture_output_set_manager(output, manager);
    manager->outputs = g_slist_append(manager->outputs, output);
}

void
capture_manager_output_packet(CaptureManager *manager, Packet *packet)
{
    for (GSList *l = manager->outputs; l != NULL; l = l->next) {
        capture_output_write(l->data, packet);
    }
}

const gchar *
capture_status_desc(CaptureManager *manager)
{
    int online = 0, offline = 0, loading = 0;

    for (GSList *l = manager->inputs; l != NULL; l = l->next) {
        CaptureInput *input = l->data;

        if (capture_input_mode(input) == CAPTURE_MODE_OFFLINE) {
            offline++;
            if (!g_source_is_destroyed(capture_input_source(input))) {
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

const gchar *
capture_keyfile(CaptureManager *manager)
{
    return manager->keyfile;
}

gboolean
capture_is_online(CaptureManager *manager)
{
    for (GSList *l = manager->inputs; l != NULL; l = l->next) {
        CaptureInput *input = l->data;

        if (capture_input_mode(input) == CAPTURE_MODE_OFFLINE)
            return FALSE;
    }

    return TRUE;
}

Address *
capture_tls_server(CaptureManager *manager)
{
    return manager->tlsserver;
}

guint
capture_sources_count(CaptureManager *manager)
{
    return g_slist_length(manager->inputs);
}

void
capture_manager_set_pause(CaptureManager *manager, gboolean paused)
{
    manager->paused = paused;
}

gboolean
capture_is_running()
{
    for (GSList *l = manager->inputs; l != NULL; l = l->next) {
        if (g_source_is_destroyed(capture_input_source(l->data)) == FALSE) {
            return TRUE;
        }
    }

    return FALSE;
}
