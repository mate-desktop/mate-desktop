/*
 * mate-thumbnail.c: Utilities for handling thumbnails
 *
 * Copyright (C) 2002 Red Hat, Inc.
 * Copyright (C) 2010 Carlos Garcia Campos <carlosgc@gnome.org>
 *
 * This file is part of the Mate Library.
 *
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#include <config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <glib.h>
#include <stdio.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#define MATE_DESKTOP_USE_UNSTABLE_API
#include "mate-desktop-thumbnail.h"
#include <glib/gstdio.h>
#include <glib-unix.h>

#define SECONDS_BETWEEN_STATS 10

static void
thumbnailers_directory_changed (GFileMonitor                 *monitor,
                                GFile                        *file,
                                GFile                        *other_file,
                                GFileMonitorEvent             event_type,
                                MateDesktopThumbnailFactory  *factory);

struct _MateDesktopThumbnailFactoryPrivate {
  MateDesktopThumbnailSize size;

  GMutex lock;

  GList *thumbnailers;
  GHashTable *mime_types_map;
  GList *monitors;

  GSettings *settings;
  gboolean loaded : 1;
  gboolean disabled : 1;
  gchar **disabled_types;
};

static const char *appname = "mate-thumbnail-factory";

G_DEFINE_TYPE_WITH_PRIVATE (MateDesktopThumbnailFactory,
                            mate_desktop_thumbnail_factory,
                            G_TYPE_OBJECT)

#define parent_class mate_desktop_thumbnail_factory_parent_class

#define THUMBNAILER_ENTRY_GROUP "Thumbnailer Entry"
#define THUMBNAILER_EXTENSION   ".thumbnailer"

typedef struct {
    volatile gint ref_count;
    gchar  *path;
    gchar  *try_exec;
    gchar  *command;
    gchar **mime_types;
} Thumbnailer;

static Thumbnailer *
thumbnailer_ref (Thumbnailer *thumb)
{
  g_return_val_if_fail (thumb != NULL, NULL);
  g_return_val_if_fail (thumb->ref_count > 0, NULL);

  g_atomic_int_inc (&thumb->ref_count);
  return thumb;
}

static void
thumbnailer_unref (Thumbnailer *thumb)
{
  g_return_if_fail (thumb != NULL);
  g_return_if_fail (thumb->ref_count > 0);

  if (g_atomic_int_dec_and_test (&thumb->ref_count))
    {
      g_free (thumb->path);
      g_free (thumb->try_exec);
      g_free (thumb->command);
      g_strfreev (thumb->mime_types);
      g_slice_free (Thumbnailer, thumb);
    }
}

static Thumbnailer *
thumbnailer_load (Thumbnailer *thumb)
{
  GKeyFile *key_file;
  GError *error = NULL;

  key_file = g_key_file_new ();
  if (!g_key_file_load_from_file (key_file, thumb->path, 0, &error))
    {
      g_warning ("Failed to load thumbnailer from \"%s\": %s\n", thumb->path, error->message);
      g_error_free (error);
      thumbnailer_unref (thumb);
      g_key_file_free (key_file);

      return NULL;
    }

  if (!g_key_file_has_group (key_file, THUMBNAILER_ENTRY_GROUP))
    {
      g_warning ("Invalid thumbnailer: missing group \"%s\"\n", THUMBNAILER_ENTRY_GROUP);
      thumbnailer_unref (thumb);
      g_key_file_free (key_file);

      return NULL;
    }

  thumb->command = g_key_file_get_string (key_file, THUMBNAILER_ENTRY_GROUP, "Exec", NULL);
  if (!thumb->command)
    {
      g_warning ("Invalid thumbnailer: missing Exec key\n");
      thumbnailer_unref (thumb);
      g_key_file_free (key_file);

      return NULL;
    }

  thumb->mime_types = g_key_file_get_string_list (key_file, THUMBNAILER_ENTRY_GROUP, "MimeType", NULL, NULL);
  if (!thumb->mime_types)
    {
      g_warning ("Invalid thumbnailer: missing MimeType key\n");
      thumbnailer_unref (thumb);
      g_key_file_free (key_file);

      return NULL;
    }

  thumb->try_exec = g_key_file_get_string (key_file, THUMBNAILER_ENTRY_GROUP, "TryExec", NULL);

  g_key_file_free (key_file);

  return thumb;
}

static Thumbnailer *
thumbnailer_reload (Thumbnailer *thumb)
{
  g_return_val_if_fail (thumb != NULL, NULL);

  g_free (thumb->command);
  thumb->command = NULL;
  g_strfreev (thumb->mime_types);
  thumb->mime_types = NULL;
  g_free (thumb->try_exec);
  thumb->try_exec = NULL;

  return thumbnailer_load (thumb);
}

static Thumbnailer *
thumbnailer_new (const gchar *path)
{
  Thumbnailer *thumb;

  thumb = g_slice_new0 (Thumbnailer);
  thumb->ref_count = 1;
  thumb->path = g_strdup (path);

  return thumbnailer_load (thumb);
}

static gboolean
thumbnailer_try_exec (Thumbnailer *thumb)
{
  gchar *path;
  gboolean retval;

  if (G_UNLIKELY (!thumb))
    return FALSE;

  /* TryExec is optinal, but Exec isn't, so we assume
   * the thumbnailer can be run when TryExec is not present
   */
  if (!thumb->try_exec)
    return TRUE;

  path = g_find_program_in_path (thumb->try_exec);
  retval = path != NULL;
  g_free (path);

  return retval;
}

