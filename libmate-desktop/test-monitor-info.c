/* test-monitor-info.c - print what MateMonitorInfo reports
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
 */

#include <config.h>
#include <gtk/gtk.h>

#define MATE_DESKTOP_USE_UNSTABLE_API
#include "mate-monitor-info.h"

static const char *
owner_name (MateBackgroundOwner owner)
{
	switch (owner) {
	case MATE_BACKGROUND_OWNER_NONE:
		return "none";
	case MATE_BACKGROUND_OWNER_CLIENT:
		return "client";
	case MATE_BACKGROUND_OWNER_COMPOSITOR:
		return "compositor";
	default:
		return "unknown";
	}
}

static void
print_all (MateMonitorInfo *info)
{
	GdkDisplay *display = gdk_display_get_default ();
	int n = gdk_display_get_n_monitors (display);

	g_print ("protocol: %s\ncapabilities:%s%s%s\n",
	         mate_monitor_info_has_protocol (info) ? "yes" : "no",
	         mate_monitor_info_has_capability (info, MATE_MONITOR_CAPABILITY_WALLPAPER) ? " wallpaper" : "",
	         mate_monitor_info_has_capability (info, MATE_MONITOR_CAPABILITY_PER_OUTPUT_WALLPAPER) ? " per-output" : "",
	         mate_monitor_info_has_capability (info, MATE_MONITOR_CAPABILITY_SPANNED_WALLPAPER) ? " spanned" : "");
	for (int i = 0; i < n; i++) {
		GdkMonitor *monitor = gdk_display_get_monitor (display, i);
		gchar *id = mate_monitor_info_get_id (info, monitor);
		gchar *name = mate_monitor_info_get_display_name (info, monitor);
		gchar *mirror = mate_monitor_info_get_mirror_of (info, monitor);
		GdkRectangle geometry, workarea;

		gdk_monitor_get_geometry (monitor, &geometry);
		mate_monitor_info_get_workarea (info, monitor, &workarea);
		g_print ("monitor %d\n"
		         "  id: %s\n  name: %s\n  internal: %s\n  primary: %s\n"
		         "  mirror of: %s\n"
		         "  geometry: %d,%d %dx%d\n  workarea: %d,%d %dx%d\n"
		         "  background: %s\n",
		         i, id, name,
		         mate_monitor_info_is_internal (info, monitor) ? "yes" : "no",
		         mate_monitor_info_is_primary (info, monitor) ? "yes" : "no",
		         mirror ? mirror : "-",
		         geometry.x, geometry.y, geometry.width, geometry.height,
		         workarea.x, workarea.y, workarea.width, workarea.height,
		         owner_name (mate_monitor_info_get_background_owner (info, monitor)));
		g_free (id);
		g_free (name);
		g_free (mirror);
	}
}

static void
on_changed (MateMonitorInfo *info, GdkMonitor *monitor, gpointer data)
{
	g_print ("--- changed\n");
	print_all (info);
}

int
main (int argc, char **argv)
{
	gboolean watch = argc > 1 && g_strcmp0 (argv[1], "--watch") == 0;
	MateMonitorInfo *info;

	gtk_init (&argc, &argv);
	info = mate_monitor_info_get_default ();
	print_all (info);
	if (!watch)
		return 0;
	g_signal_connect (info, "changed", G_CALLBACK (on_changed), NULL);
	gtk_main ();
	return 0;
}
