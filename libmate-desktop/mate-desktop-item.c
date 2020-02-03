/* -*- Mode: C; c-set-style: linux indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-desktop-item.c - MATE Desktop File Representation

   Copyright (C) 1999, 2000 Red Hat Inc.
   Copyright (C) 2001 Sid Vicious
   All rights reserved.

   This file is part of the Mate Library.

   Developed by Elliot Lee <sopwith@redhat.com> and Sid Vicious

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

#include "config.h"

#include <limits.h>
#include <ctype.h>
#include <stdio.h>
#include <glib.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <glib/gi18n-lib.h>
#include <locale.h>
#include <stdlib.h>

#include <gio/gio.h>

#ifdef HAVE_STARTUP_NOTIFICATION
#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#endif

#define sure_string(s) ((s)!=NULL?(s):"")

#define MATE_DESKTOP_USE_UNSTABLE_API
#undef MATE_DISABLE_DEPRECATED
#include <mate-desktop-item.h>
#include <mate-desktop-utils.h>

#include "private.h"

struct _MateDesktopItem {
	int refcount;

	/* all languages used */
	GList *languages;

	MateDesktopItemType type;

	/* `modified' means that the ditem has been
	 * modified since the last save. */
	gboolean modified;

	/* Keys of the main section only */
	GList *keys;

	GList *sections;

	/* This includes ALL keys, including
	 * other sections, separated by '/' */
	GHashTable *main_hash;

	char *location;

	time_t mtime;

	guint32 launch_time;
};

/* If mtime is set to this, set_location won't update mtime,
 * this is to be used internally only. */
#define DONT_UPDATE_MTIME ((time_t)-2)

typedef struct {
	char *name;
	GList *keys;
} Section;

typedef enum {
	ENCODING_UNKNOWN,
	ENCODING_UTF8,
	ENCODING_LEGACY_MIXED
} Encoding;

/*
 * IO reading utils, that look like the libc buffered io stuff
 */

#define READ_BUF_SIZE (32 * 1024)

typedef struct {
	GFile *file;
	GFileInputStream *stream;
	char *uri;
	char *buf;
	gboolean buf_needs_free;
	gboolean past_first_read;
	gboolean eof;
	guint64 size;
	gsize pos;
} ReadBuf;

static MateDesktopItem *ditem_load (ReadBuf           *rb,
				     gboolean           no_translations,
				     GError           **error);
static gboolean          ditem_save (MateDesktopItem  *item,
				     const char        *uri,
				     GError           **error);

static void mate_desktop_item_set_location_gfile (MateDesktopItem *item,
						   GFile            *file);

static MateDesktopItem *mate_desktop_item_new_from_gfile (GFile *file,
							    MateDesktopItemLoadFlags flags,
							    GError **error);

static int
readbuf_getc (ReadBuf *rb)
{
	if (rb->eof)
		return EOF;

	if (rb->size == 0 ||
	    rb->pos == rb->size) {
		gssize bytes_read;

		if (rb->stream == NULL)
			bytes_read = 0;
		else
			bytes_read = g_input_stream_read (G_INPUT_STREAM (rb->stream),
							  rb->buf,
							  READ_BUF_SIZE,
							  NULL, NULL);

		/* FIXME: handle errors other than EOF */
		if (bytes_read <= 0) {
			rb->eof = TRUE;
			return EOF;
		}

		if (rb->size != 0)
			rb->past_first_read = TRUE;
		rb->size = bytes_read;
		rb->pos = 0;

	}

	return (guchar) rb->buf[rb->pos++];
}

/* Note, does not include the trailing \n */
static char *
readbuf_gets (char *buf, gsize bufsize, ReadBuf *rb)
{
	int c;
	gsize pos;

	g_return_val_if_fail (buf != NULL, NULL);
	g_return_val_if_fail (rb != NULL, NULL);

	pos = 0;
	buf[0] = '\0';

	do {
		c = readbuf_getc (rb);
		if (c == EOF || c == '\n')
			break;
		buf[pos++] = c;
	} while (pos < bufsize-1);

	if (c == EOF && pos == 0)
		return NULL;

	buf[pos++] = '\0';

	return buf;
}

static ReadBuf *
readbuf_open (GFile *file, GError **error)
{
	GError *local_error;
	GFileInputStream *stream;
	char *uri;
	ReadBuf *rb;

	g_return_val_if_fail (file != NULL, NULL);

	uri = g_file_get_uri (file);
	local_error = NULL;
	stream = g_file_read (file, NULL, &local_error);

	if (stream == NULL) {
		g_set_error (error,
			     /* FIXME: better errors */
			     MATE_DESKTOP_ITEM_ERROR,
			     MATE_DESKTOP_ITEM_ERROR_CANNOT_OPEN,
			     _("Error reading file '%s': %s"),
			     uri, local_error->message);
		g_error_free (local_error);
		g_free (uri);
		return NULL;
	}

	rb = g_new0 (ReadBuf, 1);
	rb->stream = stream;
	rb->file = g_file_dup (file);
	rb->uri = uri;
	rb->buf = g_malloc (READ_BUF_SIZE);
	rb->buf_needs_free = TRUE;
	/* rb->past_first_read = FALSE; */
	/* rb->eof = FALSE; */
	/* rb->size = 0; */
	/* rb->pos = 0; */

	return rb;
}

static ReadBuf *
readbuf_new_from_string (const char *uri, const char *string, gssize length)
{
	ReadBuf *rb;

	g_return_val_if_fail (string != NULL, NULL);
	g_return_val_if_fail (length >= 0, NULL);

	rb = g_new0 (ReadBuf, 1);
	/* rb->file = NULL; */
	/* rb->stream = NULL; */
	rb->uri = g_strdup (uri);
	rb->buf = (char *) string;
	/* rb->buf_needs_free = FALSE; */
	/* rb->past_first_read = FALSE; */
	/* rb->eof = FALSE; */
	rb->size = length;
	/* rb->pos = 0; */

	return rb;
}

static gboolean
readbuf_rewind (ReadBuf *rb, GError **error)
{
	GError *local_error;

	rb->eof = FALSE;
	rb->pos = 0;

	if (!rb->past_first_read)
		return TRUE;

	rb->size = 0;

	if (g_seekable_seek (G_SEEKABLE (rb->stream),
			     0, G_SEEK_SET, NULL, NULL))
		return TRUE;

	g_object_unref (rb->stream);
	local_error = NULL;
	rb->stream = g_file_read (rb->file, NULL, &local_error);

	if (rb->stream == NULL) {
		g_set_error (
			error, MATE_DESKTOP_ITEM_ERROR,
			MATE_DESKTOP_ITEM_ERROR_CANNOT_OPEN,
			_("Error rewinding file '%s': %s"),
			rb->uri, local_error->message);
		g_error_free (local_error);

		return FALSE;
	}

	return TRUE;
}

static void
readbuf_close (ReadBuf *rb)
{
	if (rb->stream != NULL)
		g_object_unref (rb->stream);
	if (rb->file != NULL)
		g_object_unref (rb->file);
	g_free (rb->uri);
	if (rb->buf_needs_free)
		g_free (rb->buf);
	g_free (rb);
}

static MateDesktopItemType
type_from_string (const char *type)
{
	if (!type)
		return MATE_DESKTOP_ITEM_TYPE_NULL;

	switch (type [0]) {
	case 'A':
		if (!strcmp (type, "Application"))
			return MATE_DESKTOP_ITEM_TYPE_APPLICATION;
		break;
	case 'L':
		if (!strcmp (type, "Link"))
			return MATE_DESKTOP_ITEM_TYPE_LINK;
		break;
	case 'F':
		if (!strcmp (type, "FSDevice"))
			return MATE_DESKTOP_ITEM_TYPE_FSDEVICE;
		break;
	case 'M':
		if (!strcmp (type, "MimeType"))
			return MATE_DESKTOP_ITEM_TYPE_MIME_TYPE;
		break;
	case 'D':
		if (!strcmp (type, "Directory"))
			return MATE_DESKTOP_ITEM_TYPE_DIRECTORY;
		break;
	case 'S':
		if (!strcmp (type, "Service"))
			return MATE_DESKTOP_ITEM_TYPE_SERVICE;

		else if (!strcmp (type, "ServiceType"))
			return MATE_DESKTOP_ITEM_TYPE_SERVICE_TYPE;
		break;
	default:
		break;
	}

	return MATE_DESKTOP_ITEM_TYPE_OTHER;
}

/**
 * mate_desktop_item_new:
 *
 * Creates a MateDesktopItem object. The reference count on the returned value is set to '1'.
 *
 * Returns: The new MateDesktopItem
 */
MateDesktopItem *
mate_desktop_item_new (void)
{
	MateDesktopItem *retval;

	_mate_desktop_init_i18n ();

	retval = g_new0 (MateDesktopItem, 1);

	retval->refcount++;

	retval->main_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
						   (GDestroyNotify) g_free,
						   (GDestroyNotify) g_free);

	/* These are guaranteed to be set */
	mate_desktop_item_set_string (retval,
				       MATE_DESKTOP_ITEM_NAME,
				       /* Translators: the "name" mentioned
					* here is the name of an application or
					* a document */
				       _("No name"));
	mate_desktop_item_set_string (retval,
				       MATE_DESKTOP_ITEM_ENCODING,
				       "UTF-8");
	mate_desktop_item_set_string (retval,
				       MATE_DESKTOP_ITEM_VERSION,
				       "1.0");

	retval->launch_time = 0;

	return retval;
}

static Section *
dup_section (Section *sec)
{
	GList *li;
	Section *retval = g_new0 (Section, 1);

	retval->name = g_strdup (sec->name);

	retval->keys = g_list_copy (sec->keys);
	for (li = retval->keys; li != NULL; li = li->next)
		li->data = g_strdup (li->data);

	return retval;
}

static void
copy_string_hash (gpointer key, gpointer value, gpointer user_data)
{
	GHashTable *copy = user_data;
	g_hash_table_replace (copy,
			      g_strdup (key),
			      g_strdup (value));
}


/**
 * mate_desktop_item_copy:
 * @item: The item to be copied
 *
 * Creates a copy of a MateDesktopItem.  The new copy has a refcount of 1.
 * Note: Section stack is NOT copied.
 *
 * Returns: The new copy
 */
MateDesktopItem *
mate_desktop_item_copy (const MateDesktopItem *item)
{
	GList *li;
	MateDesktopItem *retval;

	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (item->refcount > 0, NULL);

	retval = mate_desktop_item_new ();

	retval->type = item->type;
	retval->modified = item->modified;
	retval->location = g_strdup (item->location);
	retval->mtime = item->mtime;
	retval->launch_time = item->launch_time;

	/* Languages */
	retval->languages = g_list_copy (item->languages);
	for (li = retval->languages; li != NULL; li = li->next)
		li->data = g_strdup (li->data);

	/* Keys */
	retval->keys = g_list_copy (item->keys);
	for (li = retval->keys; li != NULL; li = li->next)
		li->data = g_strdup (li->data);

	/* Sections */
	retval->sections = g_list_copy (item->sections);
	for (li = retval->sections; li != NULL; li = li->next)
		li->data = dup_section (li->data);

	retval->main_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
						   (GDestroyNotify) g_free,
						   (GDestroyNotify) g_free);

	g_hash_table_foreach (item->main_hash,
			      copy_string_hash,
			      retval->main_hash);

	return retval;
}

static void
read_sort_order (MateDesktopItem *item, GFile *dir)
{
	GFile *child;
	char buf[BUFSIZ];
	GString *str;
	ReadBuf *rb;

	child = g_file_get_child (dir, ".order");

	rb = readbuf_open (child, NULL);
	g_object_unref (child);

	if (rb == NULL)
		return;

	str = NULL;
	while (readbuf_gets (buf, sizeof (buf), rb) != NULL) {
		if (str == NULL)
			str = g_string_new (buf);
		else
			g_string_append (str, buf);
		g_string_append_c (str, ';');
	}
	readbuf_close (rb);
	if (str != NULL) {
		mate_desktop_item_set_string (item, MATE_DESKTOP_ITEM_SORT_ORDER,
					       str->str);
		g_string_free (str, TRUE);
	}
}

static MateDesktopItem *
make_fake_directory (GFile *dir)
{
	MateDesktopItem *item;
	GFile *child;

	item = mate_desktop_item_new ();
	mate_desktop_item_set_entry_type (item,
					   MATE_DESKTOP_ITEM_TYPE_DIRECTORY);


	item->mtime = DONT_UPDATE_MTIME; /* it doesn't exist, we know that */
	child = g_file_get_child (dir, ".directory");
	mate_desktop_item_set_location_gfile (item, child);
	item->mtime = 0;
	g_object_unref (child);

	read_sort_order (item, dir);

	return item;
}

/**
 * mate_desktop_item_new_from_file:
 * @file: The filename or directory path to load the MateDesktopItem from
 * @flags: Flags to influence the loading process
 *
 * This function loads 'file' and turns it into a MateDesktopItem.
 *
 * Returns: The newly loaded item.
 */
