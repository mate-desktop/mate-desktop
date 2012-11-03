/* mate-bg-crossfade.h - fade window background between two pixmaps

   Copyright 2008, Red Hat, Inc.

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
   write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth
   Floor, Boston, MA 02110-1301 US.

   Author: Ray Strode <rstrode@redhat.com>
*/

#ifndef __MATE_BG_CROSSFADE_H__
#define __MATE_BG_CROSSFADE_H__

#ifndef MATE_DESKTOP_USE_UNSTABLE_API
#error    MateBGCrossfade is unstable API. You must define MATE_DESKTOP_USE_UNSTABLE_API before including mate-bg-crossfade.h
#endif

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_TYPE_BG_CROSSFADE            (mate_bg_crossfade_get_type ())
#define MATE_BG_CROSSFADE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_BG_CROSSFADE, MateBGCrossfade))
#define MATE_BG_CROSSFADE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  MATE_TYPE_BG_CROSSFADE, MateBGCrossfadeClass))
#define MATE_IS_BG_CROSSFADE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_BG_CROSSFADE))
#define MATE_IS_BG_CROSSFADE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  MATE_TYPE_BG_CROSSFADE))
#define MATE_BG_CROSSFADE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  MATE_TYPE_BG_CROSSFADE, MateBGCrossfadeClass))

typedef struct _MateBGCrossfadePrivate MateBGCrossfadePrivate;
typedef struct _MateBGCrossfade MateBGCrossfade;
typedef struct _MateBGCrossfadeClass MateBGCrossfadeClass;

struct _MateBGCrossfade
{
	GObject parent_object;

	MateBGCrossfadePrivate *priv;
};

struct _MateBGCrossfadeClass
{
	GObjectClass parent_class;

	void (* finished) (MateBGCrossfade *fade, GdkWindow *window);
};

GType             mate_bg_crossfade_get_type              (void);
MateBGCrossfade *mate_bg_crossfade_new (int width, int height);


#if GTK_CHECK_VERSION(3, 0, 0)
	gboolean mate_bg_crossfade_set_start_pixmap(MateBGCrossfade* fade, cairo_surface_t* pixmap);
	gboolean mate_bg_crossfade_set_end_pixmap(MateBGCrossfade* fade, cairo_surface_t* pixmap);
#else
	gboolean mate_bg_crossfade_set_start_pixmap(MateBGCrossfade* fade, GdkPixmap* pixmap);
	gboolean mate_bg_crossfade_set_end_pixmap(MateBGCrossfade* fade, GdkPixmap* pixmap);
#endif

void              mate_bg_crossfade_start (MateBGCrossfade *fade,
                                            GdkWindow        *window);
gboolean          mate_bg_crossfade_is_started (MateBGCrossfade *fade);
void              mate_bg_crossfade_stop (MateBGCrossfade *fade);

#ifdef __cplusplus
}
#endif

#endif
