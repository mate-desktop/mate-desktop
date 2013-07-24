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

#include <gio/gio.h>

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