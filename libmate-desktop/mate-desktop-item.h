/* -*- Mode: C; c-set-style: linux indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-ditem.h - MATE Desktop File Representation

   Copyright (C) 1999, 2000 Red Hat Inc.
   Copyright (C) 2001 Sid Vicious
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
   Boston, MA  02110-1301, USA.  */
/*
  @NOTATION@
 */

#ifndef MATE_DITEM_H
#define MATE_DITEM_H

#include <glib.h>
#include <glib-object.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum {
	MATE_DESKTOP_ITEM_TYPE_NULL = 0 /* This means its NULL, that is, not
					   * set */,
	MATE_DESKTOP_ITEM_TYPE_OTHER /* This means it's not one of the below
					      strings types, and you must get the
					      Type attribute. */,

	/* These are the standard compliant types: */
	MATE_DESKTOP_ITEM_TYPE_APPLICATION,
	MATE_DESKTOP_ITEM_TYPE_LINK,
	MATE_DESKTOP_ITEM_TYPE_FSDEVICE,
	MATE_DESKTOP_ITEM_TYPE_MIME_TYPE,
	MATE_DESKTOP_ITEM_TYPE_DIRECTORY,
	MATE_DESKTOP_ITEM_TYPE_SERVICE,
	MATE_DESKTOP_ITEM_TYPE_SERVICE_TYPE
} MateDesktopItemType;

typedef enum {
        MATE_DESKTOP_ITEM_UNCHANGED = 0,
        MATE_DESKTOP_ITEM_CHANGED = 1,
        MATE_DESKTOP_ITEM_DISAPPEARED = 2
} MateDesktopItemStatus;

#define MATE_TYPE_DESKTOP_ITEM         (mate_desktop_item_get_type ())
GType mate_desktop_item_get_type       (void);

typedef struct _MateDesktopItem MateDesktopItem;

/* standard */
#define MATE_DESKTOP_ITEM_ENCODING	"Encoding" /* string */
#define MATE_DESKTOP_ITEM_VERSION	"Version"  /* numeric */
#define MATE_DESKTOP_ITEM_NAME		"Name" /* localestring */
#define MATE_DESKTOP_ITEM_GENERIC_NAME	"GenericName" /* localestring */
#define MATE_DESKTOP_ITEM_TYPE		"Type" /* string */
#define MATE_DESKTOP_ITEM_FILE_PATTERN "FilePattern" /* regexp(s) */
#define MATE_DESKTOP_ITEM_TRY_EXEC	"TryExec" /* string */
#define MATE_DESKTOP_ITEM_NO_DISPLAY	"NoDisplay" /* boolean */
#define MATE_DESKTOP_ITEM_COMMENT	"Comment" /* localestring */
#define MATE_DESKTOP_ITEM_EXEC		"Exec" /* string */
#define MATE_DESKTOP_ITEM_ACTIONS	"Actions" /* strings */
#define MATE_DESKTOP_ITEM_ICON		"Icon" /* string */
#define MATE_DESKTOP_ITEM_MINI_ICON	"MiniIcon" /* string */
#define MATE_DESKTOP_ITEM_HIDDEN	"Hidden" /* boolean */
#define MATE_DESKTOP_ITEM_PATH		"Path" /* string */
#define MATE_DESKTOP_ITEM_TERMINAL	"Terminal" /* boolean */
#define MATE_DESKTOP_ITEM_TERMINAL_OPTIONS "TerminalOptions" /* string */
#define MATE_DESKTOP_ITEM_SWALLOW_TITLE "SwallowTitle" /* string */
#define MATE_DESKTOP_ITEM_SWALLOW_EXEC	"SwallowExec" /* string */
#define MATE_DESKTOP_ITEM_MIME_TYPE	"MimeType" /* regexp(s) */
#define MATE_DESKTOP_ITEM_PATTERNS	"Patterns" /* regexp(s) */
#define MATE_DESKTOP_ITEM_DEFAULT_APP	"DefaultApp" /* string */
#define MATE_DESKTOP_ITEM_DEV		"Dev" /* string */
#define MATE_DESKTOP_ITEM_FS_TYPE	"FSType" /* string */
#define MATE_DESKTOP_ITEM_MOUNT_POINT	"MountPoint" /* string */
#define MATE_DESKTOP_ITEM_READ_ONLY	"ReadOnly" /* boolean */
#define MATE_DESKTOP_ITEM_UNMOUNT_ICON "UnmountIcon" /* string */
#define MATE_DESKTOP_ITEM_SORT_ORDER	"SortOrder" /* strings */
#define MATE_DESKTOP_ITEM_URL		"URL" /* string */
#define MATE_DESKTOP_ITEM_DOC_PATH	"X-MATE-DocPath" /* string */

