/* mate-bg-crossfade.h - fade window background between two pixmaps
 *
 * Copyright (C) 2008 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Author: Ray Strode <rstrode@redhat.com>
*/
#include <string.h>
#include <math.h>
#include <stdarg.h>

#include <gio/gio.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gtk/gtk.h>

#include <cairo.h>
#include <cairo-xlib.h>

#define MATE_DESKTOP_USE_UNSTABLE_API
#include <libmateui/mate-bg.h>
#include "libmateui/mate-bg-crossfade.h"

struct _MateBGCrossfadePrivate
{
	GdkWindow *window;
	int        width;
	int        height;

	#if GTK_CHECK_VERSION(3, 0, 0)
		cairo_surface_t* fading_pixmap;
		cairo_surface_t* end_pixmap;
	#else
		GdkPixmap* fading_pixmap;
		GdkPixmap* end_pixmap;
	#endif

	gdouble    start_time;
	gdouble    total_duration;
	guint      timeout_id;
	guint      is_first_frame : 1;
};

enum {
	PROP_0,
	PROP_WIDTH,
	PROP_HEIGHT,
};

enum {
	FINISHED,
	NUMBER_OF_SIGNALS
};

static guint signals[NUMBER_OF_SIGNALS] = { 0 };

G_DEFINE_TYPE (MateBGCrossfade, mate_bg_crossfade, G_TYPE_OBJECT)
#define MATE_BG_CROSSFADE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o),\
			                   MATE_TYPE_BG_CROSSFADE,\
			                   MateBGCrossfadePrivate))

