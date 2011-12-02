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
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */
/*
  @NOTATION@
 */

#include <config.h>
#include <glib.h>
#include <mateconf/mateconf-client.h>
#include <glib/gi18n-lib.h>

#define MATE_DESKTOP_USE_UNSTABLE_API
#include <libmate/mate-desktop-utils.h>

#include "private.h"

/**
 * mate_desktop_prepend_terminal_to_vector:
 * @argc: a pointer to the vector size
 * @argv: a pointer to the vector
 *
 * Description:  Prepends a terminal (either the one configured as default in
 * the user's MATE setup, or one of the common xterm emulators) to the passed
 * in vector, modifying it in the process.  The vector should be allocated with
 * #g_malloc, as this will #g_free the original vector.  Also all elements must
 * have been allocated separately.  That is the standard glib/MATE way of
 * doing vectors however.  If the integer that @argc points to is negative, the
 * size will first be computed.  Also note that passing in pointers to a vector
 * that is empty, will just create a new vector for you.
 **/
void
mate_desktop_prepend_terminal_to_vector (int *argc, char ***argv)
{
#ifndef G_OS_WIN32
        char **real_argv;
        int real_argc;
        int i, j;
	char **term_argv = NULL;
	int term_argc = 0;
	MateConfClient *client;

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

	client = mateconf_client_get_default ();
	terminal = mateconf_client_get_string (client, "/desktop/mate/applications/terminal/exec", NULL);
	g_object_unref (client);
	
	if (terminal) {
		gchar *command_line;
		gchar *exec_flag;
		exec_flag = mateconf_client_get_string (client, "/desktop/mate/applications/terminal/exec_arg", NULL);

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
#else
	/* FIXME: Implement when needed */
	g_warning ("mate_prepend_terminal_to_vector: Not implemented");
#endif
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

