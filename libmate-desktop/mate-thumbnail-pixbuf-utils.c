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
#include "libmateui/mate-desktop-thumbnail.h"

#define LOAD_BUFFER_SIZE 65536

/**
 * mate_thumbnail_scale_down_pixbuf:
 * @pixbuf: a #GdkPixbuf
 * @dest_width: the desired new width
 * @dest_height: the desired new height
 *
 * Scales the pixbuf to the desired size. This function
 * is a lot faster than gdk-pixbuf when scaling down by
 * large amounts.
 *
 * Return value: a scaled pixbuf
 * 
 * Since: 2.2
 **/
GdkPixbuf *
mate_desktop_thumbnail_scale_down_pixbuf (GdkPixbuf *pixbuf,
					   int dest_width,
					   int dest_height)
{
	int source_width, source_height;
	int s_x1, s_y1, s_x2, s_y2;
	int s_xfrac, s_yfrac;
	int dx, dx_frac, dy, dy_frac;
	div_t ddx, ddy;
	int x, y;
	int r, g, b, a;
	int n_pixels;
	gboolean has_alpha;
	guchar *dest, *src, *xsrc, *src_pixels;
	GdkPixbuf *dest_pixbuf;
	int pixel_stride;
	int source_rowstride, dest_rowstride;

	if (dest_width == 0 || dest_height == 0) {
		return NULL;
	}
	
	source_width = gdk_pixbuf_get_width (pixbuf);
	source_height = gdk_pixbuf_get_height (pixbuf);

	g_assert (source_width >= dest_width);
	g_assert (source_height >= dest_height);

	ddx = div (source_width, dest_width);
	dx = ddx.quot;
	dx_frac = ddx.rem;
	
	ddy = div (source_height, dest_height);
	dy = ddy.quot;
	dy_frac = ddy.rem;

	has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
	source_rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	src_pixels = gdk_pixbuf_get_pixels (pixbuf);

	dest_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, has_alpha, 8,
				      dest_width, dest_height);
	dest = gdk_pixbuf_get_pixels (dest_pixbuf);
	dest_rowstride = gdk_pixbuf_get_rowstride (dest_pixbuf);

	pixel_stride = (has_alpha)?4:3;
	
	s_y1 = 0;
	s_yfrac = -dest_height/2;
	while (s_y1 < source_height) {
		s_y2 = s_y1 + dy;
		s_yfrac += dy_frac;
		if (s_yfrac > 0) {
			s_y2++;
			s_yfrac -= dest_height;
		}

		s_x1 = 0;
		s_xfrac = -dest_width/2;
		while (s_x1 < source_width) {
			s_x2 = s_x1 + dx;
			s_xfrac += dx_frac;
			if (s_xfrac > 0) {
				s_x2++;
				s_xfrac -= dest_width;
			}

			/* Average block of [x1,x2[ x [y1,y2[ and store in dest */
			r = g = b = a = 0;
			n_pixels = 0;

			src = src_pixels + s_y1 * source_rowstride + s_x1 * pixel_stride;
			for (y = s_y1; y < s_y2; y++) {
				xsrc = src;
				if (has_alpha) {
					for (x = 0; x < s_x2-s_x1; x++) {
						n_pixels++;
						
						r += xsrc[3] * xsrc[0];
						g += xsrc[3] * xsrc[1];
						b += xsrc[3] * xsrc[2];
						a += xsrc[3];
						xsrc += 4;
					}
				} else {
					for (x = 0; x < s_x2-s_x1; x++) {
						n_pixels++;
						r += *xsrc++;
						g += *xsrc++;
						b += *xsrc++;
					}
				}
				src += source_rowstride;
			}
			
			if (has_alpha) {
				if (a != 0) {
					*dest++ = r / a;
					*dest++ = g / a;
					*dest++ = b / a;
					*dest++ = a / n_pixels;
				} else {
					*dest++ = 0;
					*dest++ = 0;
					*dest++ = 0;
					*dest++ = 0;
				}
			} else {
				*dest++ = r / n_pixels;
				*dest++ = g / n_pixels;
				*dest++ = b / n_pixels;
			}
			
			s_x1 = s_x2;
		}
		s_y1 = s_y2;
		dest += dest_rowstride - dest_width * pixel_stride;
	}
	
	return dest_pixbuf;
}