static gpointer
init_thumbnailers_dirs (gpointer data)
{
  const gchar * const *data_dirs;
  GPtrArray *thumbs_dirs;
  guint i;

  data_dirs = g_get_system_data_dirs ();
  thumbs_dirs = g_ptr_array_new ();

  g_ptr_array_add (thumbs_dirs, g_build_filename (g_get_user_data_dir (), "thumbnailers", NULL));
  for (i = 0; data_dirs[i] != NULL; i++)
    g_ptr_array_add (thumbs_dirs, g_build_filename (data_dirs[i], "thumbnailers", NULL));
  g_ptr_array_add (thumbs_dirs, NULL);

  return g_ptr_array_free (thumbs_dirs, FALSE);
}

static const gchar * const *
get_thumbnailers_dirs (void)
{
  static GOnce once_init = G_ONCE_INIT;
  return g_once (&once_init, init_thumbnailers_dirs, NULL);
}

/* These should be called with the lock held */
static void
mate_desktop_thumbnail_factory_register_mime_types (MateDesktopThumbnailFactory *factory,
                                                     Thumbnailer                  *thumb)
{
  MateDesktopThumbnailFactoryPrivate *priv = factory->priv;
  gint i;

  for (i = 0; thumb->mime_types[i]; i++)
    {
      if (!g_hash_table_lookup (priv->mime_types_map, thumb->mime_types[i]))
        g_hash_table_insert (priv->mime_types_map,
                             g_strdup (thumb->mime_types[i]),
                             thumbnailer_ref (thumb));
    }
}

static void
mate_desktop_thumbnail_factory_add_thumbnailer (MateDesktopThumbnailFactory *factory,
                                                 Thumbnailer                  *thumb)
{
  MateDesktopThumbnailFactoryPrivate *priv = factory->priv;

  mate_desktop_thumbnail_factory_register_mime_types (factory, thumb);
  priv->thumbnailers = g_list_prepend (priv->thumbnailers, thumb);
}

static gboolean
mate_desktop_thumbnail_factory_is_disabled (MateDesktopThumbnailFactory *factory,
                                             const gchar                  *mime_type)
{
  MateDesktopThumbnailFactoryPrivate *priv = factory->priv;
  guint i;

  if (priv->disabled)
    return TRUE;

  if (!priv->disabled_types)
    return FALSE;

  for (i = 0; priv->disabled_types[i]; i++)
    {
      if (g_strcmp0 (priv->disabled_types[i], mime_type) == 0)
        return TRUE;
    }

  return FALSE;
}

static gboolean
remove_thumbnailer_from_mime_type_map (gchar       *key,
                                       Thumbnailer *value,
                                       gchar       *path)
{
  return (strcmp (value->path, path) == 0);
}

static void
update_or_create_thumbnailer (MateDesktopThumbnailFactory *factory,
                              const gchar                 *path)
{
  MateDesktopThumbnailFactoryPrivate *priv = factory->priv;
  GList *l;
  Thumbnailer *thumb;
  gboolean found = FALSE;

  g_mutex_lock (&priv->lock);

  for (l = priv->thumbnailers; l && !found; l = g_list_next (l))
    {
      thumb = (Thumbnailer *)l->data;

      if (strcmp (thumb->path, path) == 0)
        {
          found = TRUE;

          /* First remove the mime_types associated to this thumbnailer */
          g_hash_table_foreach_remove (priv->mime_types_map,
                                       (GHRFunc)remove_thumbnailer_from_mime_type_map,
                                       (gpointer)path);
          if (!thumbnailer_reload (thumb))
              priv->thumbnailers = g_list_delete_link (priv->thumbnailers, l);
          else
              mate_desktop_thumbnail_factory_register_mime_types (factory, thumb);
        }
    }

  if (!found)
    {
      thumb = thumbnailer_new (path);
      if (thumb)
        mate_desktop_thumbnail_factory_add_thumbnailer (factory, thumb);
    }

  g_mutex_unlock (&priv->lock);
}

