//
// Created by kaian on 9/04/18.
//

#include <glib.h>
#include "glib-utils.h"

gpointer
g_sequence_nth(GSequence *sequence, guint index)
{
    GSequenceIter *pos = g_sequence_get_iter_at_pos(sequence, index);
    return (g_sequence_iter_is_end(pos)) ? NULL : g_sequence_get(pos);
}


gint
g_sequence_iter_length(GSequenceIter *iter)
{
    return g_sequence_get_length(g_sequence_iter_get_sequence(iter));
}

void
g_sequence_iter_set_pos(GSequenceIter **iter, gint pos)
{
    *iter = g_sequence_get_iter_at_pos(g_sequence_iter_get_sequence(*iter), pos);
}

gint
g_sequence_index(GSequence *sequence, gconstpointer item)
{
    GSequenceIter *it = g_sequence_get_begin_iter(sequence);

    for (;!g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        if (g_sequence_get(it) == item) {
            return g_sequence_iter_get_position(it);
        }
    }

    return -1;
}

void
g_sequence_remove_data(GSequence *sequence, gconstpointer item)
{
    GSequenceIter *it = g_sequence_get_begin_iter(sequence);

    for (;!g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        if (g_sequence_get(it) == item) {
            g_sequence_remove(g_sequence_iter_prev(it));
            break;
        }
    }
}

void
g_sequence_remove_all(GSequence *sequence)
{
    g_sequence_remove_range(
        g_sequence_get_begin_iter(sequence),
        g_sequence_get_end_iter(sequence)
    );
}

void
g_sequence_append_sequence(GSequence *sequence, GSequence *items)
{
    GSequenceIter *it = g_sequence_get_begin_iter(items);
    for (;!g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        g_sequence_append(sequence, g_sequence_get(it));
    }
}

GSequence *
g_sequence_copy(GSequence *sequence, GEqualFunc filter_func, gpointer filter_data)
{
    GSequence *copy = g_sequence_new(NULL);

    GSequenceIter *it = g_sequence_get_begin_iter(sequence);
    for (;!g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {

        if (filter_func && !filter_func(g_sequence_get(it), filter_data))
            continue;

        g_sequence_append(copy, g_sequence_get(it));
    }

    return copy;
}