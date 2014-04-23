/*
 * mate-gsettings.c: helper API for GSettings
 *
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
 *  Stefano Karapetsas <stefano@karapetsas.com>
 */

#include "mate-gsettings.h"

/**
 * mate_gsettings_schema_exists:
 * @schema: schema to check.
 *
 * Check if a given schema is installed in GSettings.
 *
 * Return value: TRUE if schema exists, FALSE if not.
 * 
 * Since: 1.7.1
 */
gboolean
mate_gsettings_schema_exists (const gchar* schema)
{
    const char * const *schemas;
    gboolean schema_exists;
    gint i;

    schemas = g_settings_list_schemas ();
    schema_exists = FALSE;

    for (i = 0; schemas[i] != NULL; i++) {
        if (g_strcmp0 (schemas[i], schema) == 0) {
            schema_exists = TRUE;
            break;
        }
    }

    return schema_exists;
}

/* (copied from gnome-panel)
 * Adapted from is_valid_keyname() in glib (gio/glib-compile-schemas.c)
 * Differences:
 *  - gettext support removed (we don't need translations here)
 *  - remove support for allow_any_name
 */
gboolean
mate_gsettings_is_valid_keyname (const gchar  *key,
                                 GError      **error)
{
  gint i;

  if (key[0] == '\0')
    {
      g_set_error_literal (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                           "empty names are not permitted");
      return FALSE;
    }

  if (!g_ascii_islower (key[0]))
    {
      g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                   "invalid name '%s': names must begin "
                     "with a lowercase letter", key);
      return FALSE;
    }

  for (i = 1; key[i]; i++)
    {
      if (key[i] != '-' &&
          !g_ascii_islower (key[i]) &&
          !g_ascii_isdigit (key[i]))
        {
          g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                       "invalid name '%s': invalid character '%c'; "
                         "only lowercase letters, numbers and dash ('-') "
                         "are permitted.", key, key[i]);
          return FALSE;
        }

      if (key[i] == '-' && key[i + 1] == '-')
        {
          g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                       "invalid name '%s': two successive dashes ('--') "
                         "are not permitted.", key);
          return FALSE;
        }
    }

  if (key[i - 1] == '-')
    {
      g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                   "invalid name '%s': the last character may not be a "
                     "dash ('-').", key);
      return FALSE;
    }

  if (i > 32)
    {
      g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                   "invalid name '%s': maximum length is 32", key);
      return FALSE;
    }

  return TRUE;
}

gboolean
mate_gsettings_append_strv (GSettings   *settings,
                            const gchar *key,
                            const gchar *value)
{
    gchar    **old;
    gchar    **new;
    gint       size;
    gboolean   retval;

    old = g_settings_get_strv (settings, key);

    for (size = 0; old[size] != NULL; size++);

    size += 1; /* appended value */
    size += 1; /* NULL */

    new = g_realloc_n (old, size, sizeof (gchar *));

    new[size - 2] = g_strdup (value);
    new[size - 1] = NULL;

    retval = g_settings_set_strv (settings, key,
                                  (const gchar **) new);

    g_strfreev (new);

    return retval;
}

gboolean
mate_gsettings_remove_all_from_strv (GSettings   *settings,
                                     const gchar *key,
                                     const gchar *value)
{
    GArray    *array;
    gchar    **old;
    gint       i;
    gboolean   retval;

    old = g_settings_get_strv (settings, key);
    array = g_array_new (TRUE, TRUE, sizeof (gchar *));

    for (i = 0; old[i] != NULL; i++) {
        if (g_strcmp0 (old[i], value) != 0)
            array = g_array_append_val (array, old[i]);
    }

    retval = g_settings_set_strv (settings, key,
                                  (const gchar **) array->data);

    g_strfreev (old);
    g_array_free (array, TRUE);

    return retval;
}

GSList*
mate_gsettings_strv_to_gslist (const gchar *const *array)
{
    GSList *list = NULL;
    gint i;
    if (array != NULL) {
        for (i = 0; array[i]; i++) {
            list = g_slist_append (list, g_strdup (array[i]));
        }
    }
    return list;
}