MateDesktopItem *
mate_desktop_item_new_from_file (const char *file,
				  MateDesktopItemLoadFlags flags,
				  GError **error)
{
	MateDesktopItem *retval;
	GFile *gfile;

	g_return_val_if_fail (file != NULL, NULL);

	gfile = g_file_new_for_path (file);
	retval = mate_desktop_item_new_from_gfile (gfile, flags, error);
	g_object_unref (gfile);

	return retval;
}

/**
 * mate_desktop_item_new_from_uri:
 * @uri: URI to load the MateDesktopItem from
 * @flags: Flags to influence the loading process
 *
 * This function loads 'uri' and turns it into a MateDesktopItem.
 *
 * Returns: The newly loaded item.
 */
MateDesktopItem *
mate_desktop_item_new_from_uri (const char *uri,
				 MateDesktopItemLoadFlags flags,
				 GError **error)
{
	MateDesktopItem *retval;
	GFile *file;

	g_return_val_if_fail (uri != NULL, NULL);

	file = g_file_new_for_uri (uri);
	retval = mate_desktop_item_new_from_gfile (file, flags, error);
	g_object_unref (file);

	return retval;
}

static MateDesktopItem *
mate_desktop_item_new_from_gfile (GFile *file,
				   MateDesktopItemLoadFlags flags,
				   GError **error)
{
	MateDesktopItem *retval;
	GFile *subfn;
	GFileInfo *info;
	GFileType type;
	GFile *parent;
	time_t mtime = 0;
	ReadBuf *rb;

	g_return_val_if_fail (file != NULL, NULL);

	info = g_file_query_info (file,
			          G_FILE_ATTRIBUTE_STANDARD_TYPE","G_FILE_ATTRIBUTE_TIME_MODIFIED,
				  G_FILE_QUERY_INFO_NONE, NULL, error);
	if (info == NULL)
		return NULL;

	type = g_file_info_get_file_type (info);

	if (type != G_FILE_TYPE_REGULAR && type != G_FILE_TYPE_DIRECTORY) {
		char *uri;

		uri = g_file_get_uri (file);
		g_set_error (error,
			     /* FIXME: better errors */
			     MATE_DESKTOP_ITEM_ERROR,
			     MATE_DESKTOP_ITEM_ERROR_INVALID_TYPE,
			     _("File '%s' is not a regular file or directory."),
			     uri);

		g_free (uri);
		g_object_unref (info);

		return NULL;
	}

	mtime = g_file_info_get_attribute_uint64 (info,
						  G_FILE_ATTRIBUTE_TIME_MODIFIED);

	g_object_unref (info);

	if (type == G_FILE_TYPE_DIRECTORY) {
		GFile *child;
		GFileInfo *child_info;

		child = g_file_get_child (file, ".directory");
		child_info = g_file_query_info (child,
						G_FILE_ATTRIBUTE_TIME_MODIFIED,
						G_FILE_QUERY_INFO_NONE,
						NULL, NULL);

		if (child_info == NULL) {
			g_object_unref (child);

			if (flags & MATE_DESKTOP_ITEM_LOAD_ONLY_IF_EXISTS) {
				return NULL;
			} else {
				return make_fake_directory (file);
			}
		}

		mtime = g_file_info_get_attribute_uint64 (child_info,
							  G_FILE_ATTRIBUTE_TIME_MODIFIED);
		g_object_unref (child_info);

		subfn = child;
	} else {
		subfn = g_file_dup (file);
	}

	rb = readbuf_open (subfn, error);

	if (rb == NULL) {
		g_object_unref (subfn);
		return NULL;
	}

	retval = ditem_load (rb,
			     (flags & MATE_DESKTOP_ITEM_LOAD_NO_TRANSLATIONS) != 0,
			     error);

	if (retval == NULL) {
		g_object_unref (subfn);
		return NULL;
	}

	if (flags & MATE_DESKTOP_ITEM_LOAD_ONLY_IF_EXISTS &&
	    ! mate_desktop_item_exists (retval)) {
		mate_desktop_item_unref (retval);
		g_object_unref (subfn);
		return NULL;
	}

	retval->mtime = DONT_UPDATE_MTIME;
	mate_desktop_item_set_location_gfile (retval, subfn);
	retval->mtime = mtime;

	parent = g_file_get_parent (file);
	if (parent != NULL) {
		read_sort_order (retval, parent);
		g_object_unref (parent);
	}

	g_object_unref (subfn);

	return retval;
}

/**
 * mate_desktop_item_new_from_string:
 * @string: string to load the MateDesktopItem from
 * @length: length of string, or -1 to use strlen
 * @flags: Flags to influence the loading process
 * @error: place to put errors
 *
 * This function turns the contents of the string into a MateDesktopItem.
 *
 * Returns: The newly loaded item.
 */
MateDesktopItem *
mate_desktop_item_new_from_string (const char *uri,
				    const char *string,
				    gssize length,
				    MateDesktopItemLoadFlags flags,
				    GError **error)
{
	MateDesktopItem *retval;
	ReadBuf *rb;

	g_return_val_if_fail (string != NULL, NULL);
	g_return_val_if_fail (length >= -1, NULL);

	if (length == -1) {
		length = strlen (string);
	}

	rb = readbuf_new_from_string (uri, string, length);

	retval = ditem_load (rb,
			     (flags & MATE_DESKTOP_ITEM_LOAD_NO_TRANSLATIONS) != 0,
			     error);

	if (retval == NULL) {
		return NULL;
	}

	/* FIXME: Sort order? */

	return retval;
}

static char *
lookup_desktop_file_in_data_dir (const char *desktop_file,
                                 const char *data_dir)
{
	char *path;

	path = g_build_filename (data_dir, "applications", desktop_file, NULL);
	if (!g_file_test (path, G_FILE_TEST_EXISTS)) {
		g_free (path);
		return NULL;
	}
	return path;
}

static char *
file_from_basename (const char *basename)
{
	const char * const *system_data_dirs;
	const char         *user_data_dir;
	char               *retval;
	int                 i;

	user_data_dir = g_get_user_data_dir ();
	system_data_dirs = g_get_system_data_dirs ();

	if ((retval = lookup_desktop_file_in_data_dir (basename, user_data_dir))) {
		return retval;
	}
	for (i = 0; system_data_dirs[i]; i++) {
		if ((retval = lookup_desktop_file_in_data_dir (basename, system_data_dirs[i]))) {
			return retval;
		}
	}
	return NULL;
}

/**
 * mate_desktop_item_new_from_basename:
 * @basename: The basename of the MateDesktopItem to load.
 * @flags: Flags to influence the loading process
 *
 * This function loads 'basename' from a system data directory and
 * returns its MateDesktopItem.
 *
 * Returns: The newly loaded item.
 */
MateDesktopItem *
mate_desktop_item_new_from_basename (const char *basename,
                                      MateDesktopItemLoadFlags flags,
                                      GError **error)
{
	MateDesktopItem *retval;
	char *file;

	g_return_val_if_fail (basename != NULL, NULL);

	if (!(file = file_from_basename (basename))) {
		g_set_error (error,
			     MATE_DESKTOP_ITEM_ERROR,
			     MATE_DESKTOP_ITEM_ERROR_CANNOT_OPEN,
			     _("Cannot find file '%s'"),
			     basename);
		return NULL;
	}

	retval = mate_desktop_item_new_from_file (file, flags, error);
	g_free (file);

	return retval;
}

/**
 * mate_desktop_item_save:
 * @item: A desktop item
 * @under: A new uri (location) for this #MateDesktopItem
 * @force: Save even if it wasn't modified
 * @error: #GError return
 *
 * Writes the specified item to disk.  If the 'under' is NULL, the original
 * location is used.  It sets the location of this entry to point to the
 * new location.
 *
 * Returns: boolean. %TRUE if the file was saved, %FALSE otherwise
 */
gboolean
mate_desktop_item_save (MateDesktopItem *item,
			 const char *under,
			 gboolean force,
			 GError **error)
{
	const char *uri;

	if (under == NULL &&
	    ! force &&
	    ! item->modified)
		return TRUE;

	if (under == NULL)
		uri = item->location;
	else
		uri = under;

	if (uri == NULL) {
		g_set_error (error,
			     MATE_DESKTOP_ITEM_ERROR,
			     MATE_DESKTOP_ITEM_ERROR_NO_FILENAME,
			     _("No filename to save to"));
		return FALSE;
	}

	if ( ! ditem_save (item, uri, error))
		return FALSE;

	item->modified = FALSE;
	item->mtime = time (NULL);

	return TRUE;
}

/**
 * mate_desktop_item_ref:
 * @item: A desktop item
 *
 * Description: Increases the reference count of the specified item.
 *
 * Returns: the newly referenced @item
 */
MateDesktopItem *
mate_desktop_item_ref (MateDesktopItem *item)
{
	g_return_val_if_fail (item != NULL, NULL);

	item->refcount++;

	return item;
}

static void
free_section (gpointer data, gpointer user_data)
{
	Section *section = data;

	g_free (section->name);
	section->name = NULL;

	g_list_foreach (section->keys, (GFunc)g_free, NULL);
	g_list_free (section->keys);
	section->keys = NULL;

	g_free (section);
}

/**
 * mate_desktop_item_unref:
 * @item: A desktop item
 *
 * Decreases the reference count of the specified item, and destroys the item if there are no more references left.
 */
void
mate_desktop_item_unref (MateDesktopItem *item)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);

	item->refcount--;

	if(item->refcount != 0)
		return;

	g_list_foreach (item->languages, (GFunc)g_free, NULL);
	g_list_free (item->languages);
	item->languages = NULL;

	g_list_foreach (item->keys, (GFunc)g_free, NULL);
	g_list_free (item->keys);
	item->keys = NULL;

	g_list_foreach (item->sections, free_section, NULL);
	g_list_free (item->sections);
	item->sections = NULL;

	g_hash_table_destroy (item->main_hash);
	item->main_hash = NULL;

	g_free (item->location);
	item->location = NULL;

	g_free (item);
}

static Section *
find_section (MateDesktopItem *item, const char *section)
{
	GList *li;
	Section *sec;

	if (section == NULL)
		return NULL;
	if (strcmp (section, "Desktop Entry") == 0)
		return NULL;

	for (li = item->sections; li != NULL; li = li->next) {
		sec = li->data;
		if (strcmp (sec->name, section) == 0)
			return sec;
	}

	sec = g_new0 (Section, 1);
	sec->name = g_strdup (section);
	sec->keys = NULL;

	item->sections = g_list_append (item->sections, sec);

	/* Don't mark the item modified, this is just an empty section,
	 * it won't be saved even */

	return sec;
}

static Section *
section_from_key (MateDesktopItem *item, const char *key)
{
	char *p;
	char *name;
	Section *sec;

	if (key == NULL)
		return NULL;

	p = strchr (key, '/');
	if (p == NULL)
		return NULL;

	name = g_strndup (key, p - key);

	sec = find_section (item, name);

	g_free (name);

	return sec;
}

static const char *
key_basename (const char *key)
{
	char *p = strrchr (key, '/');
	if (p != NULL)
		return p+1;
	else
		return key;
}


static const char *
lookup (const MateDesktopItem *item, const char *key)
{
	return g_hash_table_lookup (item->main_hash, key);
}

static const char *
lookup_locale (const MateDesktopItem *item, const char *key, const char *locale)
{
	if (locale == NULL ||
	    strcmp (locale, "C") == 0) {
		return lookup (item, key);
	} else {
		const char *ret;
		char *full = g_strdup_printf ("%s[%s]", key, locale);
		ret = lookup (item, full);
		g_free (full);
		return ret;
	}
}

static const char *
lookup_best_locale (const MateDesktopItem *item, const char *key)
{
	const char * const *langs_pointer;
	int                 i;

	langs_pointer = g_get_language_names ();
	for (i = 0; langs_pointer[i] != NULL; i++) {
		const char *ret = NULL;

		ret = lookup_locale (item, key, langs_pointer[i]);
		if (ret != NULL)
			return ret;
	}

	return NULL;
}

static void
set (MateDesktopItem *item, const char *key, const char *value)
{
	Section *sec = section_from_key (item, key);

	if (sec != NULL) {
		if (value != NULL) {
			if (g_hash_table_lookup (item->main_hash, key) == NULL)
				sec->keys = g_list_append
					(sec->keys,
					 g_strdup (key_basename (key)));

			g_hash_table_replace (item->main_hash,
					      g_strdup (key),
					      g_strdup (value));
		} else {
			GList *list = g_list_find_custom
				(sec->keys, key_basename (key),
				 (GCompareFunc)strcmp);
			if (list != NULL) {
				g_free (list->data);
				sec->keys =
					g_list_delete_link (sec->keys, list);
			}
			g_hash_table_remove (item->main_hash, key);
		}
	} else {
		if (value != NULL) {
			if (g_hash_table_lookup (item->main_hash, key) == NULL)
				item->keys = g_list_append (item->keys,
							    g_strdup (key));

			g_hash_table_replace (item->main_hash,
					      g_strdup (key),
					      g_strdup (value));
		} else {
			GList *list = g_list_find_custom
				(item->keys, key, (GCompareFunc)strcmp);
			if (list != NULL) {
				g_free (list->data);
				item->keys =
					g_list_delete_link (item->keys, list);
			}
			g_hash_table_remove (item->main_hash, key);
		}
	}
	item->modified = TRUE;
}

