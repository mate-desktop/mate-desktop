/* mate-bg.h -

   Copyright 2007, Red Hat, Inc.

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
   Boston, MA  02110-1301, USA.

   Author: Soren Sandmann <sandmann@redhat.com>
*/

#ifndef __MATE_BG_H__
#define __MATE_BG_H__

#ifndef MATE_DESKTOP_USE_UNSTABLE_API
#error    MateBG is unstable API. You must define MATE_DESKTOP_USE_UNSTABLE_API before including mate-bg.h
#endif

#include <gdk/gdk.h>
#include <gio/gio.h>
#include <libmateui/mate-desktop-thumbnail.h>
#include <libmateui/mate-bg-crossfade.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_TYPE_BG            (mate_bg_get_type ())
#define MATE_BG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_BG, MateBG))
#define MATE_BG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  MATE_TYPE_BG, MateBGClass))
#define MATE_IS_BG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_BG))
#define MATE_IS_BG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  MATE_TYPE_BG))
#define MATE_BG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  MATE_TYPE_BG, MateBGClass))

#define MATE_BG_SCHEMA "org.mate.background"

typedef struct _MateBG MateBG;
typedef struct _MateBGClass MateBGClass;

typedef enum {
	MATE_BG_COLOR_SOLID,
	MATE_BG_COLOR_H_GRADIENT,
	MATE_BG_COLOR_V_GRADIENT
} MateBGColorType;

typedef enum {
	MATE_BG_PLACEMENT_TILED,
	MATE_BG_PLACEMENT_ZOOMED,
	MATE_BG_PLACEMENT_CENTERED,
	MATE_BG_PLACEMENT_SCALED,
	MATE_BG_PLACEMENT_FILL_SCREEN,
	MATE_BG_PLACEMENT_SPANNED
} MateBGPlacement;

GType            mate_bg_get_type              (void);
MateBG *        mate_bg_new                   (void);
void             mate_bg_load_from_preferences (MateBG               *bg);
void             mate_bg_save_to_preferences   (MateBG               *bg);
/* Setters */
void             mate_bg_set_filename          (MateBG               *bg,
						 const char            *filename);
void             mate_bg_set_placement         (MateBG               *bg,
						 MateBGPlacement       placement);
void             mate_bg_set_color             (MateBG               *bg,
						 MateBGColorType       type,
						 GdkColor              *primary,
						 GdkColor              *secondary);
/* Getters */
MateBGPlacement mate_bg_get_placement         (MateBG               *bg);
void		 mate_bg_get_color             (MateBG               *bg,
						 MateBGColorType      *type,
						 GdkColor              *primary,
						 GdkColor              *secondary);
const gchar *    mate_bg_get_filename          (MateBG               *bg);

/* Drawing and thumbnailing */
void             mate_bg_draw                  (MateBG               *bg,
						 GdkPixbuf             *dest,
						 GdkScreen	       *screen,
                                                 gboolean               is_root);

#if GTK_CHECK_VERSION(3, 0, 0)
	cairo_surface_t* mate_bg_create_pixmap(MateBG* bg, GdkWindow* window, int width, int height, gboolean root);
#else
	GdkPixmap* mate_bg_create_pixmap(MateBG* bg, GdkWindow* window, int width, int height, gboolean root);
#endif

gboolean         mate_bg_get_image_size        (MateBG               *bg,
						 MateDesktopThumbnailFactory *factory,
                                                 int                    best_width,
                                                 int                    best_height,
						 int                   *width,
						 int                   *height);
GdkPixbuf *      mate_bg_create_thumbnail      (MateBG               *bg,
						 MateDesktopThumbnailFactory *factory,
						 GdkScreen             *screen,
						 int                    dest_width,
						 int                    dest_height);
gboolean         mate_bg_is_dark               (MateBG               *bg,
                                                 int                    dest_width,
						 int                    dest_height);
gboolean         mate_bg_has_multiple_sizes    (MateBG               *bg);
gboolean         mate_bg_changes_with_time     (MateBG               *bg);
GdkPixbuf *      mate_bg_create_frame_thumbnail (MateBG              *bg,
						 MateDesktopThumbnailFactory *factory,
						 GdkScreen             *screen,
						 int                    dest_width,
						 int                    dest_height,
						 int                    frame_num);

/* Set a pixmap as root - not a MateBG method. At some point
 * if we decide to stabilize the API then we may want to make
 * these object methods, drop mate_bg_create_pixmap, etc.
 */

#if GTK_CHECK_VERSION(3, 0, 0)
	void mate_bg_set_pixmap_as_root(GdkScreen* screen, cairo_surface_t* pixmap);
	MateBGCrossfade* mate_bg_set_pixmap_as_root_with_crossfade(GdkScreen* screen, cairo_surface_t* pixmap);
	cairo_surface_t* mate_bg_get_pixmap_from_root(GdkScreen* screen);
#else
	void mate_bg_set_pixmap_as_root(GdkScreen* screen, GdkPixmap* pixmap);
	MateBGCrossfade* mate_bg_set_pixmap_as_root_with_crossfade(GdkScreen* screen, GdkPixmap* pixmap);
	GdkPixmap* mate_bg_get_pixmap_from_root(GdkScreen* screen);
#endif

#ifdef __cplusplus
}
#endif

#endif