static void
remove_thumbnailer (MateDesktopThumbnailFactory *factory,
                    const gchar                 *path)
{
  MateDesktopThumbnailFactoryPrivate *priv = factory->priv;
  GList *l;
  Thumbnailer *thumb;

  g_mutex_lock (&priv->lock);

  for (l = priv->thumbnailers; l; l = g_list_next (l))
    {
      thumb = (Thumbnailer *)l->data;

      if (strcmp (thumb->path, path) == 0)
        {
          priv->thumbnailers = g_list_delete_link (priv->thumbnailers, l);
          g_hash_table_foreach_remove (priv->mime_types_map,
                                       (GHRFunc)remove_thumbnailer_from_mime_type_map,
                                       (gpointer)path);
          thumbnailer_unref (thumb);

          break;
        }
    }

  g_mutex_unlock (&priv->lock);
}

static void
remove_thumbnailers_for_dir (MateDesktopThumbnailFactory *factory,
                             const gchar                 *thumbnailer_dir,
                             GFileMonitor                *monitor)
{
  MateDesktopThumbnailFactoryPrivate *priv = factory->priv;
  GList *l;
  Thumbnailer *thumb;

  g_mutex_lock (&priv->lock);

  /* Remove all the thumbnailers inside this @thumbnailer_dir. */
  for (l = priv->thumbnailers; l; l = g_list_next (l))
    {
      thumb = (Thumbnailer *)l->data;

      if (g_str_has_prefix (thumb->path, thumbnailer_dir) == TRUE)
        {
          priv->thumbnailers = g_list_delete_link (priv->thumbnailers, l);
          g_hash_table_foreach_remove (priv->mime_types_map,
                                       (GHRFunc)remove_thumbnailer_from_mime_type_map,
                                       (gpointer)thumb->path);
          thumbnailer_unref (thumb);

          break;
        }
    }

  /* Remove the monitor for @thumbnailer_dir. */
  priv->monitors = g_list_remove (priv->monitors, monitor);
  g_signal_handlers_disconnect_by_func (monitor, thumbnailers_directory_changed, factory);

  g_mutex_unlock (&priv->lock);
}

static void
mate_desktop_thumbnail_factory_load_thumbnailers_for_dir (MateDesktopThumbnailFactory *factory,
                                                          const gchar                 *path)
{
  MateDesktopThumbnailFactoryPrivate *priv = factory->priv;
  GDir *dir;
  GFile *dir_file;
  GFileMonitor *monitor;
  const gchar *dirent;

  dir = g_dir_open (path, 0, NULL);
  if (!dir)
      return;

  /* Monitor dir */
  dir_file = g_file_new_for_path (path);
  monitor = g_file_monitor_directory (dir_file,
                                      G_FILE_MONITOR_NONE,
                                      NULL, NULL);
  if (monitor)
    {
      g_signal_connect (monitor, "changed",
                        G_CALLBACK (thumbnailers_directory_changed),
                        factory);
      priv->monitors = g_list_prepend (priv->monitors, monitor);
    }
  g_object_unref (dir_file);

  while ((dirent = g_dir_read_name (dir)))
    {
      Thumbnailer *thumb;
      gchar       *filename;

      if (!g_str_has_suffix (dirent, THUMBNAILER_EXTENSION))
          continue;

      filename = g_build_filename (path, dirent, NULL);
      thumb = thumbnailer_new (filename);
      g_free (filename);

      if (thumb)
          mate_desktop_thumbnail_factory_add_thumbnailer (factory, thumb);
    }

  g_dir_close (dir);
}

static void
thumbnailers_directory_changed (GFileMonitor                *monitor,
                                GFile                       *file,
                                GFile                       *other_file,
                                GFileMonitorEvent            event_type,
                                MateDesktopThumbnailFactory *factory)
{
  gchar *path;

  switch (event_type)
    {
    case G_FILE_MONITOR_EVENT_CREATED:
    case G_FILE_MONITOR_EVENT_CHANGED:
    case G_FILE_MONITOR_EVENT_DELETED:
      path = g_file_get_path (file);
      if (!g_str_has_suffix (path, THUMBNAILER_EXTENSION))
        {
          g_free (path);
          return;
        }

      if (event_type == G_FILE_MONITOR_EVENT_DELETED)
        remove_thumbnailer (factory, path);
      else
        update_or_create_thumbnailer (factory, path);

      g_free (path);
      break;
    case G_FILE_MONITOR_EVENT_UNMOUNTED:
    case G_FILE_MONITOR_EVENT_MOVED:
      path = g_file_get_path (file);
      remove_thumbnailers_for_dir (factory, path, monitor);

      if (event_type == G_FILE_MONITOR_EVENT_MOVED)
          mate_desktop_thumbnail_factory_load_thumbnailers_for_dir (factory, path);

      g_free (path);
      break;
    case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
    case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
    case G_FILE_MONITOR_EVENT_PRE_UNMOUNT:
    default:
      break;
    }
}

