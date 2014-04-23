/*
 * mate-gsettings.h: helper API for GSettings
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

#ifndef __MATE_GSETTINGS_H__
#define __MATE_GSETTINGS_H__

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

gboolean mate_gsettings_schema_exists (const gchar* schema);

gboolean mate_gsettings_append_strv (GSettings         *settings,
                                     const gchar       *key,
                                     const gchar       *value);

gboolean mate_gsettings_remove_all_from_strv (GSettings         *settings,
                                              const gchar       *key,
                                              const gchar       *value);

GSList*  mate_gsettings_strv_to_gslist (const gchar *const *array);

G_END_DECLS

#endif /* __MATE_GSETTINGS_H__ */
