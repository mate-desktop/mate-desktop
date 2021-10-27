/* -*- Mode: C; c-set-style: gnu indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <string.h>
#include <unistd.h>

#include <mate-desktop-item.h>

#include <locale.h>
#include <stdlib.h>

static void
test_ditem (const char *file)
{
	MateDesktopItem *ditem;
	MateDesktopItemType type;
	const gchar *text;
	char *uri;
	char path[256];

	ditem = mate_desktop_item_new_from_file (file,
						  MATE_DESKTOP_ITEM_LOAD_ONLY_IF_EXISTS,
						  NULL);
	if (ditem == NULL) {
		g_print ("File %s is not an existing ditem\n", file);
		return;
	}

	text = mate_desktop_item_get_location (ditem);
	g_print ("LOCATION: |%s|\n", text);

	type = mate_desktop_item_get_entry_type (ditem);
	g_print ("TYPE: |%u|\n", type);

	text = mate_desktop_item_get_string
		(ditem, MATE_DESKTOP_ITEM_TYPE);
	g_print ("TYPE(string): |%s|\n", text);

	text = mate_desktop_item_get_string
		(ditem, MATE_DESKTOP_ITEM_EXEC);
	g_print ("EXEC: |%s|\n", text);

	text = mate_desktop_item_get_string
		(ditem, MATE_DESKTOP_ITEM_ICON);
	g_print ("ICON: |%s|\n", text);

	text = mate_desktop_item_get_localestring
		(ditem, MATE_DESKTOP_ITEM_NAME);
	g_print ("NAME: |%s|\n", text);

	text = mate_desktop_item_get_localestring_lang
		(ditem, MATE_DESKTOP_ITEM_NAME,
		 "cs_CZ");
	g_print ("NAME(lang=cs_CZ): |%s|\n", text);

	text = mate_desktop_item_get_localestring_lang
		(ditem, MATE_DESKTOP_ITEM_NAME,
		 "de");
	g_print ("NAME(lang=de): |%s|\n", text);

	text = mate_desktop_item_get_localestring_lang
		(ditem, MATE_DESKTOP_ITEM_NAME,
		 NULL);
	g_print ("NAME(lang=null): |%s|\n", text);

	text = mate_desktop_item_get_localestring
		(ditem, MATE_DESKTOP_ITEM_COMMENT);
	g_print ("COMMENT: |%s|\n", text);

	g_print ("Setting Name[de]=Neu gestzt! (I have no idea what that means)\n");
	mate_desktop_item_set_localestring
		(ditem,
		 MATE_DESKTOP_ITEM_NAME,
		 "Neu gesetzt!");

	getcwd (path, 255 - strlen ("/foo.desktop"));
	g_strlcat (path, "/foo.desktop", sizeof (path));

	g_print ("Saving to foo.desktop\n");
	uri = g_filename_to_uri (path, NULL, NULL);
	g_print ("URI: %s\n", uri);
	mate_desktop_item_save (ditem, uri, FALSE, NULL);
	g_free (uri);
}

static void
launch_item (const char *file)
{
	MateDesktopItem *ditem;
	GList *file_list = NULL;
	int ret;

	ditem = mate_desktop_item_new_from_file (file,
						  MATE_DESKTOP_ITEM_LOAD_ONLY_IF_EXISTS,
						  NULL);
	if (ditem == NULL) {
		g_print ("File %s is not an existing ditem\n", file);
		return;

	}

#if 0
	file_list = g_list_append (NULL, "file:///bin/sh");
	file_list = g_list_append (file_list, "foo");
	file_list = g_list_append (file_list, "bar");
	file_list = g_list_append (file_list, "http://slashdot.org");
#endif

	ret = mate_desktop_item_launch (ditem, file_list, 0, NULL);
	g_print ("launch returned: %d\n", ret);
}

int
main (int argc, char **argv)
{
	char *file;
	gboolean launch = FALSE;

	if (argc < 2 || argc > 3) {
		fprintf (stderr, "Usage: test-ditem path [LAUNCH]\n");
		exit (1);
	}

	if (argc == 3 &&
	    strcmp (argv[2], "LAUNCH") == 0)
		launch = TRUE;

	file = g_strdup (argv[1]);

	gtk_init (&argc, &argv);

	if (launch)
		launch_item (file);
	else
		test_ditem (file);

	/*
	test_ditem_edit (file);
	*/

	return 0;
}
