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

#if GTK_CHECK_VERSION (3, 0, 0)
static void
gtk_style_shade (GdkRGBA *a,
                 GdkRGBA *b,
                 gdouble  k);

static void
rgb_to_hls (gdouble *r,
            gdouble *g,
            gdouble *b);

static void
hls_to_rgb (gdouble *h,
            gdouble *l,
            gdouble *s);
#endif

/**
 * mate_desktop_prepend_terminal_to_vector:
 * @argc: a pointer to the vector size
 * @argv: a pointer to the vector
 *
 * Prepends a terminal (either the one configured as default in the user's
 * MATE setup, or one of the common xterm emulators) to the passed in vector,
 * modifying it in the process.  The vector should be allocated with #g_malloc,
 * as this will #g_free the original vector.  Also all elements must have been
 * allocated separately.  That is the standard glib/MATE way of doing vectors
 * however.  If the integer that @argc points to is negative, the size will
 * first be computed.  Also note that passing in pointers to a vector that is
 * empty, will just create a new vector for you.
 **/
void
mate_desktop_prepend_terminal_to_vector (int *argc, char ***argv)
{
        char **real_argv;
        int real_argc;
        int i, j;
	char **term_argv = NULL;
	int term_argc = 0;
	GSettings *settings;

	gchar *terminal = NULL;

	char **the_argv;

        g_return_if_fail (argc != NULL);
        g_return_if_fail (argv != NULL);

        _mate_desktop_init_i18n ();

	/* sanity */
        if(*argv == NULL)
                *argc = 0;

	the_argv = *argv;

	/* compute size if not given */
	if (*argc < 0) {
		for (i = 0; the_argv[i] != NULL; i++)
			;
		*argc = i;
	}

	settings = g_settings_new ("org.mate.applications-terminal");
	terminal = g_settings_get_string (settings, "exec");

	if (terminal) {
		gchar *command_line;
		gchar *exec_flag;
		exec_flag = g_settings_get_string (settings, "exec-arg");

		if (exec_flag == NULL)
			command_line = g_strdup (terminal);
		else
			command_line = g_strdup_printf ("%s %s", terminal,
							exec_flag);

		g_shell_parse_argv (command_line,
				    &term_argc,
				    &term_argv,
				    NULL /* error */);

		g_free (command_line);
		g_free (exec_flag);
		g_free (terminal);
	}
	g_object_unref (settings);

	if (term_argv == NULL) {
		char *check;

		term_argc = 2;
		term_argv = g_new0 (char *, 3);

		check = g_find_program_in_path ("mate-terminal");
		if (check != NULL) {
			term_argv[0] = check;
			/* Note that mate-terminal takes -x and
			 * as -e in mate-terminal is broken we use that. */
			term_argv[1] = g_strdup ("-x");
		} else {
			if (check == NULL)
				check = g_find_program_in_path ("nxterm");
			if (check == NULL)
				check = g_find_program_in_path ("color-xterm");
			if (check == NULL)
				check = g_find_program_in_path ("rxvt");
			if (check == NULL)
				check = g_find_program_in_path ("xterm");
			if (check == NULL)
				check = g_find_program_in_path ("dtterm");
			if (check == NULL) {
				g_warning (_("Cannot find a terminal, using "
					     "xterm, even if it may not work"));
				check = g_strdup ("xterm");
			}
			term_argv[0] = check;
			term_argv[1] = g_strdup ("-e");
		}
	}

        real_argc = term_argc + *argc;
        real_argv = g_new (char *, real_argc + 1);

        for (i = 0; i < term_argc; i++)
                real_argv[i] = term_argv[i];

        for (j = 0; j < *argc; j++, i++)
                real_argv[i] = (char *)the_argv[j];

	real_argv[i] = NULL;

	g_free (*argv);
	*argv = real_argv;
	*argc = real_argc;

	/* we use g_free here as we sucked all the inner strings
	 * out from it into real_argv */
	g_free (term_argv);
}

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


#if GTK_CHECK_VERSION (3, 0, 0)

/**
 * gtk_style_shade:
 * @a:  the starting colour
 * @b:  [out] the resulting colour
 * @k:  amount to scale lightness and saturation by
 *
 * Takes a colour "a", scales the lightness and saturation by a certain amount,
 * and sets "b" to the resulting colour.
 * gtkstyle.c cut-and-pastage.
 */