static void
set_locale (MateDesktopItem *item, const char *key,
	    const char *locale, const char *value)
{
	if (locale == NULL ||
	    strcmp (locale, "C") == 0) {
		set (item, key, value);
	} else {
		char *full = g_strdup_printf ("%s[%s]", key, locale);
		set (item, full, value);
		g_free (full);

		/* add the locale to the list of languages if it wasn't there
		 * before */
		if (g_list_find_custom (item->languages, locale,
					(GCompareFunc)strcmp) == NULL)
			item->languages = g_list_prepend (item->languages,
							  g_strdup (locale));
	}
}

static char **
list_to_vector (GSList *list)
{
	int len = g_slist_length (list);
	char **argv;
	int i;
	GSList *li;

	argv = g_new0 (char *, len+1);

	for (i = 0, li = list;
	     li != NULL;
	     li = li->next, i++) {
		argv[i] = g_strdup (li->data);
	}
	argv[i] = NULL;

	return argv;
}

static GSList *
make_args (GList *files)
{
	GSList *list = NULL;
	GList *li;

	for (li = files; li != NULL; li = li->next) {
		GFile *gfile;
		const char *file = li->data;
		if (file == NULL)
			continue;
		gfile = g_file_new_for_uri (file);
		list = g_slist_prepend (list, gfile);
	}

	return g_slist_reverse (list);
}

static void
free_args (GSList *list)
{
	GSList *li;

	for (li = list; li != NULL; li = li->next) {
		g_object_unref (G_FILE (li->data));
		li->data = NULL;
	}
	g_slist_free (list);
}

static char *
escape_single_quotes (const char *s,
		      gboolean in_single_quotes,
		      gboolean in_double_quotes)
{
	const char *p;
	GString *gs;
	const char *pre = "";
	const char *post = "";

	if ( ! in_single_quotes && ! in_double_quotes) {
		pre = "'";
		post = "'";
	} else if ( ! in_single_quotes && in_double_quotes) {
		pre = "\"'";
		post = "'\"";
	}

	if (strchr (s, '\'') == NULL) {
		return g_strconcat (pre, s, post, NULL);
	}

	gs = g_string_new (pre);

	for (p = s; *p != '\0'; p++) {
		if (*p == '\'')
			g_string_append (gs, "'\\''");
		else
			g_string_append_c (gs, *p);
	}

	g_string_append (gs, post);

	return g_string_free (gs, FALSE);
}

typedef enum {
	URI_TO_STRING,
	URI_TO_LOCAL_PATH,
	URI_TO_LOCAL_DIRNAME,
	URI_TO_LOCAL_BASENAME
} ConversionType;

static char *
convert_uri (GFile          *file,
	     ConversionType  conversion)
{
	char *retval = NULL;

	switch (conversion) {
	case URI_TO_STRING:
		retval = g_file_get_uri (file);
		break;
	case URI_TO_LOCAL_PATH:
		retval = g_file_get_path (file);
		break;
	case URI_TO_LOCAL_DIRNAME:
		{
			char *local_path;

			local_path = g_file_get_path (file);
			retval = g_path_get_dirname (local_path);
			g_free (local_path);
		}
		break;
	case URI_TO_LOCAL_BASENAME:
		retval = g_file_get_basename (file);
		break;
	default:
		g_assert_not_reached ();
	}

	return retval;
}

typedef enum {
	ADDED_NONE = 0,
	ADDED_SINGLE,
	ADDED_ALL
} AddedStatus;

static AddedStatus
append_all_converted (GString        *str,
		      ConversionType  conversion,
		      GSList         *args,
		      gboolean        in_single_quotes,
		      gboolean        in_double_quotes,
		      AddedStatus     added_status)
{
	GSList *l;

	for (l = args; l; l = l->next) {
		char *converted;
		char *escaped;

		if (!(converted = convert_uri (l->data, conversion)))
			continue;

		g_string_append (str, " ");

		escaped = escape_single_quotes (converted,
						in_single_quotes,
						in_double_quotes);
		g_string_append (str, escaped);

		g_free (escaped);
		g_free (converted);
	}

	return ADDED_ALL;
}

static AddedStatus
append_first_converted (GString         *str,
			ConversionType   conversion,
			GSList         **arg_ptr,
			gboolean         in_single_quotes,
			gboolean         in_double_quotes,
			AddedStatus      added_status)
{
	GSList *l;
	char   *converted = NULL;
	char   *escaped;

	for (l = *arg_ptr; l; l = l->next) {
		if ((converted = convert_uri (l->data, conversion)))
			break;

		*arg_ptr = l->next;
	}

	if (!converted)
		return added_status;

	escaped = escape_single_quotes (converted, in_single_quotes, in_double_quotes);
	g_string_append (str, escaped);
	g_free (escaped);
	g_free (converted);

	return added_status != ADDED_ALL ? ADDED_SINGLE : added_status;
}

static gboolean
do_percent_subst (const MateDesktopItem  *item,
		  const char              *arg,
		  GString                 *str,
		  gboolean                 in_single_quotes,
		  gboolean                 in_double_quotes,
		  GSList                  *args,
		  GSList                 **arg_ptr,
		  AddedStatus             *added_status)
{
	char *esc;
	const char *cs;

	if (arg[0] != '%' || arg[1] == '\0') {
		return FALSE;
	}

	switch (arg[1]) {
	case '%':
		g_string_append_c (str, '%');
		break;
	case 'U':
		*added_status = append_all_converted (str,
						      URI_TO_STRING,
						      args,
						      in_single_quotes,
						      in_double_quotes,
						      *added_status);
		break;
	case 'F':
		*added_status = append_all_converted (str,
						      URI_TO_LOCAL_PATH,
						      args,
						      in_single_quotes,
						      in_double_quotes,
						      *added_status);
		break;
	case 'N':
		*added_status = append_all_converted (str,
						      URI_TO_LOCAL_BASENAME,
						      args,
						      in_single_quotes,
						      in_double_quotes,
						      *added_status);
		break;
	case 'D':
		*added_status = append_all_converted (str,
						      URI_TO_LOCAL_DIRNAME,
						      args,
						      in_single_quotes,
						      in_double_quotes,
						      *added_status);
		break;
	case 'f':
		*added_status = append_first_converted (str,
							URI_TO_LOCAL_PATH,
							arg_ptr,
							in_single_quotes,
							in_double_quotes,
							*added_status);
		break;
	case 'u':
		*added_status = append_first_converted (str,
							URI_TO_STRING,
							arg_ptr,
							in_single_quotes,
							in_double_quotes,
							*added_status);
		break;
	case 'd':
		*added_status = append_first_converted (str,
							URI_TO_LOCAL_DIRNAME,
							arg_ptr,
							in_single_quotes,
							in_double_quotes,
							*added_status);
		break;
	case 'n':
		*added_status = append_first_converted (str,
							URI_TO_LOCAL_BASENAME,
							arg_ptr,
							in_single_quotes,
							in_double_quotes,
							*added_status);
		break;
	case 'm':
		/* Note: v0.9.4 of the spec says this is deprecated
		 * and replace with --miniicon iconname */
		cs = mate_desktop_item_get_string (item, MATE_DESKTOP_ITEM_MINI_ICON);
		if (cs != NULL) {
			g_string_append (str, "--miniicon=");
			esc = escape_single_quotes (cs, in_single_quotes, in_double_quotes);
			g_string_append (str, esc);
		}
		break;
	case 'i':
		/* Note: v0.9.4 of the spec says replace with --icon iconname */
		cs = mate_desktop_item_get_string (item, MATE_DESKTOP_ITEM_ICON);
		if (cs != NULL) {
			g_string_append (str, "--icon=");
			esc = escape_single_quotes (cs, in_single_quotes, in_double_quotes);
			g_string_append (str, esc);
		}
		break;
	case 'c':
		cs = mate_desktop_item_get_localestring (item, MATE_DESKTOP_ITEM_NAME);
		if (cs != NULL) {
			esc = escape_single_quotes (cs, in_single_quotes, in_double_quotes);
			g_string_append (str, esc);
			g_free (esc);
		}
		break;
	case 'k':
		if (item->location != NULL) {
			esc = escape_single_quotes (item->location, in_single_quotes, in_double_quotes);
			g_string_append (str, esc);
			g_free (esc);
		}
		break;
	case 'v':
		cs = mate_desktop_item_get_localestring (item, MATE_DESKTOP_ITEM_DEV);
		if (cs != NULL) {
			esc = escape_single_quotes (cs, in_single_quotes, in_double_quotes);
			g_string_append (str, esc);
			g_free (esc);
		}
		break;
	default:
		/* Maintain special characters - e.g. "%20" */
		if (g_ascii_isdigit (arg [1]))
			g_string_append_c (str, '%');
		return FALSE;
	}

	return TRUE;
}

static char *
expand_string (const MateDesktopItem  *item,
	       const char              *s,
	       GSList                  *args,
	       GSList                 **arg_ptr,
	       AddedStatus             *added_status)
{
	const char *p;
	gboolean escape = FALSE;
	gboolean single_quot = FALSE;
	gboolean double_quot = FALSE;
	GString *gs = g_string_new (NULL);

	for (p = s; *p != '\0'; p++) {
		if (escape) {
			escape = FALSE;
			g_string_append_c (gs, *p);
		} else if (*p == '\\') {
			if ( ! single_quot)
				escape = TRUE;
			g_string_append_c (gs, *p);
		} else if (*p == '\'') {
			g_string_append_c (gs, *p);
			if ( ! single_quot && ! double_quot) {
				single_quot = TRUE;
			} else if (single_quot) {
				single_quot = FALSE;
			}
		} else if (*p == '"') {
			g_string_append_c (gs, *p);
			if ( ! single_quot && ! double_quot) {
				double_quot = TRUE;
			} else if (double_quot) {
				double_quot = FALSE;
			}
		} else if (*p == '%') {
			if (do_percent_subst (item, p, gs,
					      single_quot, double_quot,
					      args, arg_ptr,
					      added_status)) {
				p++;
			}
		} else {
			g_string_append_c (gs, *p);
		}
	}
	return g_string_free (gs, FALSE);
}

#ifdef HAVE_STARTUP_NOTIFICATION
static void
sn_error_trap_push (SnDisplay *display,
		    Display   *xdisplay)
{
	GdkDisplay *gdkdisplay;

	gdkdisplay = gdk_display_get_default ();
	gdk_x11_display_error_trap_push (gdkdisplay);
}

static void
sn_error_trap_pop (SnDisplay *display,
		   Display   *xdisplay)
{
	GdkDisplay *gdkdisplay;

	gdkdisplay = gdk_display_get_default ();
	gdk_x11_display_error_trap_pop_ignored (gdkdisplay);
}

static char **
make_spawn_environment_for_sn_context (SnLauncherContext *sn_context,
				       char             **envp)
{
	char **retval;
	char **freeme;
	int    i, j;
	int    desktop_startup_id_len;

	retval = freeme = NULL;

	if (envp == NULL) {
		envp = freeme = g_listenv ();
		for (i = 0; envp[i]; i++) {
			char *name = envp[i];

			envp[i] = g_strjoin ("=", name, g_getenv (name), NULL);
			g_free (name);
		}
	} else {
		for (i = 0; envp[i]; i++)
			;
	}

	retval = g_new (char *, i + 2);

	desktop_startup_id_len = strlen ("DESKTOP_STARTUP_ID");

	for (i = 0, j = 0; envp[i]; i++) {
		if (strncmp (envp[i], "DESKTOP_STARTUP_ID", desktop_startup_id_len) != 0) {
			retval[j] = g_strdup (envp[i]);
			++j;
	        }
	}

	retval[j] = g_strdup_printf ("DESKTOP_STARTUP_ID=%s",
				     sn_launcher_context_get_startup_id (sn_context));
	++j;
	retval[j] = NULL;

	g_strfreev (freeme);

	return retval;
}

/* This should be fairly long, as it's confusing to users if a startup
 * ends when it shouldn't (it appears that the startup failed, and
 * they have to relaunch the app). Also the timeout only matters when
 * there are bugs and apps don't end their own startup sequence.
 *
 * This timeout is a "last resort" timeout that ignores whether the
 * startup sequence has shown activity or not.  Marco and the
 * tasklist have smarter, and correspondingly able-to-be-shorter
 * timeouts. The reason our timeout is dumb is that we don't monitor
 * the sequence (don't use an SnMonitorContext)
 */
#define STARTUP_TIMEOUT_LENGTH_SEC 30 /* seconds */
#define STARTUP_TIMEOUT_LENGTH (STARTUP_TIMEOUT_LENGTH_SEC * 1000)

typedef struct
{
	GdkScreen *screen;
	GSList *contexts;
	guint timeout_id;
} StartupTimeoutData;

static void
free_startup_timeout (void *data)
{
	StartupTimeoutData *std = data;

	g_slist_foreach (std->contexts,
			 (GFunc) sn_launcher_context_unref,
			 NULL);
	g_slist_free (std->contexts);

	if (std->timeout_id != 0) {
		g_source_remove (std->timeout_id);
		std->timeout_id = 0;
	}

	g_free (std);
}

