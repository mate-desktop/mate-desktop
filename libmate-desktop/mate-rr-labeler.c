/* mate-rr-labeler.c - Utility to label monitors to identify them
 * while they are being configured.
 *
 * Copyright 2008, Novell, Inc.
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
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Federico Mena-Quintero <federico@novell.com>
 */

#define MATE_DESKTOP_USE_UNSTABLE_API

#include <config.h>
#include <glib/gi18n-lib.h>
#include "libmateui/mate-rr-labeler.h"
#include <gtk/gtk.h>

struct _MateRRLabeler {
	GObject parent;

	MateRRConfig *config;

	int num_outputs;

	GdkColor *palette;
	GtkWidget **windows;
};

struct _MateRRLabelerClass {
	GObjectClass parent_class;
};

G_DEFINE_TYPE (MateRRLabeler, mate_rr_labeler, G_TYPE_OBJECT);

static void mate_rr_labeler_finalize (GObject *object);

static void
mate_rr_labeler_init (MateRRLabeler *labeler)
{
	/* nothing */
}

static void
mate_rr_labeler_class_init (MateRRLabelerClass *class)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *) class;

	object_class->finalize = mate_rr_labeler_finalize;
}

static void
mate_rr_labeler_finalize (GObject *object)
{
	MateRRLabeler *labeler;

	labeler = MATE_RR_LABELER (object);

	/* We don't destroy the labeler->config (a MateRRConfig*) here; let our
	 * caller do that instead.
	 */

	if (labeler->windows != NULL) {
		mate_rr_labeler_hide (labeler);
		g_free (labeler->windows);
		labeler->windows = NULL;
	}

	g_free (labeler->palette);
	labeler->palette = NULL;

	G_OBJECT_CLASS (mate_rr_labeler_parent_class)->finalize (object);
}

static int
count_outputs (MateRRConfig *config)
{
	int i;

	for (i = 0; config->outputs[i] != NULL; i++)
		;

	return i;
}

static void
make_palette (MateRRLabeler *labeler)
{
	/* The idea is that we go around an hue color wheel.  We want to start
	 * at red, go around to green/etc. and stop at blue --- because magenta
	 * is evil.  Eeeeek, no magenta, please!
	 *
	 * Purple would be nice, though.  Remember that we are watered down
	 * (i.e. low saturation), so that would be like Like berries with cream.
	 * Mmmmm, berries.
	 */
	double start_hue;
	double end_hue;
	int i;

	g_assert (labeler->num_outputs > 0);

	labeler->palette = g_new (GdkColor, labeler->num_outputs);

	start_hue = 0.0; /* red */
	end_hue   = 2.0/3; /* blue */

	for (i = 0; i < labeler->num_outputs; i++) {
		double h, s, v;
		double r, g, b;

		h = start_hue + (end_hue - start_hue) / labeler->num_outputs * i;
		s = 1.0 / 3;
		v = 1.0;

		gtk_hsv_to_rgb (h, s, v, &r, &g, &b);

		labeler->palette[i].red   = (int) (65535 * r + 0.5);
		labeler->palette[i].green = (int) (65535 * g + 0.5);
		labeler->palette[i].blue  = (int) (65535 * b + 0.5);
	}
}

#define LABEL_WINDOW_EDGE_THICKNESS 2
#define LABEL_WINDOW_PADDING 12

static gboolean
#if GTK_CHECK_VERSION (3, 0, 0)
label_window_draw_event_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
#else
label_window_expose_event_cb (GtkWidget *widget, GdkEventExpose *event, gpointer data)
#endif
{
	GdkColor *color;
	GtkAllocation allocation;

	color = g_object_get_data (G_OBJECT (widget), "color");
	gtk_widget_get_allocation (widget, &allocation);

#if !GTK_CHECK_VERSION (3, 0, 0)
	cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
#endif

	/* edge outline */

	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_rectangle (cr,
			 LABEL_WINDOW_EDGE_THICKNESS / 2.0,
			 LABEL_WINDOW_EDGE_THICKNESS / 2.0,
			 allocation.width - LABEL_WINDOW_EDGE_THICKNESS,
			 allocation.height - LABEL_WINDOW_EDGE_THICKNESS);
	cairo_set_line_width (cr, LABEL_WINDOW_EDGE_THICKNESS);
	cairo_stroke (cr);

	/* fill */

	gdk_cairo_set_source_color (cr, color);
	cairo_rectangle (cr,
			 LABEL_WINDOW_EDGE_THICKNESS,
			 LABEL_WINDOW_EDGE_THICKNESS,
			 allocation.width - LABEL_WINDOW_EDGE_THICKNESS * 2,
			 allocation.height - LABEL_WINDOW_EDGE_THICKNESS * 2);
	cairo_fill (cr);

#if !GTK_CHECK_VERSION (3, 0, 0)
	cairo_destroy (cr);
#endif

	return FALSE;
}

