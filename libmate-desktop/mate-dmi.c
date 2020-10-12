/*
 * Copyright (C) 2009-2011 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define MATE_DESKTOP_USE_UNSTABLE_API

#include <glib-object.h>
#include <math.h>
#include <string.h>
#include <gio/gio.h>
#include <stdlib.h>

#include "mate-dmi.h"

static void     mate_dmi_finalize        (GObject     *object);

struct _MateDmi
{
    GObject    parent;
    gchar      *name;
    gchar      *version;
    gchar      *vendor;
};

static gpointer mate_dmi_object = NULL;

G_DEFINE_TYPE (MateDmi, mate_dmi, G_TYPE_OBJECT)

static gchar * mate_dmi_get_from_filename (const gchar *filename)
{
    gboolean ret;
    GError *error = NULL;
    gchar *data = NULL;

    /* get the contents */
    ret = g_file_get_contents (filename, &data, NULL, &error);
    if (!ret) {
        if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
            g_warning ("failed to get contents of %s: %s", filename, error->message);
        g_error_free (error);
    }

    /* process the random chars and trailing spaces */
    if (data != NULL) {
        g_strdelimit (data, "\t_", ' ');
        g_strdelimit (data, "\n\r", '\0');
        g_strchomp (data);
    }

    /* don't return an empty string */
    if (data != NULL && data[0] == '\0') {
        g_free (data);
        data = NULL;
    }

    return data;
}

static gchar * mate_dmi_get_from_filenames (const gchar * const * filenames)
{
    guint i;
    gchar *tmp = NULL;

    /* try each one in preference order */
    for (i = 0; filenames[i] != NULL; i++) {
        tmp = mate_dmi_get_from_filename (filenames[i]);
        if (tmp != NULL)
            break;
    }
    return tmp;
}

const gchar * mate_dmi_get_name (MateDmi *dmi)
{
    g_return_val_if_fail (MATE_IS_DMI (dmi), NULL);
    return dmi->name;
}

const gchar * mate_dmi_get_version (MateDmi *dmi)
{
    g_return_val_if_fail (MATE_IS_DMI (dmi), NULL);
    return dmi->version;
}

const gchar * mate_dmi_get_vendor (MateDmi *dmi)
{
    g_return_val_if_fail (MATE_IS_DMI (dmi), NULL);
    return dmi->vendor;
}

static void mate_dmi_class_init (MateDmiClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize = mate_dmi_finalize;
}

static void mate_dmi_init (MateDmi *dmi)
{
#if defined(__linux__)
    const gchar *sysfs_name[] = {
        "/sys/class/dmi/id/product_name",
        "/sys/class/dmi/id/board_name",
        NULL};
    const gchar *sysfs_version[] = {
        "/sys/class/dmi/id/product_version",
        "/sys/class/dmi/id/chassis_version",
        "/sys/class/dmi/id/board_version",
        NULL};
    const gchar *sysfs_vendor[] = {
        "/sys/class/dmi/id/sys_vendor",
        "/sys/class/dmi/id/chassis_vendor",
        "/sys/class/dmi/id/board_vendor",
        NULL};
#else
#warning Please add dmi support for your OS
    const gchar *sysfs_name[] = { NULL };
    const gchar *sysfs_version[] = { NULL };
    const gchar *sysfs_vendor[] = { NULL };
#endif

    /* get all the possible data now */
    dmi->name = mate_dmi_get_from_filenames (sysfs_name);
    dmi->version = mate_dmi_get_from_filenames (sysfs_version);
    dmi->vendor = mate_dmi_get_from_filenames (sysfs_vendor);
}

static void mate_dmi_finalize (GObject *object)
{
    MateDmi *dmi = MATE_DMI (object);

    g_free (dmi->name);
    g_free (dmi->version);
    g_free (dmi->vendor);

    G_OBJECT_CLASS (mate_dmi_parent_class)->finalize (object);
}

MateDmi* mate_dmi_new (void)
{
    if (mate_dmi_object != NULL) {
        g_object_ref (mate_dmi_object);
    } else {
        mate_dmi_object = g_object_new (MATE_TYPE_DMI, NULL);
        g_object_add_weak_pointer (mate_dmi_object, &mate_dmi_object);
    }
    return MATE_DMI (mate_dmi_object);
}