static gboolean
startup_timeout (void *data)
{
	StartupTimeoutData *std = data;
	GSList *tmp;
	int min_timeout;

	min_timeout = STARTUP_TIMEOUT_LENGTH;

#if GLIB_CHECK_VERSION(2,61,2)
	gint64 now = g_get_real_time ();
#else
	GTimeVal now;
	g_get_current_time (&now);
#endif

	tmp = std->contexts;
	while (tmp != NULL) {
		SnLauncherContext *sn_context = tmp->data;
		GSList *next = tmp->next;
		double elapsed;

#if GLIB_CHECK_VERSION(2,61,2)
		time_t tv_sec;
		suseconds_t tv_usec;
		gint64 tv;

		sn_launcher_context_get_last_active_time (sn_context, &tv_sec, &tv_usec);
		tv = (tv_sec * G_USEC_PER_SEC) + tv_usec;
		elapsed = (double) (now - tv) / 1000.0;
#else
		long tv_sec, tv_usec;

		sn_launcher_context_get_last_active_time (sn_context,
							  &tv_sec, &tv_usec);

		elapsed =
			((((double)now.tv_sec - tv_sec) * G_USEC_PER_SEC +
			  (now.tv_usec - tv_usec))) / 1000.0;
#endif

		if (elapsed >= STARTUP_TIMEOUT_LENGTH) {
			std->contexts = g_slist_remove (std->contexts,
							sn_context);
			sn_launcher_context_complete (sn_context);
			sn_launcher_context_unref (sn_context);
		} else {
			min_timeout = MIN (min_timeout, (STARTUP_TIMEOUT_LENGTH - elapsed));
		}

		tmp = next;
	}

	/* we'll use seconds for the timeout */
	if (min_timeout < 1000)
		min_timeout = 1000;

	if (std->contexts == NULL) {
		std->timeout_id = 0;
	} else {
		std->timeout_id = g_timeout_add_seconds (min_timeout / 1000,
							 startup_timeout,
							 std);
	}

	/* always remove this one, but we may have reinstalled another one. */
	return FALSE;
}

static void
add_startup_timeout (GdkScreen         *screen,
		     SnLauncherContext *sn_context)
{
	StartupTimeoutData *data;

	data = g_object_get_data (G_OBJECT (screen), "mate-startup-data");
	if (data == NULL) {
		data = g_new (StartupTimeoutData, 1);
		data->screen = screen;
		data->contexts = NULL;
		data->timeout_id = 0;

		g_object_set_data_full (G_OBJECT (screen), "mate-startup-data",
					data, free_startup_timeout);
	}

	sn_launcher_context_ref (sn_context);
	data->contexts = g_slist_prepend (data->contexts, sn_context);

	if (data->timeout_id == 0) {
		data->timeout_id = g_timeout_add_seconds (
						STARTUP_TIMEOUT_LENGTH_SEC,
						startup_timeout,
						data);
	}
}
#endif /* HAVE_STARTUP_NOTIFICATION */

static inline char *
stringify_uris (GSList *args)
{
	GString *str;

	str = g_string_new (NULL);

	append_all_converted (str, URI_TO_STRING, args, FALSE, FALSE, ADDED_NONE);

	return g_string_free (str, FALSE);
}

static inline char *
stringify_files (GSList *args)
{
	GString *str;

	str = g_string_new (NULL);

	append_all_converted (str, URI_TO_LOCAL_PATH, args, FALSE, FALSE, ADDED_NONE);

	return g_string_free (str, FALSE);
}

static char **
make_environment_for_screen (GdkScreen  *screen,
			     char      **envp)
{
	GdkDisplay *display;
	char      **retval;
	char      **freeme;
	char       *display_name;
	int         display_index = -1;
	int         i, env_len;

	g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

	retval = freeme = NULL;

	if (envp == NULL) {
		envp = freeme = g_listenv ();
		for (i = 0; envp [i]; i++) {
			char *name = envp[i];

			envp[i] = g_strjoin ("=", name, g_getenv (name), NULL);
			g_free (name);
		}
	}

	for (env_len = 0; envp [env_len]; env_len++)
		if (strncmp (envp [env_len], "DISPLAY", strlen ("DISPLAY")) == 0)
			display_index = env_len;

	retval = g_new (char *, env_len + 1);
	retval [env_len] = NULL;

	display = gdk_screen_get_display (screen);
	display_name = g_strdup (gdk_display_get_name (display));

	for (i = 0; i < env_len; i++)
		if (i == display_index)
			retval [i] = g_strconcat ("DISPLAY=", display_name, NULL);
		else
			retval [i] = g_strdup (envp[i]);

	g_assert (i == env_len);

	g_free (display_name);
	g_strfreev (freeme);

	return retval;
}

static void
dummy_child_watch (GPid         pid,
		   gint         status,
		   gpointer user_data)
{
	/* Nothing, this is just to ensure we don't double fork
	 * and break pkexec:
	 * https://bugzilla.gnome.org/show_bug.cgi?id=675789
	 */
}

static int
ditem_execute (const MateDesktopItem *item,
	       const char *exec,
	       GList *file_list,
	       GdkScreen *screen,
	       int workspace,
               char **envp,
	       gboolean launch_only_one,
	       gboolean use_current_dir,
	       gboolean append_uris,
	       gboolean append_paths,
	       gboolean do_not_reap_child,
	       GError **error)
{
	char **free_me = NULL;
	char **real_argv;
	int i, ret;
	char **term_argv = NULL;
	int term_argc = 0;
	GSList *vector_list;
	GSList *args, *arg_ptr;
	AddedStatus added_status;
	const char *working_dir = NULL;
	char **temp_argv = NULL;
	int temp_argc = 0;
	char *new_exec, *uris, *temp;
	char *exec_locale;
	int launched = 0;
	GPid pid;
#ifdef HAVE_STARTUP_NOTIFICATION
	GdkDisplay *gdkdisplay;
	SnLauncherContext *sn_context;
	SnDisplay *sn_display;
	const char *startup_class;
#endif

	g_return_val_if_fail (item, -1);

	if (item->type == MATE_DESKTOP_ITEM_TYPE_APPLICATION) {
		working_dir = mate_desktop_item_get_string (item, MATE_DESKTOP_ITEM_PATH);
		if (working_dir &&
		    !g_file_test (working_dir, G_FILE_TEST_IS_DIR))
			working_dir = NULL;
	}

	if (working_dir == NULL && !use_current_dir)
		working_dir = g_get_home_dir ();

	if (mate_desktop_item_get_boolean (item, MATE_DESKTOP_ITEM_TERMINAL)) {
		const char *options =
			mate_desktop_item_get_string (item, MATE_DESKTOP_ITEM_TERMINAL_OPTIONS);

		if (options != NULL) {
			g_shell_parse_argv (options,
					    &term_argc,
					    &term_argv,
					    NULL /* error */);
			/* ignore errors */
		}

		mate_desktop_prepend_terminal_to_vector (&term_argc, &term_argv);
	}

	args = make_args (file_list);
	arg_ptr = make_args (file_list);

#ifdef HAVE_STARTUP_NOTIFICATION
	if (screen)
		gdkdisplay = gdk_screen_get_display (screen);
	else
		gdkdisplay = gdk_display_get_default ();

	sn_display = sn_display_new (GDK_DISPLAY_XDISPLAY (gdkdisplay),
				     sn_error_trap_push,
				     sn_error_trap_pop);


	/* Only initiate notification if desktop file supports it.
	 * (we could avoid setting up the SnLauncherContext if we aren't going
	 * to initiate, but why bother)
	 */

	startup_class = mate_desktop_item_get_string (item,
						       "StartupWMClass");
	if (startup_class ||
	    mate_desktop_item_get_boolean (item, "StartupNotify")) {
		const char *name;
		const char *icon;

		sn_context = sn_launcher_context_new (sn_display,
						      screen ? gdk_x11_screen_get_screen_number (screen) :
						      DefaultScreen (GDK_DISPLAY_XDISPLAY (gdkdisplay)));

		name = mate_desktop_item_get_localestring (item,
							    MATE_DESKTOP_ITEM_NAME);

		if (name == NULL)
			name = mate_desktop_item_get_localestring (item,
								    MATE_DESKTOP_ITEM_GENERIC_NAME);

		if (name != NULL) {
			char *description;

			sn_launcher_context_set_name (sn_context, name);

			description = g_strdup_printf (_("Starting %s"), name);

			sn_launcher_context_set_description (sn_context, description);

			g_free (description);
		}

		icon = mate_desktop_item_get_string (item,
						      MATE_DESKTOP_ITEM_ICON);

		if (icon != NULL)
			sn_launcher_context_set_icon_name (sn_context, icon);

		sn_launcher_context_set_workspace (sn_context, workspace);

		if (startup_class != NULL)
			sn_launcher_context_set_wmclass (sn_context,
							 startup_class);
	} else {
		sn_context = NULL;
	}
#endif

	if (screen) {
		envp = make_environment_for_screen (screen, envp);
		if (free_me)
			g_strfreev (free_me);
		free_me = envp;
	}

	exec_locale = g_filename_from_utf8 (exec, -1, NULL, NULL, NULL);

	if (exec_locale == NULL) {
		exec_locale = g_strdup ("");
	}

	do {
		added_status = ADDED_NONE;
		new_exec = expand_string (item,
					  exec_locale,
					  args, &arg_ptr, &added_status);

		if (launched == 0 && added_status == ADDED_NONE && append_uris) {
			uris = stringify_uris (args);
			temp = g_strconcat (new_exec, " ", uris, NULL);
			g_free (uris);
			g_free (new_exec);
			new_exec = temp;
			added_status = ADDED_ALL;
		}

		/* append_uris and append_paths are mutually exlusive */
		if (launched == 0 && added_status == ADDED_NONE && append_paths) {
			uris = stringify_files (args);
			temp = g_strconcat (new_exec, " ", uris, NULL);
			g_free (uris);
			g_free (new_exec);
			new_exec = temp;
			added_status = ADDED_ALL;
		}

		if (launched > 0 && added_status == ADDED_NONE) {
			g_free (new_exec);
			break;
		}

		if ( ! g_shell_parse_argv (new_exec,
					   &temp_argc, &temp_argv, error)) {
			/* The error now comes from g_shell_parse_argv */
			g_free (new_exec);
			ret = -1;
			break;
		}
		g_free (new_exec);

		vector_list = NULL;
		for(i = 0; i < term_argc; i++)
			vector_list = g_slist_append (vector_list,
						      g_strdup (term_argv[i]));

		for(i = 0; i < temp_argc; i++)
			vector_list = g_slist_append (vector_list,
						      g_strdup (temp_argv[i]));

		g_strfreev (temp_argv);

		real_argv = list_to_vector (vector_list);
		g_slist_foreach (vector_list, (GFunc)g_free, NULL);
		g_slist_free (vector_list);

#ifdef HAVE_STARTUP_NOTIFICATION
		if (sn_context != NULL &&
		    !sn_launcher_context_get_initiated (sn_context)) {
			guint32 launch_time;

			/* This means that we always use the first real_argv[0]
			 * we select for the "binary name", but it's probably
			 * OK to do that. Binary name isn't super-important
			 * anyway, and we can't initiate twice, and we
			 * must initiate prior to fork/exec.
			 */

			sn_launcher_context_set_binary_name (sn_context,
							     real_argv[0]);

			if (item->launch_time > 0)
				launch_time = item->launch_time;
			else
				launch_time = gdk_x11_display_get_user_time (gdkdisplay);

			sn_launcher_context_initiate (sn_context,
						      g_get_prgname () ? g_get_prgname () : "unknown",
						      real_argv[0],
						      launch_time);

			/* Don't allow accidental reuse of same timestamp */
			((MateDesktopItem *)item)->launch_time = 0;

			envp = make_spawn_environment_for_sn_context (sn_context, envp);
			if (free_me)
				g_strfreev (free_me);
			free_me = envp;
		}
#endif


		if ( ! g_spawn_async (working_dir,
				      real_argv,
				      envp,
				      (do_not_reap_child ? G_SPAWN_DO_NOT_REAP_CHILD : 0) | G_SPAWN_SEARCH_PATH /* flags */,
				      NULL, /* child_setup_func */
				      NULL, /* child_setup_func_data */
				      (do_not_reap_child ? &pid : NULL) /* child_pid */,
				      error)) {
			/* The error was set for us,
			 * we just can't launch this thingie */
			ret = -1;
			g_strfreev (real_argv);
			break;
		} else if (do_not_reap_child) {
			g_child_watch_add (pid, dummy_child_watch, NULL);
		}

		launched ++;

		g_strfreev (real_argv);

		if (arg_ptr != NULL)
			arg_ptr = arg_ptr->next;

	/* rinse, repeat until we run out of arguments (That
	 * is if we were adding singles anyway) */
	} while (added_status == ADDED_SINGLE &&
		 arg_ptr != NULL &&
		 ! launch_only_one);

	g_free (exec_locale);
#ifdef HAVE_STARTUP_NOTIFICATION
	if (sn_context != NULL) {
		if (ret < 0)
			sn_launcher_context_complete (sn_context); /* end sequence */
		else
			add_startup_timeout (screen ? screen :
					     gdk_display_get_default_screen (gdk_display_get_default ()),
					     sn_context);
		sn_launcher_context_unref (sn_context);
	}

	sn_display_unref (sn_display);
#endif /* HAVE_STARTUP_NOTIFICATION */

	free_args (args);

	if (term_argv)
		g_strfreev (term_argv);

	if (free_me)
		g_strfreev (free_me);

	return ret;
}