static GtkWidget *
create_label_window (MateRRLabeler *labeler, MateOutputInfo *output, GdkColor *color)
{
	GtkWidget *window;
	GtkWidget *widget;
	char *str;
	const char *display_name;
	GdkColor black = { 0, 0, 0, 0 };

	window = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_widget_set_app_paintable (window, TRUE);

	gtk_container_set_border_width (GTK_CONTAINER (window), LABEL_WINDOW_PADDING + LABEL_WINDOW_EDGE_THICKNESS);

	/* This is semi-dangerous.  The color is part of the labeler->palette
	 * array.  Note that in mate_rr_labeler_finalize(), we are careful to
	 * free the palette only after we free the windows.
	 */
	g_object_set_data (G_OBJECT (window), "color", color);

#if GTK_CHECK_VERSION (3, 0, 0)
	g_signal_connect (window, "draw",
			  G_CALLBACK (label_window_draw_event_cb), labeler);
#else
	g_signal_connect (window, "expose-event",
			  G_CALLBACK (label_window_expose_event_cb), labeler);
#endif

	if (labeler->config->clone) {
		/* Keep this string in sync with mate-control-center/capplets/display/xrandr-capplet.c:get_display_name() */

		/* Translators:  this is the feature where what you see on your laptop's
		 * screen is the same as your external monitor.  Here, "Mirror" is being
		 * used as an adjective, not as a verb.  For example, the Spanish
		 * translation could be "Pantallas en Espejo", *not* "Espejar Pantallas".
		 */
		display_name = _("Mirror Screens");
	} else
		display_name = output->display_name;

	str = g_strdup_printf ("<b>%s</b>", display_name);
	widget = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (widget), str);
	g_free (str);

	/* Make the label explicitly black.  We don't want it to follow the
	 * theme's colors, since the label is always shown against a light
	 * pastel background.  See bgo#556050
	 */
	gtk_widget_modify_fg (widget, gtk_widget_get_state (widget), &black);

	gtk_container_add (GTK_CONTAINER (window), widget);

	/* Should we center this at the top edge of the monitor, instead of using the upper-left corner? */
	gtk_window_move (GTK_WINDOW (window), output->x, output->y);

	gtk_widget_show_all (window);

	return window;
}

static void
create_label_windows (MateRRLabeler *labeler)
{
	int i;
	gboolean created_window_for_clone;

	labeler->windows = g_new (GtkWidget *, labeler->num_outputs);

	created_window_for_clone = FALSE;

	for (i = 0; i < labeler->num_outputs; i++) {
		if (!created_window_for_clone && labeler->config->outputs[i]->on) {
			labeler->windows[i] = create_label_window (labeler, labeler->config->outputs[i], labeler->palette + i);

			if (labeler->config->clone)
				created_window_for_clone = TRUE;
		} else
			labeler->windows[i] = NULL;
	}
}

static void
setup_from_config (MateRRLabeler *labeler)
{
	labeler->num_outputs = count_outputs (labeler->config);

	make_palette (labeler);

	create_label_windows (labeler);
}

MateRRLabeler *
mate_rr_labeler_new (MateRRConfig *config)
{
	MateRRLabeler *labeler;

	g_return_val_if_fail (config != NULL, NULL);

	labeler = g_object_new (MATE_TYPE_RR_LABELER, NULL);
	labeler->config = config;

	setup_from_config (labeler);

	return labeler;
}

void
mate_rr_labeler_hide (MateRRLabeler *labeler)
{
	int i;

	g_return_if_fail (MATE_IS_RR_LABELER (labeler));

	if (labeler->windows == NULL)
		return;

	for (i = 0; i < labeler->num_outputs; i++)
		if (labeler->windows[i] != NULL) {
			gtk_widget_destroy (labeler->windows[i]);
			labeler->windows[i] = NULL;
		}
	g_free (labeler->windows);
	labeler->windows = NULL;

}

void
mate_rr_labeler_get_color_for_output (MateRRLabeler *labeler, MateOutputInfo *output, GdkColor *color_out)
{
	int i;

	g_return_if_fail (MATE_IS_RR_LABELER (labeler));
	g_return_if_fail (output != NULL);
	g_return_if_fail (color_out != NULL);

	for (i = 0; i < labeler->num_outputs; i++)
		if (labeler->config->outputs[i] == output) {
			*color_out = labeler->palette[i];
			return;
		}

	g_warning ("trying to get the color for unknown MateOutputInfo %p; returning magenta!", output);

	color_out->red   = 0xffff;
	color_out->green = 0;
	color_out->blue  = 0xffff;
}