static void
mate_desktop_thumbnail_factory_load_thumbnailers (MateDesktopThumbnailFactory *factory)
{
  MateDesktopThumbnailFactoryPrivate *priv = factory->priv;
  const gchar * const *dirs;
  guint i;

  if (priv->loaded)
    return;

  dirs = get_thumbnailers_dirs ();
  for (i = 0; dirs[i]; i++)
    {
      mate_desktop_thumbnail_factory_load_thumbnailers_for_dir (factory, dirs[i]);
    }

  priv->loaded = TRUE;
}

static void
external_thumbnailers_disabled_all_changed_cb (GSettings                   *settings,
                                               const gchar                 *key,
                                               MateDesktopThumbnailFactory *factory)
{
  MateDesktopThumbnailFactoryPrivate *priv = factory->priv;

  g_mutex_lock (&priv->lock);

  priv->disabled = g_settings_get_boolean (priv->settings, "disable-all");
  if (priv->disabled)
    {
      g_strfreev (priv->disabled_types);
      priv->disabled_types = NULL;
    }
  else
    {
      priv->disabled_types = g_settings_get_strv (priv->settings, "disable");
      mate_desktop_thumbnail_factory_load_thumbnailers (factory);
    }

  g_mutex_unlock (&priv->lock);
}

static void
external_thumbnailers_disabled_changed_cb (GSettings                   *settings,
                                           const gchar                 *key,
                                           MateDesktopThumbnailFactory *factory)
{
  MateDesktopThumbnailFactoryPrivate *priv = factory->priv;

  g_mutex_lock (&priv->lock);

  if (!priv->disabled)
    {
      g_strfreev (priv->disabled_types);
      priv->disabled_types = g_settings_get_strv (priv->settings, "disable");
    }

  g_mutex_unlock (&priv->lock);
}

static void
mate_desktop_thumbnail_factory_init (MateDesktopThumbnailFactory *factory)
{
  MateDesktopThumbnailFactoryPrivate *priv;

  factory->priv = mate_desktop_thumbnail_factory_get_instance_private (factory);

  priv = factory->priv;

  priv->size = MATE_DESKTOP_THUMBNAIL_SIZE_NORMAL;

  priv->mime_types_map = g_hash_table_new_full (g_str_hash,
                                                g_str_equal,
                                                (GDestroyNotify)g_free,
                                                (GDestroyNotify)thumbnailer_unref);

  g_mutex_init (&priv->lock);

  priv->settings = g_settings_new ("org.mate.thumbnailers");

  g_signal_connect (priv->settings, "changed::disable-all",
                    G_CALLBACK (external_thumbnailers_disabled_all_changed_cb),
                    factory);
  g_signal_connect (priv->settings, "changed::disable",
                    G_CALLBACK (external_thumbnailers_disabled_changed_cb),
                    factory);

  priv->disabled = g_settings_get_boolean (priv->settings, "disable-all");

  if (!priv->disabled)
    priv->disabled_types = g_settings_get_strv (priv->settings, "disable");

  if (!priv->disabled)
    mate_desktop_thumbnail_factory_load_thumbnailers (factory);
}