/* The vfolder proposal */
#define MATE_DESKTOP_ITEM_CATEGORIES	"Categories" /* string */
#define MATE_DESKTOP_ITEM_ONLY_SHOW_IN	"OnlyShowIn" /* string */

typedef enum {
	/* Use the TryExec field to determine if this should be loaded */
        MATE_DESKTOP_ITEM_LOAD_ONLY_IF_EXISTS = 1<<0,
        MATE_DESKTOP_ITEM_LOAD_NO_TRANSLATIONS = 1<<1
} MateDesktopItemLoadFlags;

typedef enum {
	/* Never launch more instances even if the app can only
	 * handle one file and we have passed many */
        MATE_DESKTOP_ITEM_LAUNCH_ONLY_ONE = 1<<0,
	/* Use current directory instead of home directory */
        MATE_DESKTOP_ITEM_LAUNCH_USE_CURRENT_DIR = 1<<1,
	/* Append the list of URIs to the command if no Exec
	 * parameter is specified, instead of launching the
	 * app without parameters. */
	MATE_DESKTOP_ITEM_LAUNCH_APPEND_URIS = 1<<2,
	/* Same as above but instead append local paths */
	MATE_DESKTOP_ITEM_LAUNCH_APPEND_PATHS = 1<<3,
	/* Don't automatically reap child process.  */
	MATE_DESKTOP_ITEM_LAUNCH_DO_NOT_REAP_CHILD = 1<<4
} MateDesktopItemLaunchFlags;

typedef enum {
	/* Don't check the kde directories */
        MATE_DESKTOP_ITEM_ICON_NO_KDE = 1<<0
} MateDesktopItemIconFlags;

typedef enum {
	MATE_DESKTOP_ITEM_ERROR_NO_FILENAME /* No filename set or given on save */,
	MATE_DESKTOP_ITEM_ERROR_UNKNOWN_ENCODING /* Unknown encoding of the file */,
	MATE_DESKTOP_ITEM_ERROR_CANNOT_OPEN /* Cannot open file */,
	MATE_DESKTOP_ITEM_ERROR_NO_EXEC_STRING /* Cannot launch due to no execute string */,
	MATE_DESKTOP_ITEM_ERROR_BAD_EXEC_STRING /* Cannot launch due to bad execute string */,
	MATE_DESKTOP_ITEM_ERROR_NO_URL /* No URL on a url entry*/,
	MATE_DESKTOP_ITEM_ERROR_NOT_LAUNCHABLE /* Not a launchable type of item */,
	MATE_DESKTOP_ITEM_ERROR_INVALID_TYPE /* Not of type application/x-mate-app-info */
} MateDesktopItemError;

/* Note that functions can also return the G_FILE_ERROR_* errors */

#define MATE_DESKTOP_ITEM_ERROR mate_desktop_item_error_quark ()
GQuark mate_desktop_item_error_quark (void);

/* Returned item from new*() and copy() methods have a refcount of 1 */
MateDesktopItem *      mate_desktop_item_new               (void);
MateDesktopItem *      mate_desktop_item_new_from_file     (const char                 *file,
							      MateDesktopItemLoadFlags   flags,
							      GError                    **error);
MateDesktopItem *      mate_desktop_item_new_from_uri      (const char                 *uri,
							      MateDesktopItemLoadFlags   flags,
							      GError                    **error);
MateDesktopItem *      mate_desktop_item_new_from_string   (const char                 *uri,
							      const char                 *string,
							      gssize                      length,
							      MateDesktopItemLoadFlags   flags,
							      GError                    **error);
MateDesktopItem *      mate_desktop_item_new_from_basename (const char                 *basename,
							      MateDesktopItemLoadFlags   flags,
							      GError                    **error);
MateDesktopItem *      mate_desktop_item_copy              (const MateDesktopItem     *item);

/* if under is NULL save in original location */
gboolean                mate_desktop_item_save              (MateDesktopItem           *item,
							      const char                 *under,
							      gboolean			  force,
							      GError                    **error);
MateDesktopItem *      mate_desktop_item_ref               (MateDesktopItem           *item);
void                    mate_desktop_item_unref             (MateDesktopItem           *item);
int			mate_desktop_item_launch	     (const MateDesktopItem     *item,
							      GList                      *file_list,
							      MateDesktopItemLaunchFlags flags,
							      GError                    **error);
int			mate_desktop_item_launch_with_env   (const MateDesktopItem     *item,
							      GList                      *file_list,
							      MateDesktopItemLaunchFlags flags,
							      char                      **envp,
							      GError                    **error);

