/*
 * mate-dconf.c: helper API for dconf
 *
 * Copyright (C) 2011 Novell, Inc.
 * Copyright (C) 2013 Stefano Karapetsas
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Authors:
 *  Vincent Untz <vuntz@gnome.org>
 *  Stefano Karapetsas <stefano@karapetsas.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <dconf.h>

#include "mate-dconf.h"

static DConfClient *
mate_dconf_client_get (void)
{
#ifdef HAVE_DCONF_0_13
    return dconf_client_new ();
#else
    return dconf_client_new (NULL, NULL, NULL, NULL);
#endif
}

/**
 * mate_dconf_write_sync:
 * @key: the key to write.
 * @value: the value to write.
 * @error: a variable to store the error, or NULL.
 *
 * Allow to write a value to dconf.
 *
 * Since: 1.7.1
 */
gboolean
mate_dconf_write_sync (const gchar  *key,
                       GVariant     *value,
                       GError      **error)
{
    gboolean     ret;
    DConfClient *client = mate_dconf_client_get ();

#ifdef HAVE_DCONF_0_13
    ret = dconf_client_write_sync (client, key, value, NULL, NULL, error);
#else
    ret = dconf_client_write (client, key, value, NULL, NULL, error);
#endif

    g_object_unref (client);

    return ret;
}

/**
 * mate_dconf_recursive_reset:
 * @dir: the dconf directory to reset.
 * @error: a variable to store the error, or NULL.
 *
 * Allow to reset a dconf path.
 * 
 * Since: 1.7.1
 */
gboolean
mate_dconf_recursive_reset (const gchar  *dir,
                            GError      **error)
{
    gboolean     ret;
    DConfClient *client = mate_dconf_client_get ();

#ifdef HAVE_DCONF_0_13
    ret = dconf_client_write_sync (client, dir, NULL, NULL, NULL, error);
#else
    ret = dconf_client_write (client, dir, NULL, NULL, NULL, error);
#endif

    g_object_unref (client);

    return ret;
}

/**
 * mate_dconf_list_subdirs:
 * @dir: the dconf directory.
 * @remove_trailing_slash: whether to remove the trailing slash from
 * paths.
 *
 * Returns the list of subdirectories of the given dconf directory.
 *
 * Return value: the list of subdirectories.
 * 
 * Since: 1.7.1
 */
gchar **
mate_dconf_list_subdirs (const gchar *dir,
                         gboolean     remove_trailing_slash)
{
    GArray       *array;
    gchar       **children;
    int       len;
    int       i;
    DConfClient  *client = mate_dconf_client_get ();

    array = g_array_new (TRUE, TRUE, sizeof (gchar *));

    children = dconf_client_list (client, dir, &len);

    g_object_unref (client);

    for (i = 0; children[i] != NULL; i++) {
        if (dconf_is_rel_dir (children[i], NULL)) {
            char *val = g_strdup (children[i]);

            if (remove_trailing_slash)
                val[strlen (val) - 1] = '\0';

            array = g_array_append_val (array, val);
        }
    }

    g_strfreev (children);

    return (gchar **) g_array_free (array, FALSE);
}

/**
 * mate_dconf_sync:
 *
 * Ensure dconf daemon syncs the written values.
 *
 * Since: 1.7.1
 */
void mate_dconf_sync ()
{
#ifdef HAVE_DCONF_0_13
    DConfClient  *client = mate_dconf_client_get ();
    dconf_client_sync (client);
    g_object_unref (client);
#endif
}