static void
gtk_style_shade (GdkRGBA *a,
                 GdkRGBA *b,
                 gdouble  k)
{
	gdouble red;
	gdouble green;
	gdouble blue;

	red = a->red;
	green = a->green;
	blue = a->blue;

	rgb_to_hls (&red, &green, &blue);

	green *= k;
	if (green > 1.0)
		green = 1.0;
	else if (green < 0.0)
		green = 0.0;

	blue *= k;
	if (blue > 1.0)
		blue = 1.0;
	else if (blue < 0.0)
		blue = 0.0;

	hls_to_rgb (&red, &green, &blue);

	b->red = red;
	b->green = green;
	b->blue = blue;
}

/**
 * rgb_to_hls:
 * @r:  on input, red; on output, hue
 * @g:  on input, green; on output, lightness
 * @b:  on input, blue; on output, saturation
 *
 * Converts a red/green/blue triplet to a hue/lightness/saturation triplet.
 */
static void
rgb_to_hls (gdouble *r,
            gdouble *g,
            gdouble *b)
{
	gdouble min;
	gdouble max;
	gdouble red;
	gdouble green;
	gdouble blue;
	gdouble h, l, s;
	gdouble delta;

	red = *r;
	green = *g;
	blue = *b;

	if (red > green)
	{
		if (red > blue)
			max = red;
		else
			max = blue;

		if (green < blue)
			min = green;
		else
			min = blue;
	}
	else
	{
		if (green > blue)
			max = green;
		else
			max = blue;

		if (red < blue)
			min = red;
		else
			min = blue;
	}

	l = (max + min) / 2;
	s = 0;
	h = 0;

	if (max != min)
	{
		if (l <= 0.5)
			s = (max - min) / (max + min);
		else
			s = (max - min) / (2 - max - min);

		delta = max -min;
		if (red == max)
			h = (green - blue) / delta;
		else if (green == max)
			h = 2 + (blue - red) / delta;
		else if (blue == max)
			h = 4 + (red - green) / delta;

		h *= 60;
		if (h < 0.0)
			h += 360;
	}

	*r = h;
	*g = l;
	*b = s;
}

/**
 * hls_to_rgb:
 * @h: on input, hue; on output, red
 * @l: on input, lightness; on output, green
 * @s  on input, saturation; on output, blue
 *
 * Converts a hue/lightness/saturation triplet to a red/green/blue triplet.
 */
static void
hls_to_rgb (gdouble *h,
            gdouble *l,
            gdouble *s)
{
	gdouble hue;
	gdouble lightness;
	gdouble saturation;
	gdouble m1, m2;
	gdouble r, g, b;

	lightness = *l;
	saturation = *s;

	if (lightness <= 0.5)
		m2 = lightness * (1 + saturation);
	else
		m2 = lightness + saturation - lightness * saturation;
	m1 = 2 * lightness - m2;

	if (saturation == 0)
	{
		*h = lightness;
		*l = lightness;
		*s = lightness;
	}
	else
	{
		hue = *h + 120;
		while (hue > 360)
			hue -= 360;
		while (hue < 0)
			hue += 360;

		if (hue < 60)
			r = m1 + (m2 - m1) * hue / 60;
		else if (hue < 180)
			r = m2;
		else if (hue < 240)
			r = m1 + (m2 - m1) * (240 - hue) / 60;
		else
			r = m1;

		hue = *h;
		while (hue > 360)
			hue -= 360;
		while (hue < 0)
			hue += 360;

		if (hue < 60)
			g = m1 + (m2 - m1) * hue / 60;
		else if (hue < 180)
			g = m2;
		else if (hue < 240)
			g = m1 + (m2 - m1) * (240 - hue) / 60;
		else
			g = m1;

		hue = *h - 120;
		while (hue > 360)
			hue -= 360;
		while (hue < 0)
			hue += 360;

		if (hue < 60)
			b = m1 + (m2 - m1) * hue / 60;
		else if (hue < 180)
			b = m2;
		else if (hue < 240)
			b = m1 + (m2 - m1) * (240 - hue) / 60;
		else
			b = m1;

		*h = r;
		*l = g;
		*s = b;
	}
}

/* Based on set_color() in gtkstyle.c */
#define LIGHTNESS_MULT 1.3
#define DARKNESS_MULT  0.7
void
mate_desktop_gtk_style_get_light_color (GtkStyleContext *style,
                                        GtkStateFlags    state,
                                        GdkRGBA         *color)
{
	gtk_style_context_get_background_color (style, state, color);
	gtk_style_shade (color, color, LIGHTNESS_MULT);
}

void
mate_desktop_gtk_style_get_dark_color (GtkStyleContext *style,
                                       GtkStateFlags    state,
                                       GdkRGBA         *color)
{
	gtk_style_context_get_background_color (style, state, color);
	gtk_style_shade (color, color, DARKNESS_MULT);
}
#endif