/* strip any trailing &, return FALSE if bad things happen and
   we end up with an empty string */
static gboolean
strip_the_amp (char *exec)
{
	size_t exec_len;

	g_strstrip (exec);
	if (*exec == '\0')
		return FALSE;

	exec_len = strlen (exec);
	/* kill any trailing '&' */
	if (exec[exec_len-1] == '&') {
		exec[exec_len-1] = '\0';
		g_strchomp (exec);
	}

	/* can't exactly launch an empty thing */
	if (*exec == '\0')
		return FALSE;

	return TRUE;
}


static int
mate_desktop_item_launch_on_screen_with_env (
		const MateDesktopItem       *item,
		GList                        *file_list,
		MateDesktopItemLaunchFlags   flags,
		GdkScreen                    *screen,
		int                           workspace,
		char                        **envp,
		GError                      **error)
{
	const char *exec;
	char *the_exec;
	int ret;

	exec = mate_desktop_item_get_string (item, MATE_DESKTOP_ITEM_EXEC);
	/* This is a URL, so launch it as a url */
	if (item->type == MATE_DESKTOP_ITEM_TYPE_LINK) {
		const char *url;
		gboolean    retval;

		url = mate_desktop_item_get_string (item, MATE_DESKTOP_ITEM_URL);
		/* Mate panel used to put this in Exec */
		if (!(url && url[0] != '\0'))
			url = exec;

		if (!(url && url[0] != '\0')) {
			g_set_error (error,
				     MATE_DESKTOP_ITEM_ERROR,
				     MATE_DESKTOP_ITEM_ERROR_NO_URL,
				     _("No URL to launch"));
			return -1;
		}

		retval = gtk_show_uri_on_window  (NULL,
		                                  url,
		                                  GDK_CURRENT_TIME,
		                                  error);
		return retval ? 0 : -1;
	}

	/* check the type, if there is one set */
	if (item->type != MATE_DESKTOP_ITEM_TYPE_APPLICATION) {
		g_set_error (error,
			     MATE_DESKTOP_ITEM_ERROR,
			     MATE_DESKTOP_ITEM_ERROR_NOT_LAUNCHABLE,
			     _("Not a launchable item"));
		return -1;
	}


	if (exec == NULL ||
	    exec[0] == '\0') {
		g_set_error (error,
			     MATE_DESKTOP_ITEM_ERROR,
			     MATE_DESKTOP_ITEM_ERROR_NO_EXEC_STRING,
			     _("No command (Exec) to launch"));
		return -1;
	}


	/* make a new copy and get rid of spaces */
	the_exec = g_alloca (strlen (exec) + 1);
	g_strlcpy (the_exec, exec, strlen (exec) + 1);

	if ( ! strip_the_amp (the_exec)) {
		g_set_error (error,
			     MATE_DESKTOP_ITEM_ERROR,
			     MATE_DESKTOP_ITEM_ERROR_BAD_EXEC_STRING,
			     _("Bad command (Exec) to launch"));
		return -1;
	}

	ret = ditem_execute (item, the_exec, file_list, screen, workspace, envp,
			     (flags & MATE_DESKTOP_ITEM_LAUNCH_ONLY_ONE),
			     (flags & MATE_DESKTOP_ITEM_LAUNCH_USE_CURRENT_DIR),
			     (flags & MATE_DESKTOP_ITEM_LAUNCH_APPEND_URIS),
			     (flags & MATE_DESKTOP_ITEM_LAUNCH_APPEND_PATHS),
			     (flags & MATE_DESKTOP_ITEM_LAUNCH_DO_NOT_REAP_CHILD),
			     error);

	return ret;
}

/**
 * mate_desktop_item_launch:
 * @item: A desktop item
 * @file_list:  Files/URIs to launch this item with, can be %NULL
 * @flags: FIXME
 * @error: FIXME
 *
 * This function runs the program listed in the specified 'item',
 * optionally appending additional arguments to its command line.  It uses
 * #g_shell_parse_argv to parse the the exec string into a vector which is
 * then passed to #g_spawn_async for execution. This can return all
 * the errors from MateURL, #g_shell_parse_argv and #g_spawn_async,
 * in addition to it's own.  The files are
 * only added if the entry defines one of the standard % strings in it's
 * Exec field.
 *
 * Returns: The the pid of the process spawned.  If more then one
 * process was spawned the last pid is returned.  On error -1
 * is returned and @error is set.
 */
int
mate_desktop_item_launch (const MateDesktopItem       *item,
			   GList                        *file_list,
			   MateDesktopItemLaunchFlags   flags,
			   GError                      **error)
{
	return mate_desktop_item_launch_on_screen_with_env (
			item, file_list, flags, NULL, -1, NULL, error);
}

/**
 * mate_desktop_item_launch_with_env:
 * @item: A desktop item
 * @file_list:  Files/URIs to launch this item with, can be %NULL
 * @flags: FIXME
 * @envp: child's environment, or %NULL to inherit parent's
 * @error: FIXME
 *
 * See mate_desktop_item_launch for a full description. This function
 * additionally passes an environment vector for the child process
 * which is to be launched.
 *
 * Returns: The the pid of the process spawned.  If more then one
 * process was spawned the last pid is returned.  On error -1
 * is returned and @error is set.
 */
int
mate_desktop_item_launch_with_env (const MateDesktopItem       *item,
				    GList                        *file_list,
				    MateDesktopItemLaunchFlags   flags,
				    char                        **envp,
				    GError                      **error)
{
	return mate_desktop_item_launch_on_screen_with_env (
			item, file_list, flags,
			NULL, -1, envp, error);
}

/**
 * mate_desktop_item_launch_on_screen:
 * @item: A desktop item
 * @file_list:  Files/URIs to launch this item with, can be %NULL
 * @flags: FIXME
 * @screen: the %GdkScreen on which the application should be launched
 * @workspace: the workspace on which the app should be launched (-1 for current)
 * @error: FIXME
 *
 * See mate_desktop_item_launch for a full description. This function
 * additionally attempts to launch the application on a given screen
 * and workspace.
 *
 * Returns: The the pid of the process spawned.  If more then one
 * process was spawned the last pid is returned.  On error -1
 * is returned and @error is set.
 */
int
mate_desktop_item_launch_on_screen (const MateDesktopItem       *item,
				     GList                        *file_list,
				     MateDesktopItemLaunchFlags   flags,
				     GdkScreen                    *screen,
				     int                           workspace,
				     GError                      **error)
{
	return mate_desktop_item_launch_on_screen_with_env (
			item, file_list, flags,
			screen, workspace, NULL, error);
}

/**
 * mate_desktop_item_drop_uri_list:
 * @item: A desktop item
 * @uri_list: text as gotten from a text/uri-list
 * @flags: FIXME
 * @error: FIXME
 *
 * A list of files or urls dropped onto an icon, the proper (Url or File)
 * exec is run you can pass directly string that you got as the
 * text/uri-list.  This just parses the list and calls
 *
 * Returns: The value returned by #mate_execute_async() upon execution of
 * the specified item or -1 on error.  If multiple instances are run, the
 * return of the last one is returned.
 */
int
mate_desktop_item_drop_uri_list (const MateDesktopItem *item,
				  const char *uri_list,
				  MateDesktopItemLaunchFlags flags,
				  GError **error)
{
	return mate_desktop_item_drop_uri_list_with_env (item, uri_list,
							  flags, NULL, error);
}

/**
* mate_desktop_item_drop_uri_list_with_env:
* @item: A desktop item
* @uri_list: text as gotten from a text/uri-list
* @flags: FIXME
* @envp: child's environment
* @error: FIXME
*
* See mate_desktop_item_drop_uri_list for a full description. This function
* additionally passes an environment vector for the child process
* which is to be launched.
*
* Returns: The value returned by #mate_execute_async() upon execution of
* the specified item or -1 on error.  If multiple instances are run, the
* return of the last one is returned.
*/
int
mate_desktop_item_drop_uri_list_with_env (const MateDesktopItem *item,
					   const char *uri_list,
					   MateDesktopItemLaunchFlags flags,
					   char                        **envp,
					   GError **error)
{
	int ret;
	char  *uri;
	char **uris;
	GList *list = NULL;

	uris = g_uri_list_extract_uris (uri_list);

	for (uri = uris[0]; uri != NULL; uri++) {
		list = g_list_prepend (list, uri);
	}
	list = g_list_reverse (list);

	ret =  mate_desktop_item_launch_with_env (
			item, list, flags, envp, error);

	g_strfreev (uris);
	g_list_free (list);

	return ret;
}

static gboolean
exec_exists (const char *exec)
{
	if (g_path_is_absolute (exec)) {
		if (access (exec, X_OK) == 0)
			return TRUE;
		else
			return FALSE;
	} else {
		char *tryme;

		tryme = g_find_program_in_path (exec);
		if (tryme != NULL) {
			g_free (tryme);
			return TRUE;
		}
		return FALSE;
	}
}

/**
 * mate_desktop_item_exists:
 * @item: A desktop item
 *
 * Attempt to figure out if the program that can be executed by this item
 * actually exists.  First it tries the TryExec attribute to see if that
 * contains a program that is in the path.  Then if there is no such
 * attribute, it tries the first word of the Exec attribute.
 *
 * Returns: A boolean, %TRUE if it exists, %FALSE otherwise.
 */
gboolean
mate_desktop_item_exists (const MateDesktopItem *item)
{
	const char *try_exec;
	const char *exec;

	g_return_val_if_fail (item != NULL, FALSE);

	try_exec = lookup (item, MATE_DESKTOP_ITEM_TRY_EXEC);

	if (try_exec != NULL &&
	    ! exec_exists (try_exec)) {
		return FALSE;
	}

	if (item->type == MATE_DESKTOP_ITEM_TYPE_APPLICATION) {
		int argc;
		char **argv;
		const char *exe;

		exec = lookup (item, MATE_DESKTOP_ITEM_EXEC);
		if (exec == NULL)
			return FALSE;

		if ( ! g_shell_parse_argv (exec, &argc, &argv, NULL))
			return FALSE;

		if (argc < 1) {
			g_strfreev (argv);
			return FALSE;
		}

		exe = argv[0];

		if ( ! exec_exists (exe)) {
			g_strfreev (argv);
			return FALSE;
		}
		g_strfreev (argv);
	}

	return TRUE;
}

/**
 * mate_desktop_item_get_entry_type:
 * @item: A desktop item
 *
 * Gets the type attribute (the 'Type' field) of the item.  This should
 * usually be 'Application' for an application, but it can be 'Directory'
 * for a directory description.  There are other types available as well.
 * The type usually indicates how the desktop item should be handeled and
 * how the 'Exec' field should be handeled.
 *
 * Returns: The type of the specified 'item'. The returned
 * memory remains owned by the MateDesktopItem and should not be freed.
 */
MateDesktopItemType
mate_desktop_item_get_entry_type (const MateDesktopItem *item)
{
	g_return_val_if_fail (item != NULL, 0);
	g_return_val_if_fail (item->refcount > 0, 0);

	return item->type;
}

void
mate_desktop_item_set_entry_type (MateDesktopItem *item,
				   MateDesktopItemType type)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);

	item->type = type;

	switch (type) {
	case MATE_DESKTOP_ITEM_TYPE_NULL:
		set (item, MATE_DESKTOP_ITEM_TYPE, NULL);
		break;
	case MATE_DESKTOP_ITEM_TYPE_APPLICATION:
		set (item, MATE_DESKTOP_ITEM_TYPE, "Application");
		break;
	case MATE_DESKTOP_ITEM_TYPE_LINK:
		set (item, MATE_DESKTOP_ITEM_TYPE, "Link");
		break;
	case MATE_DESKTOP_ITEM_TYPE_FSDEVICE:
		set (item, MATE_DESKTOP_ITEM_TYPE, "FSDevice");
		break;
	case MATE_DESKTOP_ITEM_TYPE_MIME_TYPE:
		set (item, MATE_DESKTOP_ITEM_TYPE, "MimeType");
		break;
	case MATE_DESKTOP_ITEM_TYPE_DIRECTORY:
		set (item, MATE_DESKTOP_ITEM_TYPE, "Directory");
		break;
	case MATE_DESKTOP_ITEM_TYPE_SERVICE:
		set (item, MATE_DESKTOP_ITEM_TYPE, "Service");
		break;
	case MATE_DESKTOP_ITEM_TYPE_SERVICE_TYPE:
		set (item, MATE_DESKTOP_ITEM_TYPE, "ServiceType");
		break;
	default:
		break;
	}
}



/**
 * mate_desktop_item_get_file_status:
 * @item: A desktop item
 *
 * This function checks the modification time of the on-disk file to
 * see if it is more recent than the in-memory data.
 *
 * Returns: An enum value that specifies whether the item has changed since being loaded.
 */
