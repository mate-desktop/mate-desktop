/* -*- Mode: C; c-set-style: linux indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-desktop-utils.c - Utilities for the MATE Desktop

   Copyright (C) 1998 Tom Tromey
   All rights reserved.

   This file is part of the Mate Library.

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.  */
/*
  @NOTATION@
 */

#include <config.h>
#include <glib.h>
#include <gio/gio.h>
#include <glib/gi18n-lib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#define MATE_DESKTOP_USE_UNSTABLE_API
#include <mate-desktop-utils.h>

#include "private.h"

/**
 * mate_gdk_spawn_command_line_on_screen:
 * @screen: a GdkScreen
 * @command: a command line
 * @error: return location for errors
 *
 * This is a replacement for gdk_spawn_command_line_on_screen, deprecated
 * in GDK 2.24 and removed in GDK 3.0.
 *
 * gdk_spawn_command_line_on_screen is like g_spawn_command_line_async(),
 * except the child process is spawned in such an environment that on
 * calling gdk_display_open() it would be returned a GdkDisplay with
 * screen as the default screen.
 *
 * This is useful for applications which wish to launch an application
 * on a specific screen.
 *
 * Returns: TRUE on success, FALSE if error is set.
 *
 * Since: 1.7.1
 **/
gboolean
mate_gdk_spawn_command_line_on_screen (GdkScreen *screen, const gchar *command, GError **error)
{
	GAppInfo *appinfo = NULL;
	GdkAppLaunchContext *context = NULL;
	gboolean res = FALSE;

	appinfo = g_app_info_create_from_commandline (command, NULL, G_APP_INFO_CREATE_NONE, error);

	if (appinfo) {
#if GTK_CHECK_VERSION (3, 0, 0)
		context = gdk_display_get_app_launch_context (gdk_screen_get_display (screen));
#else
		/* Deprecated in GDK 3.0 */
		context = gdk_app_launch_context_new ();
		gdk_app_launch_context_set_screen (context, screen);
#endif
		res = g_app_info_launch (appinfo, NULL, G_APP_LAUNCH_CONTEXT (context), error);
		g_object_unref (context);
		g_object_unref (appinfo);
	}

	return res;
}

void
_mate_desktop_init_i18n (void) {
	static gboolean initialized = FALSE;

	if (!initialized) {
		bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
		bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
		initialized = TRUE;
	}
}

