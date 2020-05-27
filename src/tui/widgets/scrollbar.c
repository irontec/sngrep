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
 * @file scrollbar.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in scrollbar.h
 */
#include "config.h"
#include <glib-object.h>
#include "tui/tui.h"
#include "tui/widgets/scrollbar.h"

enum
{
    PROP_0,
    PROP_ORIENTATION,
    N_PROPERTIES,
};

// Class definition
G_DEFINE_TYPE_WITH_CODE(SngScrollbar, sng_scrollbar, SNG_TYPE_WIDGET,
                        G_IMPLEMENT_INTERFACE(SNG_TYPE_ORIENTABLE, NULL))

SngWidget *
sng_scrollbar_new(SngOrientation orientation)
{
    return g_object_new(
        SNG_TYPE_SCROLLBAR,
        "orientation", orientation,
        NULL
    );
}

gint
sng_scrollbar_get_position(SngScrollbar *scrollbar)
{
    return scrollbar->position;
}

void
sng_scrollbar_set_position(SngScrollbar *scrollbar, gint position)
{
    scrollbar->position = CLAMP(position, 0, scrollbar->max_position);
}

gint
sng_scrollbar_get_max_position(SngScrollbar *scrollbar)
{
    return scrollbar->max_position;
}

void
sng_scrollbar_set_max_position(SngScrollbar *scrollbar, gint max_position)
{
    scrollbar->max_position = max_position;
}

gint
sng_scrollbar_get_max(SngScrollbar *scrollbar)
{
    SngWidget *widget = SNG_WIDGET(scrollbar);
    g_return_val_if_fail(widget != NULL, 0);

    SngWidget *parent = sng_widget_get_parent(widget);
    g_return_val_if_fail(parent != NULL, 0);
    g_return_val_if_fail(SNG_IS_SCROLLABLE(parent), 0);

    // Get scrollable content area
    SngWidget *content = sng_scrollable_get_content(SNG_SCROLLABLE(parent));

    WINDOW *content_win = sng_widget_get_ncurses_window(content);
    g_return_val_if_fail(content_win != NULL, 0);

    if (scrollbar->orientation == SNG_ORIENTATION_VERTICAL) {
        // Max parent win height minus padding size
        return getmaxy(content_win) - (sng_widget_get_ypos(widget) - sng_widget_get_ypos(parent));
    } else {
        // Max parent win width minus padding size
        return getmaxx(content_win) - (sng_widget_get_xpos(widget) - sng_widget_get_xpos(parent));
    }
}

static void
sng_scrollbar_draw_vertical(SngWidget *widget)
{
    SngScrollbar *scrollbar = SNG_SCROLLBAR(widget);
    g_return_if_fail(scrollbar != NULL);

    WINDOW *win = sng_widget_get_ncurses_window(widget);

    // Scrollbar total height
    gint height = sng_widget_get_height(widget);
    // Max scrollbar position
    gint max = sng_scrollbar_get_max(scrollbar);

    // Initialize scrollbar line
    mvwvline(win, 0, 0, ACS_VLINE, height);

    // How long the scroll will be
    gint scroll_len = CLAMP(
        ((height * 1.0f) / max) * height,
        1,
        height
    );

    // Where will the scroll start
    gint scroll_ypos;
    if (scrollbar->position == 0) {
        scroll_ypos = 0;
    } else if (scrollbar->position + height >= max) {
        scroll_ypos = height - scroll_len;
    } else {
        scroll_ypos = (height - 1.0f) * (scrollbar->position * 1.0f / max);
    }

    // Draw the N blocks of the scrollbar
    for (gint line = 0; line <= scroll_len; line++) {
        mvwaddwstr(
            win,
            line + scroll_ypos,
            0,
            tui_acs_utf8(ACS_BOARD)
        );
    }
}

static void
sng_scrollbar_draw_horizontal(SngWidget *widget)
{
    SngScrollbar *scrollbar = SNG_SCROLLBAR(widget);
    g_return_if_fail(scrollbar != NULL);

    WINDOW *win = sng_widget_get_ncurses_window(widget);

    // Scrollbar total width
    gint width = sng_widget_get_width(widget);
    // Max scrollbar position
    gint max = sng_scrollbar_get_max(scrollbar);

    // Initialize scrollbar line
    mvwhline(win, 0, 0, ACS_HLINE, width);

    // How long the scroll will be
    gint scroll_len = CLAMP(
        ((width * 1.0f) / max) * width,
        1,
        width
    );

    // Where will the scroll start
    gint scroll_xpos;
    if (scrollbar->position == 0) {
        scroll_xpos = 0;
    } else if (scrollbar->position + width >= max) {
        scroll_xpos = width - scroll_len;
    } else {
        scroll_xpos = (width - 1.0f) * (scrollbar->position * 1.0f / max);
    }

    // Draw the N blocks of the scrollbar
    for (gint line = 0; line <= scroll_len; line++) {
        mvwaddwstr(
            win,
            0,
            line + scroll_xpos,
            tui_acs_utf8(ACS_CKBOARD)
        );
    }
}

static void
sng_scrollbar_draw(SngWidget *widget)
{
    SngScrollbar *scrollbar = SNG_SCROLLBAR(widget);
    if (scrollbar->orientation == SNG_ORIENTATION_VERTICAL) {
        sng_scrollbar_draw_vertical(widget);
    } else {
        sng_scrollbar_draw_horizontal(widget);
    }

    // Chain-up parent draw function
    SNG_WIDGET_CLASS(sng_scrollbar_parent_class)->draw(widget);
}

static void
sng_scrollbar_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
    SngScrollbar *scrollbar = SNG_SCROLLBAR(object);
    switch (property_id) {
        case PROP_ORIENTATION:
            scrollbar->orientation = g_value_get_enum(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
sng_scrollbar_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    SngScrollbar *scrollbar = SNG_SCROLLBAR(object);
    switch (property_id) {
        case PROP_ORIENTATION:
            g_value_set_enum(value, scrollbar->orientation);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
sng_scrollbar_class_init(SngScrollbarClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->set_property = sng_scrollbar_set_property;
    object_class->get_property = sng_scrollbar_get_property;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->draw = sng_scrollbar_draw;

    g_object_class_override_property(
        object_class,
        PROP_ORIENTATION,
        "orientation"
    );
}

static void
sng_scrollbar_init(G_GNUC_UNUSED SngScrollbar *self)
{
}
