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
 * @file widget.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Basic SngWidget interface component
 */

#ifndef __SNGREP_WIDGET_H__
#define __SNGREP_WIDGET_H__

#include <glib.h>
#include <glib-object.h>
#include <ncurses.h>

G_BEGIN_DECLS

#define SNG_TYPE_WIDGET sng_widget_get_type()
G_DECLARE_DERIVABLE_TYPE(SngWidget, sng_widget, SNG, WIDGET, GObject)

//! Possible key handler results
typedef enum
{
    KEY_HANDLED = 0,        // Panel has handled the key, dont'use default key handler
    KEY_NOT_HANDLED = -1,   // Panel has not handled the key, try default key handler
    KEY_PROPAGATED = -2,    // Panel destroys and requests previous panel to handle key
    KEY_DESTROY = -3,       // Panel request destroy
} SngWidgetKeyHandlerRet;

/**
 * @brief Panel information structure
 *
 * This struct contains the panel related data, including
 * a pointer to the function that manages its drawing
 */
struct _SngWidgetClass
{
    //! Parent class
    GObjectClass parent;
    //! Handle widget content changes
    void (*update)(SngWidget *widget);
    //! Handle widget size request
    void (*size_request)(SngWidget *widget);
    //! Create Ncurses components for the widget
    void (*realize)(SngWidget *widget);
    //! Map the widget into the screen
    void (*map)(SngWidget *widget);
    //! Request widget to draw its data in their internal window
    void (*draw)(SngWidget *widget);
    //! Callback for focused event
    void (*focus_gained)(SngWidget *widget);
    //! Callback for focused event
    void (*focus_lost)(SngWidget *widget);
    //! Handle a custom keybinding on this panel
    void (*key_pressed)(SngWidget *widget, gint key);
    //! Handle a mouse event on this panel
    void (*clicked)(SngWidget *widget, MEVENT event);
    //! Get widget preferred height
    gint (*preferred_height)(SngWidget *widget);
    //! Get widget preferred width
    gint (*preferred_width)(SngWidget *widget);
};

/**
 * @brief Create a ncurses panel for the given ui
 *
 * Create a panel and associated widget and store their
 * pointers in ui structure.
 * If height and width doesn't match the screen dimensions
 * the panel will be centered on the screen.
 *
 * @param height panel widget height
 * @param width panel windo width
 * @return widget instance pointer
 */
SngWidget *
sng_widget_new();

void
sng_widget_free(SngWidget *widget);

/**
 * @brief Destroy a panel structure
 *
 * Removes the panel associated to the given ui and free
 * its memory. Most part of this task is done in the custom
 * destroy function of the panel.
 *
 * @param widget UI structure
 */
void
sng_widget_destroy(SngWidget *widget);

gboolean
sng_widget_is_destroying(SngWidget *widget);

void
sng_widget_set_parent(SngWidget *widget, SngWidget *parent);

SngWidget *
sng_widget_get_parent(SngWidget *widget);

void
sng_widget_set_name(SngWidget *widget, const gchar *name);

void
sng_widget_show(SngWidget *widget);

void
sng_widget_hide(SngWidget *widget);

gboolean
sng_widget_is_visible(SngWidget *widget);

gboolean
sng_widget_is_hidden(SngWidget *widget);

gboolean
sng_widget_is_realized(SngWidget *widget);

gboolean
sng_widget_has_focus(SngWidget *widget);

SngWidget *
sng_widget_get_toplevel(SngWidget *widget);

void
sng_widget_update(SngWidget *widget);

void
sng_widget_size_request(SngWidget *widget);

void
sng_widget_realize(SngWidget *widget);

/**
 * @brief Notifies current ui the screen size has changed
 *
 * This function acts as wrapper to custom ui resize functions
 * with some checks
 *
 * @param widget UI structure
 * @return 0 if ui has been resize, -1 otherwise
 */
void
sng_widget_draw(SngWidget *widget);



void
sng_widget_map(SngWidget *widget);

/**
 * @brief Callback when widget receives window focus
 * @param widget SngWidget to be focused
 * @return TRUE if the widget can be focused, FALSE otherwise
 */
void
sng_widget_focus_gain(SngWidget *widget);

/**
 * @brief Callback when widget loses window focus
 * @param widget SngWidget to remove focus from
 */
void
sng_widget_focus_lost(SngWidget *widget);

void
sng_widget_lose_focus(SngWidget *widget);

void
sng_widget_grab_focus(SngWidget *widget);

/**
 * @brief Handle moves events on given widget
 *
 * This function will pass the mouse event
 * to the given widget function.
 *
 * @return enum @key_handler_ret*
 */
void
sng_widget_clicked(SngWidget *widget, MEVENT event);

/**
 * @brief Handle key inputs on given UI
 *
 * This function will pass the input key sequence
 * to the given UI.
 *
 * @param ui UI structure
 * @param key keycode sequence of the pressed keys and mods
 * @return enum @key_handler_ret*
 */
void
sng_widget_key_pressed(SngWidget *widget, gint key);

gint
sng_widget_get_preferred_height(SngWidget *widget);

gint
sng_widget_get_preferred_width(SngWidget *widget);

void
sng_widget_set_ncurses_window(SngWidget *widget, WINDOW *win);

WINDOW *
sng_widget_get_ncurses_window(SngWidget *widget);

void
sng_widget_set_size(SngWidget *widget, gint width, gint height);

void
sng_widget_set_width(SngWidget *widget, gint width);

gint
sng_widget_get_width(SngWidget *widget);

void
sng_widget_set_height(SngWidget *widget, gint height);

gint
sng_widget_get_height(SngWidget *widget);

void
sng_widget_set_position(SngWidget *widget, gint xpos, gint ypos);

gint
sng_widget_get_xpos(SngWidget *widget);

gint
sng_widget_get_ypos(SngWidget *widget);

void
sng_widget_set_vexpand(SngWidget *widget, gboolean expand);

gboolean
sng_widget_get_vexpand(SngWidget *widget);

void
sng_widget_set_hexpand(SngWidget *widget, gboolean expand);

gboolean
sng_widget_get_hexpand(SngWidget *widget);

void
sng_widget_set_floating(SngWidget *widget, gboolean floating);

gboolean
sng_widget_is_floating(SngWidget *widget);

gboolean
sng_widget_can_focus(SngWidget *widget);


G_END_DECLS

#endif /* __SNGREP_WIDGET_H__ */
