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
 * @file {file}
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#include "flow_column.h"

#include "config.h"
#include <glib.h>
#include <glib-object.h>
#include "setting.h"
#include "storage/address.h"
#include "tui/theme.h"
#include "tui/widgets/flow_column.h"

enum
{
    PROP_IP = 1,
    PROP_PORT,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

G_DEFINE_TYPE(SngFlowColumn, sng_flow_column, SNG_TYPE_WIDGET)

SngWidget *
sng_flow_column_new(Address address)
{
    return g_object_new(
        SNG_TYPE_FLOW_COLUMN,
        "ip", address.ip,
        "port", address.port,
        "width", CF_COLUMN_WIDTH,
        "vexpand", TRUE,
        NULL
    );
}

static void
sng_flow_column_draw(SngWidget *widget)
{
    SngFlowColumn *column = SNG_FLOW_COLUMN(widget);

    // Draw horizontal line below column address
    WINDOW *win = sng_widget_get_ncurses_window(widget);
    tui_whline(win, 1, 9, ACS_HLINE, 21);
    tui_mvwaddch(win, 1, 19, ACS_TTEE);

    // Draw vertical line below column (in arrows pad)
    tui_wvline(win, 2, 19, ACS_VLINE, sng_widget_get_height(widget) - 2);

    // Set bold to this address if it's local
    if (setting_enabled(SETTING_TUI_CF_LOCALHIGHLIGHT)) {
        if (address_is_local(column->addr))
            wattron(win, A_BOLD);
    }

    if (setting_enabled(SETTING_TUI_CF_SPLITCALLID) || !address_get_port(column->addr)) {
        mvwprintw(win, 0, 11, "%s", address_get_ip(column->addr));
    } else  {
        mvwprintw(win, 0, 11, "%.*s:%hu",
                  SETTING_MAX_LEN - 7,
                  address_get_ip(column->addr),
                  address_get_port(column->addr)
        );
    }
}

static void
sng_flow_column_set_property(GObject *self, guint property_id, const GValue *value, GParamSpec *pspec)
{
    SngFlowColumn *column = SNG_FLOW_COLUMN(self);

    switch (property_id) {
        case PROP_IP:
            column->addr.ip = g_strdup(g_value_get_string(value));
            break;
        case PROP_PORT:
            column->addr.port = g_value_get_int(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}

static void
sng_flow_column_get_property(GObject *self, guint property_id, GValue *value, GParamSpec *pspec)
{
    SngFlowColumn *column = SNG_FLOW_COLUMN(self);
    switch (property_id) {
        case PROP_IP:
            g_value_set_string(value, column->addr.ip);
            break;
        case PROP_PORT:
            g_value_set_int(value, column->addr.port);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}

static void
sng_flow_column_class_init(G_GNUC_UNUSED SngFlowColumnClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->get_property = sng_flow_column_get_property;
    object_class->set_property = sng_flow_column_set_property;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->draw = sng_flow_column_draw;

    obj_properties[PROP_IP] =
        g_param_spec_string("ip",
                            "Column IP Address",
                            "Column IP Address",
                            NULL,
                            G_PARAM_CONSTRUCT | G_PARAM_READWRITE
        );

    obj_properties[PROP_PORT] =
        g_param_spec_int("port",
                         "Column IP Port",
                         "Column IP Port",
                         0,
                         65535,
                         0,
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE
        );


    g_object_class_install_properties(
        object_class,
        N_PROPERTIES,
        obj_properties
    );
}

static void
sng_flow_column_init(G_GNUC_UNUSED SngFlowColumn *column)
{
}
