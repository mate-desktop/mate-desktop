/* test-monitor-migrate.c - per-monitor settings move from the fallback id
 *
 * Copyright (C) 2026 Kerem Soke
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Run with GSETTINGS_BACKEND=memory, the background schemas compiled into
 * GSETTINGS_SCHEMA_DIR, on a compositor whose mate_output_manager_v1 ids
 * differ from the fallback ids (two identical monitors). Exits 77 when the
 * ids agree, as there is nothing to migrate.
 */

#include <config.h>
#include <gtk/gtk.h>

#define MATE_DESKTOP_USE_UNSTABLE_API
#include "mate-bg.h"
#include "mate-monitor-info.h"

static GSettings *
at (const gchar *id)
{
	gchar *path = g_strdup_printf (MATE_BG_MONITOR_PATH_PREFIX "%s/", id);
	GSettings *settings = g_settings_new_with_path (MATE_BG_MONITOR_SCHEMA, path);

	g_free (path);
	return settings;
}

static gboolean
check (gboolean ok, const gchar *what)
{
	g_print ("%s: %s\n", ok ? "ok" : "FAIL", what);
	return ok;
}

int
main (int argc, char **argv)
{
	MateMonitorInfo *info;
	GdkMonitor *monitor;
	GSettings *old_settings, *new_settings;
	gchar *id, *old_id, *bg_id, *value;
	gboolean ok = TRUE;

	gtk_init (&argc, &argv);
	info = mate_monitor_info_get_default ();
	monitor = gdk_display_get_monitor (gdk_display_get_default (), 0);
	id = mate_monitor_info_get_id (info, monitor);
	old_id = mate_monitor_info_get_fallback_id (info, monitor);
	if (g_strcmp0 (id, old_id) == 0) {
		g_print ("skip: id %s is the fallback id\n", id);
		return 77;
	}
	bg_id = mate_bg_get_monitor_id (monitor);
	ok &= check (g_strcmp0 (bg_id, id) == 0, "mate_bg_get_monitor_id () is the protocol's id");

	old_settings = at (old_id);
	new_settings = at (id);
	g_settings_set_string (old_settings, "picture-filename", "/old.png");
	g_settings_set_boolean (old_settings, "use-shared", FALSE);
	g_object_unref (mate_bg_get_monitor_settings (monitor));
	value = g_settings_get_string (new_settings, "picture-filename");
	ok &= check (g_strcmp0 (value, "/old.png") == 0 &&
	             !g_settings_get_boolean (new_settings, "use-shared"),
	             "the fallback id's settings were copied");
	g_free (value);

	g_settings_set_string (new_settings, "picture-filename", "/new.png");
	g_settings_set_string (old_settings, "picture-filename", "/older.png");
	mate_monitor_info_migrate_settings (info, monitor, MATE_BG_MONITOR_SCHEMA,
	                                    MATE_BG_MONITOR_PATH_PREFIX);
	value = g_settings_get_string (new_settings, "picture-filename");
	ok &= check (g_strcmp0 (value, "/new.png") == 0, "settings already at the id are not overwritten");
	g_free (value);
	value = g_settings_get_string (old_settings, "picture-filename");
	ok &= check (g_strcmp0 (value, "/older.png") == 0, "the fallback id's settings stay");
	g_free (value);

	g_object_unref (old_settings);
	g_object_unref (new_settings);
	g_free (id);
	g_free (old_id);
	g_free (bg_id);
	return ok ? 0 : 1;
}