MateDesktopItemStatus
mate_desktop_item_get_file_status (const MateDesktopItem *item)
{
	MateDesktopItemStatus retval;
	GFile *file;
	GFileInfo *info;

	g_return_val_if_fail (item != NULL, MATE_DESKTOP_ITEM_DISAPPEARED);
	g_return_val_if_fail (item->refcount > 0, MATE_DESKTOP_ITEM_DISAPPEARED);

	if (item->location == NULL)
		return MATE_DESKTOP_ITEM_DISAPPEARED;

	file = g_file_new_for_uri (item->location);
	info = g_file_query_info (file, G_FILE_ATTRIBUTE_TIME_MODIFIED,
				  G_FILE_QUERY_INFO_NONE, NULL, NULL);

	retval = MATE_DESKTOP_ITEM_UNCHANGED;

	if (!g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_TIME_MODIFIED))
		retval = MATE_DESKTOP_ITEM_DISAPPEARED;
	else if (item->mtime < g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_TIME_MODIFIED))
		retval = MATE_DESKTOP_ITEM_CHANGED;

	g_object_unref (info);
	g_object_unref (file);

	return retval;
}

/**
 * mate_desktop_item_find_icon:
 * @icon_theme: a #GtkIconTheme
 * @icon: icon name, something you'd get out of the Icon key
 * @desired_size: FIXME
 * @flags: FIXME
 *
 * Description:  This function goes and looks for the icon file.  If the icon
 * is not an absolute filename, this will look for it in the standard places.
 * If it can't find the icon, it will return %NULL
 *
 * Returns: A newly allocated string
 */
char *
mate_desktop_item_find_icon (GtkIconTheme *icon_theme,
			      const char *icon,
			      int desired_size,
			      int flags)
{
	GtkIconInfo *info;
	char *full = NULL;

	g_return_val_if_fail (icon_theme == NULL ||
			      GTK_IS_ICON_THEME (icon_theme), NULL);

	if (icon == NULL || strcmp(icon,"") == 0) {
		return NULL;
	} else if (g_path_is_absolute (icon)) {
		if (g_file_test (icon, G_FILE_TEST_EXISTS)) {
			return g_strdup (icon);
		} else {
			return NULL;
		}
	} else {
		char *icon_no_extension;
		char *p;

		if (icon_theme == NULL)
			icon_theme = gtk_icon_theme_get_default ();

		icon_no_extension = g_strdup (icon);
		p = strrchr (icon_no_extension, '.');
		if (p &&
		    (strcmp (p, ".png") == 0 ||
		     strcmp (p, ".xpm") == 0 ||
		     strcmp (p, ".svg") == 0)) {
		    *p = 0;
		}


		info = gtk_icon_theme_lookup_icon (icon_theme,
						   icon_no_extension,
						   desired_size,
						   0);

		full = NULL;
		if (info) {
			full = g_strdup (gtk_icon_info_get_filename (info));
			g_object_unref (info);
		}
		g_free (icon_no_extension);
	}

	return full;

}

/**
 * mate_desktop_item_get_icon:
 * @icon_theme: a #GtkIconTheme
 * @item: A desktop item
 *
 * Description:  This function goes and looks for the icon file.  If the icon
 * is not set as an absolute filename, this will look for it in the standard places.
 * If it can't find the icon, it will return %NULL
 *
 * Returns: A newly allocated string
 */
char *
mate_desktop_item_get_icon (const MateDesktopItem *item,
			     GtkIconTheme *icon_theme)
{
	/* maybe this function should be deprecated in favour of find icon
	 * -George */
	const char *icon;

	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (item->refcount > 0, NULL);

	icon = mate_desktop_item_get_string (item, MATE_DESKTOP_ITEM_ICON);

	return mate_desktop_item_find_icon (icon_theme, icon,
					     48 /* desired_size */,
					     0 /* flags */);
}

/**
 * mate_desktop_item_get_location:
 * @item: A desktop item
 *
 * Returns: The file location associated with 'item'.
 *
 */
const char *
mate_desktop_item_get_location (const MateDesktopItem *item)
{
	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (item->refcount > 0, NULL);

	return item->location;
}

/**
 * mate_desktop_item_set_location:
 * @item: A desktop item
 * @location: A uri string specifying the file location of this particular item.
 *
 * Set's the 'location' uri of this item.
 */
void
mate_desktop_item_set_location (MateDesktopItem *item, const char *location)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);

	if (item->location != NULL &&
	    location != NULL &&
	    strcmp (item->location, location) == 0)
		return;

	g_free (item->location);
	item->location = g_strdup (location);

	/* This is ugly, but useful internally */
	if (item->mtime != DONT_UPDATE_MTIME) {
		item->mtime = 0;

		if (item->location) {
			GFile     *file;
			GFileInfo *info;

			file = g_file_new_for_uri (item->location);

			info = g_file_query_info (file,
						  G_FILE_ATTRIBUTE_TIME_MODIFIED,
						  G_FILE_QUERY_INFO_NONE,
						  NULL, NULL);
			if (info) {
				if (g_file_info_has_attribute (info,
							       G_FILE_ATTRIBUTE_TIME_MODIFIED))
					item->mtime = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_TIME_MODIFIED);
				g_object_unref (info);
			}

			g_object_unref (file);
		}
	}

	/* Make sure that save actually saves */
	item->modified = TRUE;
}

/**
 * mate_desktop_item_set_location_file:
 * @item: A desktop item
 * @file: A local filename specifying the file location of this particular item.
 *
 * Set's the 'location' uri of this item to the given @file.
 */
void
mate_desktop_item_set_location_file (MateDesktopItem *item, const char *file)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);

	if (file != NULL) {
		GFile *gfile;

		gfile = g_file_new_for_path (file);
		mate_desktop_item_set_location_gfile (item, gfile);
		g_object_unref (gfile);
	} else {
		mate_desktop_item_set_location (item, NULL);
	}
}

static void
mate_desktop_item_set_location_gfile (MateDesktopItem *item, GFile *file)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);

	if (file != NULL) {
		char *uri;

		uri = g_file_get_uri (file);
		mate_desktop_item_set_location (item, uri);
		g_free (uri);
	} else {
		mate_desktop_item_set_location (item, NULL);
	}
}

/*
 * Reading/Writing different sections, NULL is the standard section
 */

gboolean
mate_desktop_item_attr_exists (const MateDesktopItem *item,
				const char *attr)
{
	g_return_val_if_fail (item != NULL, FALSE);
	g_return_val_if_fail (item->refcount > 0, FALSE);
	g_return_val_if_fail (attr != NULL, FALSE);

	return lookup (item, attr) != NULL;
}

/*
 * String type
 */
const char *
mate_desktop_item_get_string (const MateDesktopItem *item,
			       const char *attr)
{
	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (item->refcount > 0, NULL);
	g_return_val_if_fail (attr != NULL, NULL);

	return lookup (item, attr);
}

void
mate_desktop_item_set_string (MateDesktopItem *item,
			       const char *attr,
			       const char *value)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);
	g_return_if_fail (attr != NULL);

	set (item, attr, value);

	if (strcmp (attr, MATE_DESKTOP_ITEM_TYPE) == 0)
		item->type = type_from_string (value);
}

/*
 * LocaleString type
 */
const char* mate_desktop_item_get_localestring(const MateDesktopItem* item, const char* attr)
{
	g_return_val_if_fail(item != NULL, NULL);
	g_return_val_if_fail(item->refcount > 0, NULL);
	g_return_val_if_fail(attr != NULL, NULL);

	return lookup_best_locale(item, attr);
}

const char* mate_desktop_item_get_localestring_lang(const MateDesktopItem* item, const char* attr, const char* language)
{
	g_return_val_if_fail(item != NULL, NULL);
	g_return_val_if_fail(item->refcount > 0, NULL);
	g_return_val_if_fail(attr != NULL, NULL);

	return lookup_locale(item, attr, language);
}

/**
 * mate_desktop_item_get_string_locale:
 * @item: A desktop item
 * @attr: An attribute name
 *
 * Returns the current locale that is used for the given attribute.
 * This might not be the same for all attributes. For example, if your
 * locale is "en_US.ISO8859-1" but attribute FOO only has "en_US" then
 * that would be returned for attr = "FOO". If attribute BAR has
 * "en_US.ISO8859-1" then that would be returned for "BAR".
 *
 * Returns: a string equal to the current locale or NULL
 * if the attribute is invalid or there is no matching locale.
 */
const char *
mate_desktop_item_get_attr_locale (const MateDesktopItem *item,
				    const char             *attr)
{
	const char * const *langs_pointer;
	int                 i;

	langs_pointer = g_get_language_names ();
	for (i = 0; langs_pointer[i] != NULL; i++) {
		const char *value = NULL;

		value = lookup_locale (item, attr, langs_pointer[i]);
		if (value)
			return langs_pointer[i];
	}

	return NULL;
}

GList *
mate_desktop_item_get_languages (const MateDesktopItem *item,
				  const char *attr)
{
	GList *li;
	GList *list = NULL;

	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (item->refcount > 0, NULL);

	for (li = item->languages; li != NULL; li = li->next) {
		char *language = li->data;
		if (attr == NULL ||
		    lookup_locale (item, attr, language) != NULL) {
			list = g_list_prepend (list, language);
		}
	}

	return g_list_reverse (list);
}

static const char *
get_language (void)
{
	const char * const *langs_pointer;
	int                 i;

	langs_pointer = g_get_language_names ();
	for (i = 0; langs_pointer[i] != NULL; i++) {
		/* find first without encoding  */
		if (strchr (langs_pointer[i], '.') == NULL) {
			return langs_pointer[i];
		}
	}
	return NULL;
}

void
mate_desktop_item_set_localestring (MateDesktopItem *item,
				     const char *attr,
				     const char *value)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);
	g_return_if_fail (attr != NULL);

	set_locale (item, attr, get_language (), value);
}

void
mate_desktop_item_set_localestring_lang (MateDesktopItem *item,
					  const char *attr,
					  const char *language,
					  const char *value)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);
	g_return_if_fail (attr != NULL);

	set_locale (item, attr, language, value);
}

void
mate_desktop_item_clear_localestring (MateDesktopItem *item,
				       const char *attr)
{
	GList *l;

	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);
	g_return_if_fail (attr != NULL);

	for (l = item->languages; l != NULL; l = l->next)
		set_locale (item, attr, l->data, NULL);

	set (item, attr, NULL);
}

/*
 * Strings, Regexps types
 */

char **
mate_desktop_item_get_strings (const MateDesktopItem *item,
				const char *attr)
{
	const char *value;

	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (item->refcount > 0, NULL);
	g_return_val_if_fail (attr != NULL, NULL);

	value = lookup (item, attr);
	if (value == NULL)
		return NULL;

	/* FIXME: there's no way to escape semicolons apparently */
	return g_strsplit (value, ";", -1);
}

void
mate_desktop_item_set_strings (MateDesktopItem *item,
				const char *attr,
				char **strings)
{
	char *str, *str2;

	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);
	g_return_if_fail (attr != NULL);

	str = g_strjoinv (";", strings);
	str2 = g_strconcat (str, ";", NULL);
	/* FIXME: there's no way to escape semicolons apparently */
	set (item, attr, str2);
	g_free (str);
	g_free (str2);
}

/*
 * Boolean type
 */
gboolean
mate_desktop_item_get_boolean (const MateDesktopItem *item,
				const char *attr)
{
	const char *value;

	g_return_val_if_fail (item != NULL, FALSE);
	g_return_val_if_fail (item->refcount > 0, FALSE);
	g_return_val_if_fail (attr != NULL, FALSE);

	value = lookup (item, attr);
	if (value == NULL)
		return FALSE;

	return (value[0] == 'T' ||
		value[0] == 't' ||
		value[0] == 'Y' ||
		value[0] == 'y' ||
		atoi (value) != 0);
}

void
mate_desktop_item_set_boolean (MateDesktopItem *item,
				const char *attr,
				gboolean value)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);
	g_return_if_fail (attr != NULL);

	set (item, attr, value ? "true" : "false");
}

void
mate_desktop_item_set_launch_time (MateDesktopItem *item,
				    guint32           timestamp)
{
	g_return_if_fail (item != NULL);

	item->launch_time = timestamp;
}

/*
 * Clearing attributes
 */
void
mate_desktop_item_clear_section (MateDesktopItem *item,
				  const char *section)
{
	Section *sec;
	GList *li;

	g_return_if_fail (item != NULL);
	g_return_if_fail (item->refcount > 0);

	sec = find_section (item, section);

	if (sec == NULL) {
		for (li = item->keys; li != NULL; li = li->next) {
			g_hash_table_remove (item->main_hash, li->data);
			g_free (li->data);
			li->data = NULL;
		}
		g_list_free (item->keys);
		item->keys = NULL;
	} else {
		for (li = sec->keys; li != NULL; li = li->next) {
			char *key = li->data;
			char *full = g_strdup_printf ("%s/%s",
						      sec->name, key);
			g_hash_table_remove (item->main_hash, full);
			g_free (full);
			g_free (key);
			li->data = NULL;
		}
		g_list_free (sec->keys);
		sec->keys = NULL;
	}
	item->modified = TRUE;
}

/************************************************************
 * Parser:                                                  *
 ************************************************************/