static void
mate_desktop_thumbnail_factory_finalize (GObject *object)
{
  MateDesktopThumbnailFactory *factory;
  MateDesktopThumbnailFactoryPrivate *priv;
  
  factory = MATE_DESKTOP_THUMBNAIL_FACTORY (object);

  priv = factory->priv;

  if (priv->thumbnailers)
    {
      g_list_free_full (priv->thumbnailers, (GDestroyNotify)thumbnailer_unref);
      priv->thumbnailers = NULL;
    }

  g_clear_pointer (&priv->mime_types_map, g_hash_table_destroy);

  if (priv->monitors)
    {
      g_list_free_full (priv->monitors, (GDestroyNotify)g_object_unref);
      priv->monitors = NULL;
    }

  g_mutex_clear (&priv->lock);

  g_clear_pointer (&priv->disabled_types, g_strfreev);

  if (priv->settings)
    {
      g_signal_handlers_disconnect_by_func (priv->settings,
                                            external_thumbnailers_disabled_all_changed_cb,
                                            factory);
      g_signal_handlers_disconnect_by_func (priv->settings,
                                            external_thumbnailers_disabled_changed_cb,
                                            factory);
      g_clear_object (&priv->settings);
    }

  if (G_OBJECT_CLASS (parent_class)->finalize)
    (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
mate_desktop_thumbnail_factory_class_init (MateDesktopThumbnailFactoryClass *class)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (class);

  gobject_class->finalize = mate_desktop_thumbnail_factory_finalize;
}

/**
 * mate_desktop_thumbnail_factory_new:
 * @size: The thumbnail size to use
 *
 * Creates a new #MateDesktopThumbnailFactory.
 *
 * This function must be called on the main thread.
 *
 * Return value: a new #MateDesktopThumbnailFactory
 *
 * Since: 2.2
 **/
MateDesktopThumbnailFactory *
mate_desktop_thumbnail_factory_new (MateDesktopThumbnailSize size)
{
  MateDesktopThumbnailFactory *factory;

  factory = g_object_new (MATE_DESKTOP_TYPE_THUMBNAIL_FACTORY, NULL);

  factory->priv->size = size;

  return factory;
}

static char *
thumbnail_filename (const char *uri)
{
  GChecksum *checksum;
  guint8 digest[16];
  gsize digest_len = sizeof (digest);
  char *file;

  checksum = g_checksum_new (G_CHECKSUM_MD5);
  g_checksum_update (checksum, (const guchar *) uri, strlen (uri));

  g_checksum_get_digest (checksum, digest, &digest_len);
  g_assert (digest_len == 16);

  file = g_strconcat (g_checksum_get_string (checksum), ".png", NULL);

  g_checksum_free (checksum);

  return file;
}

static char *
thumbnail_path (const char               *uri,
                MateDesktopThumbnailSize  size)
{
  char *path, *file;

  file = thumbnail_filename (uri);
  path = g_build_filename (g_get_user_cache_dir (),
                           "thumbnails",
                           size == MATE_DESKTOP_THUMBNAIL_SIZE_LARGE ? "large" : "normal",
                           file,
                           NULL);
  g_free (file);
  return path;
}

static char *
thumbnail_failed_path (const char *uri)
{
  char *path, *file;

  file = thumbnail_filename (uri);
  /* XXX: appname is only used for failed thumbnails. Is this a mistake? */
  path = g_build_filename (g_get_user_cache_dir (),
                           "thumbnails",
                           "fail",
                           appname,
                           file,
                           NULL);
  g_free (file);
  return path;
}

static char *
validate_thumbnail_path (char                     *path,
                         const char               *uri,
                         time_t                    mtime,
                         MateDesktopThumbnailSize  size)
{
  GdkPixbuf *pixbuf;

  pixbuf = gdk_pixbuf_new_from_file (path, NULL);
  if (pixbuf == NULL ||
      !mate_desktop_thumbnail_is_valid (pixbuf, uri, mtime)) {
      g_free (path);
      return NULL;
  }

  g_clear_object (&pixbuf);

  return path;
}

static char *
lookup_thumbnail_path (const char               *uri,
                       time_t                    mtime,
                       MateDesktopThumbnailSize  size)
{
  char *path = thumbnail_path (uri, size);
  return validate_thumbnail_path (path, uri, mtime, size);
}

static char *
lookup_failed_thumbnail_path (const char               *uri,
                              time_t                    mtime,
                              MateDesktopThumbnailSize  size)
{
  char *path = thumbnail_failed_path (uri);
  return validate_thumbnail_path (path, uri, mtime, size);
}

/**
 * mate_desktop_thumbnail_factory_lookup:
 * @factory: a #MateDesktopThumbnailFactory
 * @uri: the uri of a file
 * @mtime: the mtime of the file
 *
 * Tries to locate an existing thumbnail for the file specified.
 *
 * Usage of this function is threadsafe.
 *
 * Return value: (transfer full): The absolute path of the thumbnail, or %NULL if none exist.
 *
 * Since: 2.2
 **/
char *
mate_desktop_thumbnail_factory_lookup (MateDesktopThumbnailFactory *factory,
                                       const char                  *uri,
                                       time_t                       mtime)
{
  MateDesktopThumbnailFactoryPrivate *priv = factory->priv;

  g_return_val_if_fail (uri != NULL, NULL);

  return lookup_thumbnail_path (uri, mtime, priv->size);
}

/**
 * mate_desktop_thumbnail_factory_has_valid_failed_thumbnail:
 * @factory: a #MateDesktopThumbnailFactory
 * @uri: the uri of a file
 * @mtime: the mtime of the file
 *
 * Tries to locate an failed thumbnail for the file specified. Writing
 * and looking for failed thumbnails is important to avoid to try to
 * thumbnail e.g. broken images several times.
 *
 * Usage of this function is threadsafe.
 *
 * Return value: TRUE if there is a failed thumbnail for the file.
 *
 * Since: 2.2
 **/
gboolean
mate_desktop_thumbnail_factory_has_valid_failed_thumbnail (MateDesktopThumbnailFactory *factory,
                                                           const char                  *uri,
                                                           time_t                       mtime)
{
  char *path;

  g_return_val_if_fail (uri != NULL, FALSE);

  path = lookup_failed_thumbnail_path (uri, mtime, factory->priv->size);
  if (path == NULL)
    return FALSE;

  g_free (path);

  return TRUE;
}

/**
 * mate_desktop_thumbnail_factory_can_thumbnail:
 * @factory: a #MateDesktopThumbnailFactory
 * @uri: the uri of a file
 * @mime_type: the mime type of the file
 * @mtime: the mtime of the file
 *
 * Returns TRUE if this MateDesktopThumbnail can (at least try) to thumbnail
 * this file. Thumbnails or files with failed thumbnails won't be thumbnailed.
 *
 * Usage of this function is threadsafe.
 *
 * Return value: TRUE if the file can be thumbnailed.
 *
 * Since: 2.2
 **/
gboolean
mate_desktop_thumbnail_factory_can_thumbnail (MateDesktopThumbnailFactory *factory,
                                              const char                  *uri,
                                              const char                  *mime_type,
                                              time_t                       mtime)
{
  gboolean have_script = FALSE;

  /* Don't thumbnail thumbnails */
  if (uri &&
      strncmp (uri, "file:/", 6) == 0 &&
      (strstr (uri, "/.thumbnails/") != NULL ||
      strstr (uri, "/.cache/thumbnails/") != NULL))
    return FALSE;

  if (!mime_type)
    return FALSE;

  g_mutex_lock (&factory->priv->lock);
  if (!mate_desktop_thumbnail_factory_is_disabled (factory, mime_type))
    {
      Thumbnailer *thumb;

      thumb = g_hash_table_lookup (factory->priv->mime_types_map, mime_type);
      have_script = thumbnailer_try_exec (thumb);
    }
  g_mutex_unlock (&factory->priv->lock);

  if (uri && (have_script ))
    {
      return !mate_desktop_thumbnail_factory_has_valid_failed_thumbnail (factory,
                                                                         uri,
                                                                         mtime);
    }

  return FALSE;
}

static char *
expand_thumbnailing_script (const char *script,
                            const int   size,
                            const char *inuri,
                            const char *outfile)
{
  GString *str;
  const char *p, *last;
  char *localfile, *quoted;
  gboolean got_in;

  str = g_string_new (NULL);

  got_in = FALSE;
  last = script;
  while ((p = strchr (last, '%')) != NULL)
    {
      g_string_append_len (str, last, p - last);
      p++;

      switch (*p) {
      case 'u':
    quoted = g_shell_quote (inuri);
    g_string_append (str, quoted);
    g_free (quoted);
    got_in = TRUE;
    p++;
    break;
      case 'i':
    localfile = g_filename_from_uri (inuri, NULL, NULL);
    if (localfile)
      {
        quoted = g_shell_quote (localfile);
        g_string_append (str, quoted);
        got_in = TRUE;
        g_free (quoted);
        g_free (localfile);
      }
    p++;
    break;
      case 'o':
    quoted = g_shell_quote (outfile);
    g_string_append (str, quoted);
    g_free (quoted);
    p++;
    break;
      case 's':
    g_string_append_printf (str, "%d", size);
    p++;
    break;
      case '%':
    g_string_append_c (str, '%');
    p++;
    break;
      case 0:
      default:
    break;
      }
      last = p;
    }
  g_string_append (str, last);

  if (got_in)
    return g_string_free (str, FALSE);

  g_string_free (str, TRUE);
  return NULL;
}

static GdkPixbuf *
get_preview_thumbnail (const char *uri,
                       int         size)
{
    GdkPixbuf *pixbuf;
    GFile *file;
    GFileInfo *file_info;
    GInputStream *input_stream;
    GObject *object;

    g_return_val_if_fail (uri != NULL, NULL);

    input_stream = NULL;

    file = g_file_new_for_uri (uri);

    /* First see if we can get an input stream via preview::icon  */
    file_info = g_file_query_info (file,
                                   G_FILE_ATTRIBUTE_PREVIEW_ICON,
                                   G_FILE_QUERY_INFO_NONE,
                                   NULL,  /* GCancellable */
                                   NULL); /* return location for GError */
    g_object_unref (file);

    if (file_info == NULL)
      return NULL;

    object = g_file_info_get_attribute_object (file_info,
                                               G_FILE_ATTRIBUTE_PREVIEW_ICON);
    if (object)
        g_object_ref (object);
    g_object_unref (file_info);

    if (!object)
      return NULL;
    if (!G_IS_LOADABLE_ICON (object)) {
      g_object_unref (object);
      return NULL;
    }

    input_stream = g_loadable_icon_load (G_LOADABLE_ICON (object),
                                         0,     /* size */
                                         NULL,  /* return location for type */
                                         NULL,  /* GCancellable */
                                         NULL); /* return location for GError */
    g_object_unref (object);

    if (!input_stream)
      return NULL;

    pixbuf = gdk_pixbuf_new_from_stream_at_scale (input_stream,
                                                  size, size,
                                                  TRUE, NULL, NULL);
    g_object_unref (input_stream);

    return pixbuf;
}

/**
 * mate_desktop_thumbnail_factory_generate_thumbnail:
 * @factory: a #MateDesktopThumbnailFactory
 * @uri: the uri of a file
 * @mime_type: the mime type of the file
 *
 * Tries to generate a thumbnail for the specified file. If it succeeds
 * it returns a pixbuf that can be used as a thumbnail.
 *
 * Usage of this function is threadsafe.
 *
 * Return value: (transfer full): thumbnail pixbuf if thumbnailing succeeded, %NULL otherwise.
 *
 * Since: 2.2
 **/
GdkPixbuf *
mate_desktop_thumbnail_factory_generate_thumbnail (MateDesktopThumbnailFactory *factory,
                                                   const char                  *uri,
                                                   const char                  *mime_type)
{
  GdkPixbuf *pixbuf;
  char *script, *expanded_script;
  int size;
  int exit_status;
  char *tmpname;

  g_return_val_if_fail (uri != NULL, NULL);
  g_return_val_if_fail (mime_type != NULL, NULL);

  /* Doesn't access any volatile fields in factory, so it's threadsafe */

  size = 128;
  if (factory->priv->size == MATE_DESKTOP_THUMBNAIL_SIZE_LARGE)
    size = 256;

  pixbuf = NULL;

  pixbuf = get_preview_thumbnail (uri, size);
  if (pixbuf != NULL)
    return pixbuf;

  script = NULL;
  g_mutex_lock (&factory->priv->lock);
  if (!mate_desktop_thumbnail_factory_is_disabled (factory, mime_type))
    {
      Thumbnailer *thumb;

      thumb = g_hash_table_lookup (factory->priv->mime_types_map, mime_type);
      if (thumb)
        script = g_strdup (thumb->command);
    }
  g_mutex_unlock (&factory->priv->lock);

  if (script)
    {
      int fd;

      fd = g_file_open_tmp (".mate_desktop_thumbnail.XXXXXX", &tmpname, NULL);

      if (fd != -1)
    {
      close (fd);

      expanded_script = expand_thumbnailing_script (script, size, uri, tmpname);
      if (expanded_script != NULL &&
          g_spawn_command_line_sync (expanded_script,
                     NULL, NULL, &exit_status, NULL) &&
          exit_status == 0)
        {
          pixbuf = gdk_pixbuf_new_from_file (tmpname, NULL);
        }

      g_free (expanded_script);
      g_unlink (tmpname);
      g_free (tmpname);
    }

      g_free (script);
    }

  return pixbuf;
}

static gboolean
save_thumbnail (GdkPixbuf  *pixbuf,
                char       *path,
                const char *uri,
                time_t      mtime)
{
  char *dirname;
  char *tmp_path = NULL;
  int tmp_fd;
  gchar *mtime_str;
  gboolean ret = FALSE;
  GError *error = NULL;
  const char *width, *height;

  if (pixbuf == NULL)
    return FALSE;

  dirname = g_path_get_dirname (path);

  if (g_mkdir_with_parents (dirname, 0700) != 0)

    goto out;

  tmp_path = g_strconcat (path, ".XXXXXX", NULL);
  tmp_fd = g_mkstemp (tmp_path);

  if (tmp_fd == -1)
    goto out;
  close (tmp_fd);

  mtime_str = g_strdup_printf ("%" G_GINT64_FORMAT,  (gint64) mtime);
  width = gdk_pixbuf_get_option (pixbuf, "tEXt::Thumb::Image::Width");
  height = gdk_pixbuf_get_option (pixbuf, "tEXt::Thumb::Image::Height");

  error = NULL;
  if (width != NULL && height != NULL)
    ret = gdk_pixbuf_save (pixbuf,
                           tmp_path,
                           "png", &error,
                           "tEXt::Thumb::Image::Width", width,
                           "tEXt::Thumb::Image::Height", height,
                           "tEXt::Thumb::URI", uri,
                           "tEXt::Thumb::MTime", mtime_str,
                           "tEXt::Software", "MATE::ThumbnailFactory",
                           NULL);
  else
    ret = gdk_pixbuf_save (pixbuf,
                           tmp_path,
                           "png", &error,
                           "tEXt::Thumb::URI", uri,
                           "tEXt::Thumb::MTime", mtime_str,
                           "tEXt::Software", "MATE::ThumbnailFactory",
                           NULL);
  g_free (mtime_str);

  if (!ret)
    goto out;

  g_chmod (tmp_path, 0600);
  g_rename (tmp_path, path);

 out:
  if (error != NULL)
    {
      g_warning ("Failed to create thumbnail %s: %s", tmp_path, error->message);
      g_error_free (error);
    }
  g_unlink (tmp_path);
  g_free (tmp_path);
  g_free (dirname);
  return ret;
}

static GdkPixbuf *
make_failed_thumbnail (void)
{
  GdkPixbuf *pixbuf;

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 1, 1);
  gdk_pixbuf_fill (pixbuf, 0x00000000);
  return pixbuf;
}