int                     mate_desktop_item_launch_on_screen  (const MateDesktopItem       *item,
							      GList                        *file_list,
							      MateDesktopItemLaunchFlags   flags,
							      GdkScreen                    *screen,
							      int                           workspace,
							      GError                      **error);

/* A list of files or urls dropped onto an icon */
int                     mate_desktop_item_drop_uri_list     (const MateDesktopItem     *item,
							      const char                 *uri_list,
							      MateDesktopItemLaunchFlags flags,
							      GError                    **error);

int                     mate_desktop_item_drop_uri_list_with_env    (const MateDesktopItem     *item,
								      const char                 *uri_list,
								      MateDesktopItemLaunchFlags flags,
								      char                      **envp,
								      GError                    **error);

gboolean                mate_desktop_item_exists            (const MateDesktopItem     *item);

MateDesktopItemType	mate_desktop_item_get_entry_type    (const MateDesktopItem	 *item);
/* You could also just use the set_string on the TYPE argument */
void			mate_desktop_item_set_entry_type    (MateDesktopItem		 *item,
							      MateDesktopItemType	  type);

/* Get current location on disk */
const char *            mate_desktop_item_get_location      (const MateDesktopItem     *item);
void                    mate_desktop_item_set_location      (MateDesktopItem           *item,
							      const char                 *location);
void                    mate_desktop_item_set_location_file (MateDesktopItem           *item,
							      const char                 *file);
MateDesktopItemStatus  mate_desktop_item_get_file_status   (const MateDesktopItem     *item);

/*
 * Get the icon, this is not as simple as getting the Icon attr as it actually tries to find
 * it and returns %NULL if it can't
 */
char *                  mate_desktop_item_get_icon          (const MateDesktopItem     *item,
							      GtkIconTheme               *icon_theme);

char *                  mate_desktop_item_find_icon         (GtkIconTheme               *icon_theme,
							      const char                 *icon,
							      /* size is only a suggestion */
							      int                         desired_size,
							      int                         flags);

/*
 * Reading/Writing different sections, NULL is the standard section
 */
gboolean                mate_desktop_item_attr_exists       (const MateDesktopItem     *item,
							      const char                 *attr);

/*
 * String type
 */
const char *            mate_desktop_item_get_string        (const MateDesktopItem     *item,
							      const char		 *attr);

void                    mate_desktop_item_set_string        (MateDesktopItem           *item,
							      const char		 *attr,
							      const char                 *value);

const char *            mate_desktop_item_get_attr_locale   (const MateDesktopItem     *item,
							      const char		 *attr);

/*
 * LocaleString type
 */
const char *            mate_desktop_item_get_localestring  (const MateDesktopItem     *item,
							      const char		 *attr);
const char *            mate_desktop_item_get_localestring_lang (const MateDesktopItem *item,
								  const char		 *attr,
								  const char             *language);
/* use g_list_free only */
GList *                 mate_desktop_item_get_languages     (const MateDesktopItem     *item,
							      const char		 *attr);

void                    mate_desktop_item_set_localestring  (MateDesktopItem           *item,
							      const char		 *attr,
							      const char                 *value);
void                    mate_desktop_item_set_localestring_lang  (MateDesktopItem      *item,
								   const char		 *attr,
								   const char		 *language,
								   const char            *value);
void                    mate_desktop_item_clear_localestring(MateDesktopItem           *item,
							      const char		 *attr);

/*
 * Strings, Regexps types
 */

/* use mate_desktop_item_free_string_list */
char **                 mate_desktop_item_get_strings       (const MateDesktopItem     *item,
							      const char		 *attr);

void			mate_desktop_item_set_strings       (MateDesktopItem           *item,
							      const char                 *attr,
							      char                      **strings);

/*
 * Boolean type
 */
gboolean                mate_desktop_item_get_boolean       (const MateDesktopItem     *item,
							      const char		 *attr);

void                    mate_desktop_item_set_boolean       (MateDesktopItem           *item,
							      const char		 *attr,
							      gboolean                    value);

/*
 * Xserver time of user action that caused the application launch to start.
 */
void                    mate_desktop_item_set_launch_time   (MateDesktopItem           *item,
							      guint32                     timestamp);

/*
 * Clearing attributes
 */
#define                 mate_desktop_item_clear_attr(item,attr) \
				mate_desktop_item_set_string(item,attr,NULL)
void			mate_desktop_item_clear_section     (MateDesktopItem           *item,
							      const char                 *section);

G_END_DECLS

#endif /* MATE_DITEM_H */
