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
 * @file separator.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 */

#include "config.h"
#include "tui/tui.h"
#include "tui/theme.h"
#include "tui/widgets/orientable.h"
#include "tui/widgets/separator.h"

enum
{
    PROP_0,
    PROP_ORIENTATION,
    N_PROPERTIES,
};

// Class definition
G_DEFINE_TYPE_WITH_CODE(SngSeparator, sng_separator, SNG_TYPE_WIDGET,
                        G_IMPLEMENT_INTERFACE(SNG_TYPE_ORIENTABLE, NULL))

SngWidget *
sng_separator_new(SngOrientation orientation)
{
    return g_object_new(
        SNG_TYPE_SEPARATOR,
        "orientation", orientation,
        "visible", TRUE,
        "hexpand", TRUE,
        "can-focus", FALSE,
        NULL
    );
}

static void
sng_separator_draw(SngWidget *widget)
{
    SngSeparator *separator = SNG_SEPARATOR(widget);

    WINDOW *win = sng_widget_get_ncurses_window(widget);
    wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    if (separator->orientation == SNG_ORIENTATION_HORIZONTAL) {
        tui_whline(win, 0, 0, ACS_HLINE, sng_widget_get_width(widget));
    } else {
        tui_wvline(win, 0, 0, ACS_VLINE, sng_widget_get_height(widget));
    }

    // Chain up parent draw
    SNG_WIDGET_CLASS(sng_separator_parent_class)->draw(widget);
}

static gint
sng_separator_get_preferred_width(SngWidget *widget)
{
    SngSeparator *separator = SNG_SEPARATOR(widget);
    if (separator->orientation == SNG_ORIENTATION_VERTICAL) {
        return 1;
    }
    return sng_widget_get_width(widget);
}

static gint
sng_separator_get_preferred_height(SngWidget *widget)
{
    SngSeparator *separator = SNG_SEPARATOR(widget);
    if (separator->orientation == SNG_ORIENTATION_HORIZONTAL) {
        return 1;
    }
    return sng_widget_get_height(widget);
}

static void
sng_separator_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
    SngSeparator *separator = SNG_SEPARATOR(object);
    switch (property_id) {
        case PROP_ORIENTATION:
            separator->orientation = g_value_get_enum(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
sng_separator_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    SngSeparator *separator = SNG_SEPARATOR(object);
    switch (property_id) {
        case PROP_ORIENTATION:
            g_value_set_enum(value, separator->orientation);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
sng_separator_class_init(SngSeparatorClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->set_property = sng_separator_set_property;
    object_class->get_property = sng_separator_get_property;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->draw = sng_separator_draw;
    widget_class->preferred_width = sng_separator_get_preferred_width;
    widget_class->preferred_height = sng_separator_get_preferred_height;

    g_object_class_override_property(
        object_class,
        PROP_ORIENTATION,
        "orientation"
    );
}


static void
sng_separator_init(G_GNUC_UNUSED SngSeparator *self)
{
}