/**
 * mate_desktop_thumbnail_factory_save_thumbnail:
 * @factory: a #MateDesktopThumbnailFactory
 * @thumbnail: the thumbnail as a pixbuf
 * @uri: the uri of a file
 * @original_mtime: the modification time of the original file
 *
 * Saves @thumbnail at the right place. If the save fails a
 * failed thumbnail is written.
 *
 * Usage of this function is threadsafe.
 *
 * Since: 2.2
 **/
void
mate_desktop_thumbnail_factory_save_thumbnail (MateDesktopThumbnailFactory *factory,
                                               GdkPixbuf                   *thumbnail,
                                               const char                  *uri,
                                               time_t                       original_mtime)
{
  char *path;

  path = thumbnail_path (uri, factory->priv->size);
  if (!save_thumbnail (thumbnail, path, uri, original_mtime))
    {
      thumbnail = make_failed_thumbnail ();
      g_free (path);
      path = thumbnail_failed_path (uri);
      save_thumbnail (thumbnail, path, uri, original_mtime);
      g_object_unref (thumbnail);
    }
  g_free (path);
}

/**
 * mate_desktop_thumbnail_factory_create_failed_thumbnail:
 * @factory: a #MateDesktopThumbnailFactory
 * @uri: the uri of a file
 * @mtime: the modification time of the file
 *
 * Creates a failed thumbnail for the file so that we don't try
 * to re-thumbnail the file later.
 *
 * Usage of this function is threadsafe.
 *
 * Since: 2.2
 **/