static void
mate_bg_crossfade_set_property (GObject      *object,
				 guint         property_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
	MateBGCrossfade *fade;

	g_assert (MATE_IS_BG_CROSSFADE (object));

	fade = MATE_BG_CROSSFADE (object);

	switch (property_id)
	{
	case PROP_WIDTH:
		fade->priv->width = g_value_get_int (value);
		break;
	case PROP_HEIGHT:
		fade->priv->height = g_value_get_int (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
mate_bg_crossfade_get_property (GObject    *object,
			     guint       property_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
	MateBGCrossfade *fade;

	g_assert (MATE_IS_BG_CROSSFADE (object));

	fade = MATE_BG_CROSSFADE (object);

	switch (property_id)
	{
	case PROP_WIDTH:
		g_value_set_int (value, fade->priv->width);
		break;
	case PROP_HEIGHT:
		g_value_set_int (value, fade->priv->height);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
mate_bg_crossfade_finalize (GObject *object)
{
	MateBGCrossfade *fade;

	fade = MATE_BG_CROSSFADE (object);

	mate_bg_crossfade_stop (fade);

	if (fade->priv->fading_pixmap != NULL) {
		g_object_unref (fade->priv->fading_pixmap);
		fade->priv->fading_pixmap = NULL;
	}

	if (fade->priv->end_pixmap != NULL) {
		g_object_unref (fade->priv->end_pixmap);
		fade->priv->end_pixmap = NULL;
	}
}

static void
mate_bg_crossfade_class_init (MateBGCrossfadeClass *fade_class)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (fade_class);

	gobject_class->get_property = mate_bg_crossfade_get_property;
	gobject_class->set_property = mate_bg_crossfade_set_property;
	gobject_class->finalize = mate_bg_crossfade_finalize;

	/**
	 * MateBGCrossfade:width:
	 *
	 * When a crossfade is running, this is width of the fading
	 * pixmap.
	 */
	g_object_class_install_property (gobject_class,
					 PROP_WIDTH,
					 g_param_spec_int ("width",
						           "Window Width",
							    "Width of window to fade",
							    0, G_MAXINT, 0,
							    G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

	/**
	 * MateBGCrossfade:height:
	 *
	 * When a crossfade is running, this is height of the fading
	 * pixmap.
	 */
	g_object_class_install_property (gobject_class,
					 PROP_HEIGHT,
					 g_param_spec_int ("height", "Window Height",
						           "Height of window to fade on",
							   0, G_MAXINT, 0,
							   G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

	/**
	 * MateBGCrossfade::finished:
	 * @fade: the #MateBGCrossfade that received the signal
	 * @window: the #GdkWindow the crossfade happend on.
	 *
	 * When a crossfade finishes, @window will have a copy
	 * of the end pixmap as its background, and this signal will
	 * get emitted.
	 */
	signals[FINISHED] = g_signal_new ("finished",
					  G_OBJECT_CLASS_TYPE (gobject_class),
					  G_SIGNAL_RUN_LAST, 0, NULL, NULL,
					  g_cclosure_marshal_VOID__OBJECT,
					  G_TYPE_NONE, 1, G_TYPE_OBJECT);

	g_type_class_add_private (gobject_class, sizeof (MateBGCrossfadePrivate));
}

static void
mate_bg_crossfade_init (MateBGCrossfade *fade)
{
	fade->priv = MATE_BG_CROSSFADE_GET_PRIVATE (fade);

	fade->priv->fading_pixmap = NULL;
	fade->priv->end_pixmap = NULL;
	fade->priv->timeout_id = 0;
}

/**
 * mate_bg_crossfade_new:
 * @width: The width of the crossfading window
 * @height: The height of the crossfading window
 *
 * Creates a new object to manage crossfading a
 * window background between two #GdkPixmap drawables.
 *
 * Return value: the new #MateBGCrossfade
 **/
MateBGCrossfade* mate_bg_crossfade_new(int width, int height)
{
	GObject* object;

	object = g_object_new(MATE_TYPE_BG_CROSSFADE,
		"width", width,
		"height", height,
		NULL);

	return (MateBGCrossfade*) object;
}

#if GTK_CHECK_VERSION(3, 0, 0)
	static cairo_surface_t* tile_pixmap(cairo_surface_t* pixmap, int width, int height)
#else
	static GdkPixmap* tile_pixmap(GdkPixmap* pixmap, int width, int height)
#endif
{
	#if GTK_CHECK_VERSION(3, 0, 0)
		cairo_surface_t* copy;
	#else
		GdkPixmap* copy;
	#endif

	cairo_t *cr;

	copy = gdk_pixmap_new(pixmap, width, height, pixmap == NULL? 24 : -1);

	cr = gdk_cairo_create(copy);

	if (pixmap != NULL)
	{
		cairo_pattern_t *pattern;
		gdk_cairo_set_source_pixmap (cr, pixmap, 0.0, 0.0);
		pattern = cairo_get_source (cr);
		cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
	}
	else
	{
		GtkStyle *style;
		style = gtk_widget_get_default_style ();
		gdk_cairo_set_source_color(cr, &style->bg[GTK_STATE_NORMAL]);
	}

	cairo_paint (cr);

	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
	{
		g_object_unref (copy);
		copy = NULL;
	}

	cairo_destroy(cr);

	return copy;
}

/**
 * mate_bg_crossfade_set_start_pixmap:
 * @fade: a #MateBGCrossfade
 * @pixmap: The #GdkPixmap to fade from
 *
 * Before initiating a crossfade with mate_bg_crossfade_start()
 * a start and end pixmap have to be set.  This function sets
 * the pixmap shown at the beginning of the crossfade effect.
 *
 * Return value: %TRUE if successful, or %FALSE if the pixmap
 * could not be copied.
 **/
#if GTK_CHECK_VERSION(3, 0, 0)
	gboolean mate_bg_crossfade_set_start_pixmap(MateBGCrossfade* fade, cairo_surface_t* pixmap)
#else
	gboolean mate_bg_crossfade_set_start_pixmap(MateBGCrossfade* fade, GdkPixmap* pixmap)
#endif
{
/* I am disabling this because background fade break the mate-file-manager
 * Estoy deshabilitando esto por que el efecto de desvanecimiento del fondo de pantalla, hace
 * el mate-file-manager se cierre por un evento de BadDrawing enviado por Xorg.
 * Y hasta que no sea solucionado, no es tan indispensable esta funcion.
 * Pero sospecho que la funcion tile_pixmap tiene algo que ver...
 */
return FALSE;

	g_return_val_if_fail (MATE_IS_BG_CROSSFADE (fade), FALSE);

	if (fade->priv->fading_pixmap != NULL) {
		g_object_unref (fade->priv->fading_pixmap);
		fade->priv->fading_pixmap = NULL;
	}

	fade->priv->fading_pixmap = tile_pixmap (pixmap,
						 fade->priv->width,
						 fade->priv->height);

	return fade->priv->fading_pixmap != NULL;
}

static gdouble
get_current_time (void)
{
	const double microseconds_per_second = (double) G_USEC_PER_SEC;
	double timestamp;
	GTimeVal now;

	g_get_current_time (&now);

	timestamp = ((microseconds_per_second * now.tv_sec) + now.tv_usec) /
	            microseconds_per_second;

	return timestamp;
}

/**
 * mate_bg_crossfade_set_end_pixmap:
 * @fade: a #MateBGCrossfade
 * @pixmap: The #GdkPixmap to fade to
 *
 * Before initiating a crossfade with mate_bg_crossfade_start()
 * a start and end pixmap have to be set.  This function sets
 * the pixmap shown at the end of the crossfade effect.
 *
 * Return value: %TRUE if successful, or %FALSE if the pixmap
 * could not be copied.
 **/
#if GTK_CHECK_VERSION(3, 0, 0)
	gboolean mate_bg_crossfade_set_end_pixmap(MateBGCrossfade* fade, cairo_surface_t* pixmap)
#else
	gboolean mate_bg_crossfade_set_end_pixmap(MateBGCrossfade* fade, GdkPixmap* pixmap)
#endif
{
	g_return_val_if_fail (MATE_IS_BG_CROSSFADE (fade), FALSE);

	if (fade->priv->end_pixmap != NULL) {
		g_object_unref (fade->priv->end_pixmap);
		fade->priv->end_pixmap = NULL;
	}

	fade->priv->end_pixmap = tile_pixmap (pixmap,
					      fade->priv->width,
					      fade->priv->height);

	/* Reset timer in case we're called while animating
	 */
	fade->priv->start_time = get_current_time ();
	return fade->priv->end_pixmap != NULL;
}

static gboolean
animations_are_disabled (MateBGCrossfade *fade)
{
	GtkSettings *settings;
	GdkScreen *screen;
	gboolean are_enabled;

	g_assert (fade->priv->window != NULL);

	#if GTK_CHECK_VERSION(2, 24, 0)
		screen = gdk_window_get_screen(fade->priv->window);
	#else // since 2.2
		screen = gdk_drawable_get_screen(GDK_DRAWABLE(fade->priv->window));
	#endif

	settings = gtk_settings_get_for_screen (screen);

	g_object_get (settings, "gtk-enable-animations", &are_enabled, NULL);

	return !are_enabled;
}

static void
draw_background (MateBGCrossfade *fade)
{
	if (GDK_WINDOW_TYPE (fade->priv->window) == GDK_WINDOW_ROOT) {
		GdkDisplay *display;
		display = gdk_drawable_get_display (fade->priv->window);
		gdk_window_clear (fade->priv->window);
		gdk_flush ();
	} else {
		gdk_window_invalidate_rect (fade->priv->window, NULL, FALSE);
		gdk_window_process_updates (fade->priv->window, FALSE);
	}
}

static gboolean
on_tick (MateBGCrossfade *fade)
{
	gdouble now, percent_done;
	cairo_t *cr;
	cairo_status_t status;

	g_return_val_if_fail (MATE_IS_BG_CROSSFADE (fade), FALSE);

	now = get_current_time ();

	percent_done = (now - fade->priv->start_time) / fade->priv->total_duration;
	percent_done = CLAMP (percent_done, 0.0, 1.0);

	/* If it's taking a long time to get to the first frame,
	 * then lengthen the duration, so the user will get to see
	 * the effect.
	 */
	if (fade->priv->is_first_frame && percent_done > .33) {
		fade->priv->is_first_frame = FALSE;
		fade->priv->total_duration *= 1.5;
		return on_tick (fade);
	}

	if (fade->priv->fading_pixmap == NULL) {
		return FALSE;
	}

	if (animations_are_disabled (fade)) {
		return FALSE;
	}

	/* We accumulate the results in place for performance reasons.
	 *
	 * This means 1) The fade is exponential, not linear (looks good!)
	 * 2) The rate of fade is not independent of frame rate. Slower machines
	 * will get a slower fade (but never longer than .75 seconds), and
	 * even the fastest machines will get *some* fade because the framerate
	 * is capped.
	 */
	cr = gdk_cairo_create (fade->priv->fading_pixmap);

	gdk_cairo_set_source_pixmap (cr, fade->priv->end_pixmap,
				     0.0, 0.0);
	cairo_paint_with_alpha (cr, percent_done);

	status = cairo_status (cr);
	cairo_destroy (cr);

	if (status == CAIRO_STATUS_SUCCESS) {
		draw_background (fade);
	}
	return percent_done <= .99;
}

static void
on_finished (MateBGCrossfade *fade)
{
	if (fade->priv->timeout_id == 0)
		return;

	g_assert (fade->priv->end_pixmap != NULL);

	gdk_window_set_back_pixmap (fade->priv->window,
				    fade->priv->end_pixmap,
				    FALSE);
	draw_background (fade);

	g_object_unref (fade->priv->end_pixmap);
	fade->priv->end_pixmap = NULL;

	g_assert (fade->priv->fading_pixmap != NULL);

	g_object_unref (fade->priv->fading_pixmap);
	fade->priv->fading_pixmap = NULL;

	fade->priv->timeout_id = 0;
	g_signal_emit (fade, signals[FINISHED], 0, fade->priv->window);
}

/**
 * mate_bg_crossfade_start:
 * @fade: a #MateBGCrossfade
 * @window: The #GdkWindow to draw crossfade on
 *
 * This function initiates a quick crossfade between two pixmaps on
 * the background of @window.  Before initiating the crossfade both
 * mate_bg_crossfade_start() and mate_bg_crossfade_end() need to
 * be called. If animations are disabled, the crossfade is skipped,
 * and the window background is set immediately to the end pixmap.
 **/
void
mate_bg_crossfade_start (MateBGCrossfade *fade,
			  GdkWindow        *window)
{
	GSource *source;
	GMainContext *context;

	g_return_if_fail (MATE_IS_BG_CROSSFADE (fade));
	g_return_if_fail (window != NULL);
	g_return_if_fail (fade->priv->fading_pixmap != NULL);
	g_return_if_fail (fade->priv->end_pixmap != NULL);
	g_return_if_fail (!mate_bg_crossfade_is_started (fade));
	g_return_if_fail (GDK_WINDOW_TYPE (window) != GDK_WINDOW_FOREIGN);

	source = g_timeout_source_new (1000 / 60.0);
	g_source_set_callback (source,
			       (GSourceFunc) on_tick,
			       fade,
			       (GDestroyNotify) on_finished);
	context = g_main_context_default ();
	fade->priv->timeout_id = g_source_attach (source, context);
	g_source_unref (source);

	fade->priv->window = window;
	gdk_window_set_back_pixmap (fade->priv->window,
				    fade->priv->fading_pixmap,
				    FALSE);
	draw_background (fade);

	fade->priv->is_first_frame = TRUE;
	fade->priv->total_duration = .75;
	fade->priv->start_time = get_current_time ();
}


/**
 * mate_bg_crossfade_is_started:
 * @fade: a #MateBGCrossfade
 *
 * This function reveals whether or not @fade is currently
 * running on a window.  See mate_bg_crossfade_start() for
 * information on how to initiate a crossfade.
 *
 * Return value: %TRUE if fading, or %FALSE if not fading
 **/
gboolean
mate_bg_crossfade_is_started (MateBGCrossfade *fade)
{
	g_return_val_if_fail (MATE_IS_BG_CROSSFADE (fade), FALSE);

	return fade->priv->timeout_id != 0;
}

/**
 * mate_bg_crossfade_stop:
 * @fade: a #MateBGCrossfade
 *
 * This function stops any in progress crossfades that may be
 * happening.  It's harmless to call this function if @fade is
 * already stopped.
 **/
void
mate_bg_crossfade_stop (MateBGCrossfade *fade)
{
	g_return_if_fail (MATE_IS_BG_CROSSFADE (fade));

	if (!mate_bg_crossfade_is_started (fade))
		return;

	g_assert (fade->priv->timeout_id != 0);
	g_source_remove (fade->priv->timeout_id);
	fade->priv->timeout_id = 0;
}