static gboolean G_GNUC_CONST
standard_is_boolean (const char * key)
{
	static GHashTable *bools = NULL;

	if (bools == NULL) {
		bools = g_hash_table_new (g_str_hash, g_str_equal);
		g_hash_table_insert (bools,
				     MATE_DESKTOP_ITEM_NO_DISPLAY,
				     MATE_DESKTOP_ITEM_NO_DISPLAY);
		g_hash_table_insert (bools,
				     MATE_DESKTOP_ITEM_HIDDEN,
				     MATE_DESKTOP_ITEM_HIDDEN);
		g_hash_table_insert (bools,
				     MATE_DESKTOP_ITEM_TERMINAL,
				     MATE_DESKTOP_ITEM_TERMINAL);
		g_hash_table_insert (bools,
				     MATE_DESKTOP_ITEM_READ_ONLY,
				     MATE_DESKTOP_ITEM_READ_ONLY);
	}

	return g_hash_table_lookup (bools, key) != NULL;
}

static gboolean G_GNUC_CONST
standard_is_strings (const char *key)
{
	static GHashTable *strings = NULL;

	if (strings == NULL) {
		strings = g_hash_table_new (g_str_hash, g_str_equal);
		g_hash_table_insert (strings,
				     MATE_DESKTOP_ITEM_FILE_PATTERN,
				     MATE_DESKTOP_ITEM_FILE_PATTERN);
		g_hash_table_insert (strings,
				     MATE_DESKTOP_ITEM_ACTIONS,
				     MATE_DESKTOP_ITEM_ACTIONS);
		g_hash_table_insert (strings,
				     MATE_DESKTOP_ITEM_MIME_TYPE,
				     MATE_DESKTOP_ITEM_MIME_TYPE);
		g_hash_table_insert (strings,
				     MATE_DESKTOP_ITEM_PATTERNS,
				     MATE_DESKTOP_ITEM_PATTERNS);
		g_hash_table_insert (strings,
				     MATE_DESKTOP_ITEM_SORT_ORDER,
				     MATE_DESKTOP_ITEM_SORT_ORDER);
	}

	return g_hash_table_lookup (strings, key) != NULL;
}

/* If no need to cannonize, returns NULL */
static char *
cannonize (const char *key, const char *value)
{
	if (standard_is_boolean (key)) {
		if (value[0] == 'T' ||
		    value[0] == 't' ||
		    value[0] == 'Y' ||
		    value[0] == 'y' ||
		    atoi (value) != 0) {
			return g_strdup ("true");
		} else {
			return g_strdup ("false");
		}
	} else if (standard_is_strings (key)) {
		int len = strlen (value);
		if (len == 0 || value[len-1] != ';') {
			return g_strconcat (value, ";", NULL);
		}
	}
	/* XXX: Perhaps we should canonize numeric values as well, but this
	 * has caused some subtle problems before so it needs to be done
	 * carefully if at all */
	return NULL;
}


static char *
decode_string_and_dup (const char *s)
{
	char *p = g_malloc (strlen (s) + 1);
	char *q = p;

	do {
		if (*s == '\\'){
			switch (*(++s)){
			case 's':
				*p++ = ' ';
				break;
			case 't':
				*p++ = '\t';
				break;
			case 'n':
				*p++ = '\n';
				break;
			case '\\':
				*p++ = '\\';
				break;
			case 'r':
				*p++ = '\r';
				break;
			default:
				*p++ = '\\';
				*p++ = *s;
				break;
			}
		} else {
			*p++ = *s;
		}
	} while (*s++);

	return q;
}

static char *
escape_string_and_dup (const char *s)
{
	char *return_value, *p;
	const char *q;
	int len = 0;

	if (s == NULL)
		return g_strdup("");

	q = s;
	while (*q){
		len++;
		if (strchr ("\n\r\t\\", *q) != NULL)
			len++;
		q++;
	}
	return_value = p = (char *) g_malloc (len + 1);
	do {
		switch (*s){
		case '\t':
			*p++ = '\\';
			*p++ = 't';
			break;
		case '\n':
			*p++ = '\\';
			*p++ = 'n';
			break;
		case '\r':
			*p++ = '\\';
			*p++ = 'r';
			break;
		case '\\':
			*p++ = '\\';
			*p++ = '\\';
			break;
		default:
			*p++ = *s;
		}
	} while (*s++);
	return return_value;
}

static gboolean
check_locale (const char *locale)
{
	GIConv cd = g_iconv_open ("UTF-8", locale);
	if ((GIConv)-1 == cd)
		return FALSE;
	g_iconv_close (cd);
	return TRUE;
}

static void
insert_locales (GHashTable *encodings, char *enc, ...)
{
	va_list args;
	char *s;

	va_start (args, enc);
	for (;;) {
		s = va_arg (args, char *);
		if (s == NULL)
			break;
		g_hash_table_insert (encodings, s, enc);
	}
	va_end (args);
}

/* make a standard conversion table from the desktop standard spec */
static GHashTable *
init_encodings (void)
{
	GHashTable *encodings = g_hash_table_new (g_str_hash, g_str_equal);

	/* "C" is plain ascii */
	insert_locales (encodings, "ASCII", "C", NULL);

	insert_locales (encodings, "ARMSCII-8", "by", NULL);
	insert_locales (encodings, "BIG5", "zh_TW", NULL);
	insert_locales (encodings, "CP1251", "be", "bg", NULL);
	if (check_locale ("EUC-CN")) {
		insert_locales (encodings, "EUC-CN", "zh_CN", NULL);
	} else {
		insert_locales (encodings, "GB2312", "zh_CN", NULL);
	}
	insert_locales (encodings, "EUC-JP", "ja", NULL);
	insert_locales (encodings, "EUC-KR", "ko", NULL);
	/*insert_locales (encodings, "GEORGIAN-ACADEMY", NULL);*/
	insert_locales (encodings, "GEORGIAN-PS", "ka", NULL);
	insert_locales (encodings, "ISO-8859-1", "br", "ca", "da", "de", "en", "es", "eu", "fi", "fr", "gl", "it", "nl", "wa", "no", "pt", "pt", "sv", NULL);
	insert_locales (encodings, "ISO-8859-2", "cs", "hr", "hu", "pl", "ro", "sk", "sl", "sq", "sr", NULL);
	insert_locales (encodings, "ISO-8859-3", "eo", NULL);
	insert_locales (encodings, "ISO-8859-5", "mk", "sp", NULL);
	insert_locales (encodings, "ISO-8859-7", "el", NULL);
	insert_locales (encodings, "ISO-8859-9", "tr", NULL);
	insert_locales (encodings, "ISO-8859-13", "lt", "lv", "mi", NULL);
	insert_locales (encodings, "ISO-8859-14", "ga", "cy", NULL);
	insert_locales (encodings, "ISO-8859-15", "et", NULL);
	insert_locales (encodings, "KOI8-R", "ru", NULL);
	insert_locales (encodings, "KOI8-U", "uk", NULL);
	if (check_locale ("TCVN-5712")) {
		insert_locales (encodings, "TCVN-5712", "vi", NULL);
	} else {
		insert_locales (encodings, "TCVN", "vi", NULL);
	}
	insert_locales (encodings, "TIS-620", "th", NULL);
	/*insert_locales (encodings, "VISCII", NULL);*/

	return encodings;
}

static const char *
get_encoding_from_locale (const char *locale)
{
	char lang[3];
	const char *encoding;
	static GHashTable *encodings = NULL;

	if (locale == NULL)
		return NULL;

	/* if locale includes encoding, use it */
	encoding = strchr (locale, '.');
	if (encoding != NULL) {
		return encoding+1;
	}

	if (encodings == NULL)
		encodings = init_encodings ();

	/* first try the entire locale (at this point ll_CC) */
	encoding = g_hash_table_lookup (encodings, locale);
	if (encoding != NULL)
		return encoding;

	/* Try just the language */
	strncpy (lang, locale, 2);
	lang[2] = '\0';
	return g_hash_table_lookup (encodings, lang);
}

static Encoding
get_encoding (ReadBuf *rb)
{
	gboolean old_kde = FALSE;
	char     buf [BUFSIZ];
	gboolean all_valid_utf8 = TRUE;

	while (readbuf_gets (buf, sizeof (buf), rb) != NULL) {
		if (strncmp (MATE_DESKTOP_ITEM_ENCODING,
			     buf,
			     strlen (MATE_DESKTOP_ITEM_ENCODING)) == 0) {
			char *p = &buf[strlen (MATE_DESKTOP_ITEM_ENCODING)];
			if (*p == ' ')
				p++;
			if (*p != '=')
				continue;
			p++;
			if (*p == ' ')
				p++;
			if (strcmp (p, "UTF-8") == 0) {
				return ENCODING_UTF8;
			} else if (strcmp (p, "Legacy-Mixed") == 0) {
				return ENCODING_LEGACY_MIXED;
			} else {
				/* According to the spec we're not supposed
				 * to read a file like this */
				return ENCODING_UNKNOWN;
			}
		} else if (strcmp ("[KDE Desktop Entry]", buf) == 0) {
			old_kde = TRUE;
			/* don't break yet, we still want to support
			 * Encoding even here */
		}
		if (all_valid_utf8 && ! g_utf8_validate (buf, -1, NULL))
			all_valid_utf8 = FALSE;
	}

	if (old_kde)
		return ENCODING_LEGACY_MIXED;

	/* try to guess by location */
	if (rb->uri != NULL && strstr (rb->uri, "mate/apps/") != NULL) {
		/* old mate */
		return ENCODING_LEGACY_MIXED;
	}

	/* A dilemma, new KDE files are in UTF-8 but have no Encoding
	 * info, at this time we really can't tell.  The best thing to
	 * do right now is to just assume UTF-8 if the whole file
	 * validates as utf8 I suppose */

	if (all_valid_utf8)
		return ENCODING_UTF8;
	else
		return ENCODING_LEGACY_MIXED;
}

static char *
decode_string (const char *value, Encoding encoding, const char *locale)
{
	char *retval = NULL;

	/* if legacy mixed, then convert */
	if (locale != NULL && encoding == ENCODING_LEGACY_MIXED) {
		const char *char_encoding = get_encoding_from_locale (locale);
		char *utf8_string;
		if (char_encoding == NULL)
			return NULL;
		if (strcmp (char_encoding, "ASCII") == 0) {
			return decode_string_and_dup (value);
		}
		utf8_string = g_convert (value, -1, "UTF-8", char_encoding,
					NULL, NULL, NULL);
		if (utf8_string == NULL)
			return NULL;
		retval = decode_string_and_dup (utf8_string);
		g_free (utf8_string);
		return retval;
	/* if utf8, then validate */
	} else if (locale != NULL && encoding == ENCODING_UTF8) {
		if ( ! g_utf8_validate (value, -1, NULL))
			/* invalid utf8, ignore this key */
			return NULL;
		return decode_string_and_dup (value);
	} else {
		/* Meaning this is not a localized string */
		return decode_string_and_dup (value);
	}
}

static char *
snarf_locale_from_key (const char *key)
{
	const char *brace;
	char *locale, *p;

	brace = strchr (key, '[');
	if (brace == NULL)
		return NULL;

	locale = g_strdup (brace + 1);
	if (*locale == '\0') {
		g_free (locale);
		return NULL;
	}
	p = strchr (locale, ']');
	if (p == NULL) {
		g_free (locale);
		return NULL;
	}
	*p = '\0';
	return locale;
}

static void
insert_key (MateDesktopItem *item,
	    Section *cur_section,
	    Encoding encoding,
	    const char *key,
	    const char *value,
	    gboolean old_kde,
	    gboolean no_translations)
{
	char *k;
	char *val;
	/* we always store everything in UTF-8 */
	if (cur_section == NULL &&
	    strcmp (key, MATE_DESKTOP_ITEM_ENCODING) == 0) {
		k = g_strdup (key);
		val = g_strdup ("UTF-8");
	} else {
		char *locale = snarf_locale_from_key (key);
		/* If we're ignoring translations */
		if (no_translations && locale != NULL) {
			g_free (locale);
			return;
		}
		val = decode_string (value, encoding, locale);

		/* Ignore this key, it's whacked */
		if (val == NULL) {
			g_free (locale);
			return;
		}

		g_strchomp (val);

		/* For old KDE entries, we can also split by a comma
		 * on sort order, so convert to semicolons */
		if (old_kde &&
		    cur_section == NULL &&
		    strcmp (key, MATE_DESKTOP_ITEM_SORT_ORDER) == 0 &&
		    strchr (val, ';') == NULL) {
			int i;
			for (i = 0; val[i] != '\0'; i++) {
				if (val[i] == ',')
					val[i] = ';';
			}
		}

		/* Check some types, not perfect, but catches a lot
		 * of things */
		if (cur_section == NULL) {
			char *cannon = cannonize (key, val);
			if (cannon != NULL) {
				g_free (val);
				val = cannon;
			}
		}

		k = g_strdup (key);

		/* Take care of the language part */
		if (locale != NULL &&
		    strcmp (locale, "C") == 0) {
			char *p;
			/* Whack C locale */
			p = strchr (k, '[');
			*p = '\0';
			g_free (locale);
		} else if (locale != NULL) {
			char *p, *brace;

			/* Whack the encoding part */
			p = strchr (locale, '.');
			if (p != NULL)
				*p = '\0';

			if (g_list_find_custom (item->languages, locale,
						(GCompareFunc)strcmp) == NULL) {
				item->languages = g_list_prepend
					(item->languages, locale);
			} else {
				g_free (locale);
			}

			/* Whack encoding from encoding in the key */
			brace = strchr (k, '[');
			p = strchr (brace, '.');
			if (p != NULL) {
				*p = ']';
				*(p+1) = '\0';
			}
		}
	}


	if (cur_section == NULL) {
		/* only add to list if we haven't seen it before */
		if (g_hash_table_lookup (item->main_hash, k) == NULL) {
			item->keys = g_list_prepend (item->keys,
						     g_strdup (k));
		}
		/* later duplicates override earlier ones */
		g_hash_table_replace (item->main_hash, k, val);
	} else {
		char *full = g_strdup_printf
			("%s/%s",
			 cur_section->name, k);
		/* only add to list if we haven't seen it before */
		if (g_hash_table_lookup (item->main_hash, full) == NULL) {
			cur_section->keys =
				g_list_prepend (cur_section->keys, k);
		}
		/* later duplicates override earlier ones */
		g_hash_table_replace (item->main_hash,
				      full, val);
	}
}