void
mate_desktop_thumbnail_factory_create_failed_thumbnail (MateDesktopThumbnailFactory *factory,
                                                        const char                  *uri,
                                                        time_t                      mtime)
{
  char *path;
  GdkPixbuf *pixbuf;

  path = thumbnail_failed_path (uri);
  pixbuf = make_failed_thumbnail ();
  save_thumbnail (pixbuf, path, uri, mtime);

  g_free (path);
  g_object_unref (pixbuf);
}

/**
 * mate_desktop_thumbnail_md5:
 * @uri: an uri
 *
 * Calculates the MD5 checksum of the uri. This can be useful
 * if you want to manually handle thumbnail files.
 *
 * Return value: A string with the MD5 digest of the uri string.
 *
 * Since: 2.2
 * Deprecated: 2.22: Use #GChecksum instead
 **/
char *
mate_desktop_thumbnail_md5 (const char *uri)
{
  return g_compute_checksum_for_data (G_CHECKSUM_MD5,
                                      (const guchar *) uri,
                                      strlen (uri));
}

/**
 * mate_desktop_thumbnail_path_for_uri:
 * @uri: an uri
 * @size: a thumbnail size
 *
 * Returns the filename that a thumbnail of size @size for @uri would have.
 *
 * Return value: an absolute filename
 *
 * Since: 2.2
 **/
