/*
 * mate-thumbnail-pixbuf-utils.c: Utilities for handling pixbufs when thumbnailing
 *
 * Copyright (C) 2002 Red Hat, Inc.
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
 * write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <glib.h>

#define MATE_DESKTOP_USE_UNSTABLE_API
#include "mate-desktop-thumbnail.h"

/**
 * mate_desktop_thumbnail_scale_down_pixbuf:
 * @pixbuf: a #GdkPixbuf
 * @dest_width: the desired new width
 * @dest_height: the desired new height
 *
 * Scales the pixbuf to the desired size. This function
 * used to be a lot faster than gdk-pixbuf when scaling
 * down by large amounts. This is not true anymore since
 * gdk-pixbuf UNRELEASED. You should use
 * gdk_pixbuf_scale_simple() instead, which this function
 * now does internally.
 *
 * Return value: (transfer full): a scaled pixbuf
 *
 * Since: 2.2
 **/
GdkPixbuf *
mate_desktop_thumbnail_scale_down_pixbuf (GdkPixbuf *pixbuf,
					   int dest_width,
					   int dest_height)
{
	g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);
	g_return_val_if_fail (gdk_pixbuf_get_width (pixbuf) >= dest_width, NULL);
	g_return_val_if_fail (gdk_pixbuf_get_height (pixbuf) >= dest_height, NULL);

	if (dest_width == 0 || dest_height == 0)
		return NULL;

	return gdk_pixbuf_scale_simple (pixbuf, dest_width, dest_height, GDK_INTERP_HYPER);
}