static void
setup_type (MateDesktopItem *item, const char *uri)
{
	const char *type = g_hash_table_lookup (item->main_hash,
						MATE_DESKTOP_ITEM_TYPE);
	if (type == NULL && uri != NULL) {
		char *base = g_path_get_basename (uri);
		if (base != NULL &&
		    strcmp (base, ".directory") == 0) {
			/* This gotta be a directory */
			g_hash_table_replace (item->main_hash,
					      g_strdup (MATE_DESKTOP_ITEM_TYPE),
					      g_strdup ("Directory"));
			item->keys = g_list_prepend
				(item->keys, g_strdup (MATE_DESKTOP_ITEM_TYPE));
			item->type = MATE_DESKTOP_ITEM_TYPE_DIRECTORY;
		} else {
			item->type = MATE_DESKTOP_ITEM_TYPE_NULL;
		}
		g_free (base);
	} else {
		item->type = type_from_string (type);
	}
}

/* fallback to find something suitable for C locale */
static char *
try_english_key (MateDesktopItem *item, const char *key)
{
	char *str;
	char *locales[] = { "en_US", "en_GB", "en_AU", "en", NULL };
	int i;

	str = NULL;
	for (i = 0; locales[i] != NULL && str == NULL; i++) {
		str = g_strdup (lookup_locale (item, key, locales[i]));
	}
	if (str != NULL) {
		/* We need a 7-bit ascii string, so whack all
		 * above 127 chars */
		guchar *p;
		for (p = (guchar *)str; *p != '\0'; p++) {
			if (*p > 127)
				*p = '?';
		}
	}
	return str;
}


static void
sanitize (MateDesktopItem *item, const char *uri)
{
	const char *type;

	type = lookup (item, MATE_DESKTOP_ITEM_TYPE);

	/* understand old mate style url exec thingies */
	if (type != NULL && strcmp (type, "URL") == 0) {
		const char *exec = lookup (item, MATE_DESKTOP_ITEM_EXEC);
		set (item, MATE_DESKTOP_ITEM_TYPE, "Link");
		if (exec != NULL) {
			/* Note, this must be in this order */
			set (item, MATE_DESKTOP_ITEM_URL, exec);
			set (item, MATE_DESKTOP_ITEM_EXEC, NULL);
		}
	}

	/* we make sure we have Name, Encoding and Version */
	if (lookup (item, MATE_DESKTOP_ITEM_NAME) == NULL) {
		char *name = try_english_key (item, MATE_DESKTOP_ITEM_NAME);
		/* If no name, use the basename */
		if (name == NULL && uri != NULL)
			name = g_path_get_basename (uri);
		/* If no uri either, use same default as mate_desktop_item_new */
		if (name == NULL) {
		       /* Translators: the "name" mentioned here is the name of
			* an application or a document */
			name = g_strdup (_("No name"));
		}
		g_hash_table_replace (item->main_hash,
				      g_strdup (MATE_DESKTOP_ITEM_NAME),
				      name);
		item->keys = g_list_prepend
			(item->keys, g_strdup (MATE_DESKTOP_ITEM_NAME));
	}
	if (lookup (item, MATE_DESKTOP_ITEM_ENCODING) == NULL) {
		/* We store everything in UTF-8 so write that down */
		g_hash_table_replace (item->main_hash,
				      g_strdup (MATE_DESKTOP_ITEM_ENCODING),
				      g_strdup ("UTF-8"));
		item->keys = g_list_prepend
			(item->keys, g_strdup (MATE_DESKTOP_ITEM_ENCODING));
	}
	if (lookup (item, MATE_DESKTOP_ITEM_VERSION) == NULL) {
		/* this is the version that we follow, so write it down */
		g_hash_table_replace (item->main_hash,
				      g_strdup (MATE_DESKTOP_ITEM_VERSION),
				      g_strdup ("1.0"));
		item->keys = g_list_prepend
			(item->keys, g_strdup (MATE_DESKTOP_ITEM_VERSION));
	}
}

enum {
	FirstBrace,
	OnSecHeader,
	IgnoreToEOL,
	IgnoreToEOLFirst,
	KeyDef,
	KeyDefOnKey,
	KeyValue
};

static MateDesktopItem *
ditem_load (ReadBuf *rb,
	    gboolean no_translations,
	    GError **error)
{
	int state;
	char CharBuffer [1024];
	char *next = CharBuffer;
	int c;
	Encoding encoding;
	MateDesktopItem *item;
	Section *cur_section = NULL;
	char *key = NULL;
	gboolean old_kde = FALSE;

	encoding = get_encoding (rb);
	if (encoding == ENCODING_UNKNOWN) {
		/* spec says, don't read this file */
		g_set_error (error,
			     MATE_DESKTOP_ITEM_ERROR,
			     MATE_DESKTOP_ITEM_ERROR_UNKNOWN_ENCODING,
			     _("Unknown encoding of: %s"),
			     rb->uri);
		readbuf_close (rb);
		return NULL;
	}

	/* Rewind since get_encoding goes through the file */
	if (! readbuf_rewind (rb, error)) {
		readbuf_close (rb);
		/* spec says, don't read this file */
		return NULL;
	}

	item = mate_desktop_item_new ();
	item->modified = FALSE;

	/* Note: location and mtime are filled in by the new_from_file
	 * function since it has those values */

#define OVERFLOW (next == &CharBuffer [sizeof(CharBuffer)-1])

	state = FirstBrace;
	while ((c = readbuf_getc (rb)) != EOF) {
		if (c == '\r')		/* Ignore Carriage Return */
			continue;

		switch (state) {

		case OnSecHeader:
			if (c == ']' || OVERFLOW) {
				*next = '\0';
				next = CharBuffer;

				/* keys were inserted in reverse */
				if (cur_section != NULL &&
				    cur_section->keys != NULL) {
					cur_section->keys = g_list_reverse
						(cur_section->keys);
				}
				if (strcmp (CharBuffer,
					    "KDE Desktop Entry") == 0) {
					/* Main section */
					cur_section = NULL;
					old_kde = TRUE;
				} else if (strcmp (CharBuffer,
						   "Desktop Entry") == 0) {
					/* Main section */
					cur_section = NULL;
				} else {
					cur_section = g_new0 (Section, 1);
					cur_section->name =
						g_strdup (CharBuffer);
					cur_section->keys = NULL;
					item->sections = g_list_prepend
						(item->sections, cur_section);
				}
				state = IgnoreToEOL;
			} else if (c == '[') {
				/* FIXME: probably error out instead of ignoring this */
			} else {
				*next++ = c;
			}
			break;

		case IgnoreToEOL:
		case IgnoreToEOLFirst:
			if (c == '\n'){
				if (state == IgnoreToEOLFirst)
					state = FirstBrace;
				else
					state = KeyDef;
				next = CharBuffer;
			}
			break;

		case FirstBrace:
		case KeyDef:
		case KeyDefOnKey:
			if (c == '#') {
				if (state == FirstBrace)
					state = IgnoreToEOLFirst;
				else
					state = IgnoreToEOL;
				break;
			}

			if (c == '[' && state != KeyDefOnKey){
				state = OnSecHeader;
				next = CharBuffer;
				g_free (key);
				key = NULL;
				break;
			}
			/* On first pass, don't allow dangling keys */
			if (state == FirstBrace)
				break;

			if ((c == ' ' && state != KeyDefOnKey) || c == '\t')
				break;

			if (c == '\n' || OVERFLOW) { /* Abort Definition */
				next = CharBuffer;
				state = KeyDef;
				break;
			}

			if (c == '=' || OVERFLOW){
				*next = '\0';

				g_free (key);
				key = g_strdup (CharBuffer);
				state = KeyValue;
				next = CharBuffer;
			} else {
				*next++ = c;
				state = KeyDefOnKey;
			}
			break;

		case KeyValue:
			if (OVERFLOW || c == '\n'){
				*next = '\0';

				insert_key (item, cur_section, encoding,
					    key, CharBuffer, old_kde,
					    no_translations);

				g_free (key);
				key = NULL;

				state = (c == '\n') ? KeyDef : IgnoreToEOL;
				next = CharBuffer;
			} else {
				*next++ = c;
			}
			break;

		} /* switch */

	} /* while ((c = getc_unlocked (f)) != EOF) */
	if (c == EOF && state == KeyValue) {
		*next = '\0';

		insert_key (item, cur_section, encoding,
			    key, CharBuffer, old_kde,
			    no_translations);

		g_free (key);
		key = NULL;
	}

#undef OVERFLOW

	/* keys were inserted in reverse */
	if (cur_section != NULL &&
	    cur_section->keys != NULL) {
		cur_section->keys = g_list_reverse (cur_section->keys);
	}
	/* keys were inserted in reverse */
	item->keys = g_list_reverse (item->keys);
	/* sections were inserted in reverse */
	item->sections = g_list_reverse (item->sections);

	/* sanitize some things */
	sanitize (item, rb->uri);

	/* make sure that we set up the type */
	setup_type (item, rb->uri);

	readbuf_close (rb);

	return item;
}

static void stream_printf (GFileOutputStream *stream,
			   const char *format, ...) G_GNUC_PRINTF (2, 3);

static void
stream_printf (GFileOutputStream *stream, const char *format, ...)
{
    va_list args;
    gchar *s;

    va_start (args, format);
    s = g_strdup_vprintf (format, args);
    va_end (args);

    /* FIXME: what about errors */
    g_output_stream_write (G_OUTPUT_STREAM (stream), s, strlen (s),
		    	   NULL, NULL);
    g_free (s);
}

static void
dump_section (MateDesktopItem *item, GFileOutputStream *stream, Section *section)
{
	GList *li;

	stream_printf (stream, "[%s]\n", section->name);
	for (li = section->keys; li != NULL; li = li->next) {
		const char *key = li->data;
		char *full = g_strdup_printf ("%s/%s", section->name, key);
		const char *value = g_hash_table_lookup (item->main_hash, full);
		if (value != NULL) {
			char *val = escape_string_and_dup (value);
			stream_printf (stream, "%s=%s\n", key, val);
			g_free (val);
		}
		g_free (full);
	}
}

static gboolean
ditem_save (MateDesktopItem *item, const char *uri, GError **error)
{
	GList *li;
	GFile *file;
	GFileOutputStream *stream;

	file = g_file_new_for_uri (uri);
	stream = g_file_replace (file, NULL, FALSE, G_FILE_CREATE_NONE,
				 NULL, error);
	if (stream == NULL)
		return FALSE;

	stream_printf (stream, "[Desktop Entry]\n");
	for (li = item->keys; li != NULL; li = li->next) {
		const char *key = li->data;
		const char *value = g_hash_table_lookup (item->main_hash, key);
		if (value != NULL) {
			char *val = escape_string_and_dup (value);
			stream_printf (stream, "%s=%s\n", key, val);
			g_free (val);
		}
	}

	if (item->sections != NULL)
		stream_printf (stream, "\n");

	for (li = item->sections; li != NULL; li = li->next) {
		Section *section = li->data;

		/* Don't write empty sections */
		if (section->keys == NULL)
			continue;

		dump_section (item, stream, section);

		if (li->next != NULL)
			stream_printf (stream, "\n");
	}

	g_object_unref (stream);
	g_object_unref (file);

	return TRUE;
}

static gpointer
_mate_desktop_item_copy (gpointer boxed)
{
	return mate_desktop_item_copy (boxed);
}

static void
_mate_desktop_item_free (gpointer boxed)
{
	mate_desktop_item_unref (boxed);
}

GType
mate_desktop_item_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		type = g_boxed_type_register_static ("MateDesktopItem",
						     _mate_desktop_item_copy,
						     _mate_desktop_item_free);
	}

	return type;
}

GQuark
mate_desktop_item_error_quark (void)
{
	static GQuark q = 0;
	if (q == 0)
		q = g_quark_from_static_string ("mate-desktop-item-error-quark");

	return q;
}