char *
mate_desktop_thumbnail_path_for_uri (const char               *uri,
                                     MateDesktopThumbnailSize  size)
{
  return thumbnail_path (uri, size);
}

/**
 * mate_desktop_thumbnail_has_uri:
 * @pixbuf: an loaded thumbnail pixbuf
 * @uri: a uri
 *
 * Returns whether the thumbnail has the correct uri embedded in the
 * Thumb::URI option in the png.
 *
 * Return value: TRUE if the thumbnail is for @uri
 *
 * Since: 2.2
 **/
gboolean
mate_desktop_thumbnail_has_uri (GdkPixbuf          *pixbuf,
                                const char         *uri)
{
  const char *thumb_uri;

  thumb_uri = gdk_pixbuf_get_option (pixbuf, "tEXt::Thumb::URI");
  if (!thumb_uri)
    return FALSE;

  return strcmp (uri, thumb_uri) == 0;
}

/**
 * mate_desktop_thumbnail_is_valid:
 * @pixbuf: an loaded thumbnail #GdkPixbuf
 * @uri: a uri
 * @mtime: the mtime
 *
 * Returns whether the thumbnail has the correct uri and mtime embedded in the
 * png options.
 *
 * Return value: TRUE if the thumbnail has the right @uri and @mtime
 *
 * Since: 2.2
 **/
gboolean
mate_desktop_thumbnail_is_valid (GdkPixbuf          *pixbuf,
                                 const char         *uri,
                                 time_t              mtime)
{
  const char *thumb_uri, *thumb_mtime_str;
  time_t thumb_mtime;

  thumb_uri = gdk_pixbuf_get_option (pixbuf, "tEXt::Thumb::URI");
  if (!thumb_uri)
    return FALSE;
  if (strcmp (uri, thumb_uri) != 0)
    return FALSE;

  thumb_mtime_str = gdk_pixbuf_get_option (pixbuf, "tEXt::Thumb::MTime");
  if (!thumb_mtime_str)
    return FALSE;
  thumb_mtime = (time_t)g_ascii_strtoll (thumb_mtime_str, (gchar**)NULL, 10);
  if (mtime != thumb_mtime)
    return FALSE;

  return TRUE;
}
