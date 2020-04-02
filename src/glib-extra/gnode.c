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
 * @file gnode.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Helper functions for GList and GSList containers
 */
#include "config.h"
#include "gnode.h"

gpointer
g_node_parent_data(GNode *node)
{
    return (node->parent)
           ? node->parent->data
           : NULL;
}

gpointer
g_node_nth_child_data(GNode *node, guint index)
{
    GNode *child = g_node_nth_child(node, index);
    return (child != NULL)
           ? child->data
           : NULL;
}
