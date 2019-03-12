/* HSV color selector for GTK+
 *
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Simon Budig <Simon.Budig@unix-ag.org> (original code)
 *          Federico Mena-Quintero <federico@gimp.org> (cleanup for GTK+)
 *          Jonathan Blandford <jrb@redhat.com> (cleanup for GTK+)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 *
 * Modified to work internally in mate-desktop by Pablo Barciela 2019
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mate-hsv.h"

#include <math.h>
#include <string.h>

#include <gtk/gtk.h>

#define I_(string) g_intern_static_string (string)

/**
 * SECTION:mate-hsv
 * @Short_description: A “color wheel” widget
 * @Title: MateHSV
 *
 * #MateHSV is the “color wheel” part of a complete color selector widget.
 * It allows to select a color by determining its HSV components in an
 * intuitive way. Moving the selection around the outer ring changes the hue,
 * and moving the selection point inside the inner triangle changes value and
 * saturation.
 */

/* Default width/height */
#define DEFAULT_SIZE 100

/* Default ring width */
#define DEFAULT_RING_WIDTH 10


/* Dragging modes */
typedef enum {
  DRAG_NONE,
  DRAG_H,
  DRAG_SV
} DragMode;

/* Private part of the MateHSV structure */
struct _MateHSVPrivate
{
  /* Color value */
  double h;
  double s;
  double v;
  
  /* Size and ring width */
  int size;
  int ring_width;
  
  /* Window for capturing events */
  GdkWindow *window;
  
  /* Dragging mode */
  DragMode mode;

  guint focus_on_ring : 1;
};


/* Signal IDs */

enum {
  CHANGED,
  MOVE,
  LAST_SIGNAL
};

static void     mate_hsv_destroy              (GtkWidget          *widget);
static void     mate_hsv_realize              (GtkWidget          *widget);
static void     mate_hsv_unrealize            (GtkWidget          *widget);
static void     mate_hsv_get_preferred_width  (GtkWidget          *widget,
                                               gint               *minimum,
                                               gint               *natural);
static void     mate_hsv_get_preferred_height (GtkWidget          *widget,
                                               gint               *minimum,
                                               gint               *natural);
static void     mate_hsv_size_allocate        (GtkWidget          *widget,
                                               GtkAllocation      *allocation);
static gboolean mate_hsv_button_press         (GtkWidget          *widget,
                                               GdkEventButton     *event);
static gboolean mate_hsv_button_release       (GtkWidget          *widget,
                                               GdkEventButton     *event);
static gboolean mate_hsv_motion               (GtkWidget          *widget,
                                               GdkEventMotion     *event);
static gboolean mate_hsv_draw                 (GtkWidget          *widget,
                                               cairo_t            *cr);
static gboolean mate_hsv_grab_broken          (GtkWidget          *widget,
                                               GdkEventGrabBroken *event);
static gboolean mate_hsv_focus                (GtkWidget          *widget,
                                               GtkDirectionType    direction);
static void     mate_hsv_move                 (MateHSV            *hsv,
                                               GtkDirectionType    dir);

static guint hsv_signals[LAST_SIGNAL];

G_DEFINE_TYPE_WITH_PRIVATE (MateHSV, mate_hsv, GTK_TYPE_WIDGET)

/* Class initialization function for the HSV color selector */
static void
mate_hsv_class_init (MateHSVClass *class)
{
  GObjectClass    *gobject_class;
  GtkWidgetClass  *widget_class;
  MateHSVClass    *hsv_class;
  GtkBindingSet   *binding_set;

  gobject_class = (GObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;
  hsv_class = MATE_HSV_CLASS (class);

  widget_class->destroy = mate_hsv_destroy;
  widget_class->realize = mate_hsv_realize;
  widget_class->unrealize = mate_hsv_unrealize;
  widget_class->get_preferred_width = mate_hsv_get_preferred_width;
  widget_class->get_preferred_height = mate_hsv_get_preferred_height;
  widget_class->size_allocate = mate_hsv_size_allocate;
  widget_class->button_press_event = mate_hsv_button_press;
  widget_class->button_release_event = mate_hsv_button_release;
  widget_class->motion_notify_event = mate_hsv_motion;
  widget_class->draw = mate_hsv_draw;
  widget_class->focus = mate_hsv_focus;
  widget_class->grab_broken_event = mate_hsv_grab_broken;

  gtk_widget_class_set_accessible_role (widget_class, ATK_ROLE_COLOR_CHOOSER);

  hsv_class->move = mate_hsv_move;

  hsv_signals[CHANGED] =
    g_signal_new (I_("changed"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (MateHSVClass, changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  hsv_signals[MOVE] =
    g_signal_new (I_("move"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (MateHSVClass, move),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  GTK_TYPE_DIRECTION_TYPE);

  binding_set = gtk_binding_set_by_class (class);

  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Up, 0,
                                "move", 1,
                                G_TYPE_ENUM, GTK_DIR_UP);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Up, 0,
                                "move", 1,
                                G_TYPE_ENUM, GTK_DIR_UP);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Down, 0,
                                "move", 1,
                                G_TYPE_ENUM, GTK_DIR_DOWN);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Down, 0,
                                "move", 1,
                                G_TYPE_ENUM, GTK_DIR_DOWN);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Right, 0,
                                "move", 1,
                                G_TYPE_ENUM, GTK_DIR_RIGHT);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Right, 0,
                                "move", 1,
                                G_TYPE_ENUM, GTK_DIR_RIGHT);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Left, 0,
                                "move", 1,
                                G_TYPE_ENUM, GTK_DIR_LEFT);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Left, 0,
                                "move", 1,
                                G_TYPE_ENUM, GTK_DIR_LEFT);
}

static void
mate_hsv_init (MateHSV *hsv)
{
  MateHSVPrivate *priv;

  priv = mate_hsv_get_instance_private (hsv);
  hsv->priv = priv;

  gtk_widget_set_has_window (GTK_WIDGET (hsv), FALSE);
  gtk_widget_set_can_focus (GTK_WIDGET (hsv), TRUE);

  priv->h = 0.0;
  priv->s = 0.0;
  priv->v = 0.0;

  priv->size = DEFAULT_SIZE;
  priv->ring_width = DEFAULT_RING_WIDTH;
}

static void
mate_hsv_destroy (GtkWidget *widget)
{
  GTK_WIDGET_CLASS (mate_hsv_parent_class)->destroy (widget);
}

static void
mate_hsv_realize (GtkWidget *widget)
{
  MateHSV *hsv = MATE_HSV (widget);
  MateHSVPrivate *priv = hsv->priv;
  GtkAllocation allocation;
  GdkWindow *parent_window;
  GdkWindowAttr attr;
  int attr_mask;

  gtk_widget_set_realized (widget, TRUE);

  gtk_widget_get_allocation (widget, &allocation);

  attr.window_type = GDK_WINDOW_CHILD;
  attr.x = allocation.x;
  attr.y = allocation.y;
  attr.width = allocation.width;
  attr.height = allocation.height;
  attr.wclass = GDK_INPUT_ONLY;
  attr.event_mask = gtk_widget_get_events (widget);
  attr.event_mask |= (GDK_KEY_PRESS_MASK
                      | GDK_BUTTON_PRESS_MASK
                      | GDK_BUTTON_RELEASE_MASK
                      | GDK_POINTER_MOTION_MASK
                      | GDK_ENTER_NOTIFY_MASK
                      | GDK_LEAVE_NOTIFY_MASK);
  attr_mask = GDK_WA_X | GDK_WA_Y;

  parent_window = gtk_widget_get_parent_window (widget);
  gtk_widget_set_window (widget, parent_window);
  g_object_ref (parent_window);

  priv->window = gdk_window_new (parent_window, &attr, attr_mask);
  gdk_window_set_user_data (priv->window, hsv);
  gdk_window_show (priv->window);
}

static void
mate_hsv_unrealize (GtkWidget *widget)
{
  MateHSV *hsv = MATE_HSV (widget);
  MateHSVPrivate *priv = hsv->priv;

  gdk_window_set_user_data (priv->window, NULL);
  gdk_window_destroy (priv->window);
  priv->window = NULL;

  GTK_WIDGET_CLASS (mate_hsv_parent_class)->unrealize (widget);
}

static void
mate_hsv_get_preferred_width (GtkWidget *widget,
                              gint      *minimum,
                              gint      *natural)
{
  MateHSV *hsv = MATE_HSV (widget);
  MateHSVPrivate *priv = hsv->priv;
  gint focus_width;
  gint focus_pad;

  gtk_widget_style_get (widget,
                        "focus-line-width", &focus_width,
                        "focus-padding", &focus_pad,
                        NULL);

  *minimum = priv->size + 2 * (focus_width + focus_pad);
  *natural = priv->size + 2 * (focus_width + focus_pad);
}

static void
mate_hsv_get_preferred_height (GtkWidget *widget,
                               gint      *minimum,
                               gint      *natural)
{
  MateHSV *hsv = MATE_HSV (widget);
  MateHSVPrivate *priv = hsv->priv;
  gint focus_width;
  gint focus_pad;

  gtk_widget_style_get (widget,
                        "focus-line-width", &focus_width,
                        "focus-padding", &focus_pad,
                        NULL);

  *minimum = priv->size + 2 * (focus_width + focus_pad);
  *natural = priv->size + 2 * (focus_width + focus_pad);
}

static void
mate_hsv_size_allocate (GtkWidget     *widget,
                        GtkAllocation *allocation)
{
  MateHSV *hsv = MATE_HSV (widget);
  MateHSVPrivate *priv = hsv->priv;

  gtk_widget_set_allocation (widget, allocation);

  if (gtk_widget_get_realized (widget))
    gdk_window_move_resize (priv->window,
                            allocation->x,
                            allocation->y,
                            allocation->width,
                            allocation->height);
}


/* Utility functions */

#define INTENSITY(r, g, b) ((r) * 0.30 + (g) * 0.59 + (b) * 0.11)

/* Converts from HSV to RGB */
static void
hsv_to_rgb (gdouble *h,
            gdouble *s,
            gdouble *v)
{
  gdouble hue, saturation, value;
  gdouble f, p, q, t;

  if (*s == 0.0)
    {
      *h = *v;
      *s = *v;
      *v = *v; /* heh */
    }
  else
    {
      hue = *h * 6.0;
      saturation = *s;
      value = *v;

      if (hue == 6.0)
        hue = 0.0;

      f = hue - (int) hue;
      p = value * (1.0 - saturation);
      q = value * (1.0 - saturation * f);
      t = value * (1.0 - saturation * (1.0 - f));

      switch ((int) hue)
        {
        case 0:
          *h = value;
          *s = t;
          *v = p;
          break;

        case 1:
          *h = q;
          *s = value;
          *v = p;
          break;

        case 2:
          *h = p;
          *s = value;
          *v = t;
          break;

        case 3:
          *h = p;
          *s = q;
          *v = value;
          break;

        case 4:
          *h = t;
          *s = p;
          *v = value;
          break;

        case 5:
          *h = value;
          *s = p;
          *v = q;
          break;

        default:
          g_assert_not_reached ();
        }
    }
}

/* Computes the vertices of the saturation/value triangle */
static void
compute_triangle (MateHSV *hsv,
                  gint    *hx,
                  gint    *hy,
                  gint    *sx,
                  gint    *sy,
                  gint    *vx,
                  gint    *vy)
{
  MateHSVPrivate *priv = hsv->priv;
  GtkWidget *widget = GTK_WIDGET (hsv);
  gdouble center_x;
  gdouble center_y;
  gdouble inner, outer;
  gdouble angle;

  center_x = gtk_widget_get_allocated_width (widget) / 2.0;
  center_y = gtk_widget_get_allocated_height (widget) / 2.0;
  outer = priv->size / 2.0;
  inner = outer - priv->ring_width;
  angle = priv->h * 2.0 * G_PI;

  *hx = floor (center_x + cos (angle) * inner + 0.5);
  *hy = floor (center_y - sin (angle) * inner + 0.5);
  *sx = floor (center_x + cos (angle + 2.0 * G_PI / 3.0) * inner + 0.5);
  *sy = floor (center_y - sin (angle + 2.0 * G_PI / 3.0) * inner + 0.5);
  *vx = floor (center_x + cos (angle + 4.0 * G_PI / 3.0) * inner + 0.5);
  *vy = floor (center_y - sin (angle + 4.0 * G_PI / 3.0) * inner + 0.5);
}

/* Computes whether a point is inside the hue ring */
static gboolean
is_in_ring (MateHSV *hsv,
            gdouble  x,
            gdouble  y)
{
  MateHSVPrivate *priv = hsv->priv;
  GtkWidget *widget = GTK_WIDGET (hsv);
  gdouble dx, dy, dist;
  gdouble center_x;
  gdouble center_y;
  gdouble inner, outer;

  center_x = gtk_widget_get_allocated_width (widget) / 2.0;
  center_y = gtk_widget_get_allocated_height (widget) / 2.0;
  outer = priv->size / 2.0;
  inner = outer - priv->ring_width;

  dx = x - center_x;
  dy = center_y - y;
  dist = dx * dx + dy * dy;

  return (dist >= inner * inner && dist <= outer * outer);
}

/* Computes a saturation/value pair based on the mouse coordinates */
static void
compute_sv (MateHSV  *hsv,
            gdouble   x,
            gdouble   y,
            gdouble  *s,
            gdouble  *v)
{
  GtkWidget *widget = GTK_WIDGET (hsv);
  int ihx, ihy, isx, isy, ivx, ivy;
  double hx, hy, sx, sy, vx, vy;
  double center_x;
  double center_y;

  compute_triangle (hsv, &ihx, &ihy, &isx, &isy, &ivx, &ivy);
  center_x = gtk_widget_get_allocated_width (widget) / 2.0;
  center_y = gtk_widget_get_allocated_height (widget) / 2.0;
  hx = ihx - center_x;
  hy = center_y - ihy;
  sx = isx - center_x;
  sy = center_y - isy;
  vx = ivx - center_x;
  vy = center_y - ivy;
  x -= center_x;
  y = center_y - y;
  
  if (vx * (x - sx) + vy * (y - sy) < 0.0)
    {
      *s = 1.0;
      *v = (((x - sx) * (hx - sx) + (y - sy) * (hy-sy))
            / ((hx - sx) * (hx - sx) + (hy - sy) * (hy - sy)));
      
      if (*v < 0.0)
        *v = 0.0;
      else if (*v > 1.0)
        *v = 1.0;
    }
  else if (hx * (x - sx) + hy * (y - sy) < 0.0)
    {
      *s = 0.0;
      *v = (((x - sx) * (vx - sx) + (y - sy) * (vy - sy))
            / ((vx - sx) * (vx - sx) + (vy - sy) * (vy - sy)));
      
      if (*v < 0.0)
        *v = 0.0;
      else if (*v > 1.0)
        *v = 1.0;
    }
  else if (sx * (x - hx) + sy * (y - hy) < 0.0)
    {
      *v = 1.0;
      *s = (((x - vx) * (hx - vx) + (y - vy) * (hy - vy)) /
            ((hx - vx) * (hx - vx) + (hy - vy) * (hy - vy)));
      
      if (*s < 0.0)
        *s = 0.0;
      else if (*s > 1.0)
        *s = 1.0;
    }
  else
    {
      *v = (((x - sx) * (hy - vy) - (y - sy) * (hx - vx))
            / ((vx - sx) * (hy - vy) - (vy - sy) * (hx - vx)));
      
      if (*v<= 0.0)
        {
          *v = 0.0;
          *s = 0.0;
        }
      else
        {
          if (*v > 1.0)
            *v = 1.0;

          if (fabs (hy - vy) < fabs (hx - vx))
            *s = (x - sx - *v * (vx - sx)) / (*v * (hx - vx));
          else
            *s = (y - sy - *v * (vy - sy)) / (*v * (hy - vy));

          if (*s < 0.0)
            *s = 0.0;
          else if (*s > 1.0)
            *s = 1.0;
        }
    }
}

/* Computes whether a point is inside the saturation/value triangle */
static gboolean
is_in_triangle (MateHSV *hsv,
                gdouble  x,
                gdouble  y)
{
  int hx, hy, sx, sy, vx, vy;
  double det, s, v;
  
  compute_triangle (hsv, &hx, &hy, &sx, &sy, &vx, &vy);
  
  det = (vx - sx) * (hy - sy) - (vy - sy) * (hx - sx);
  
  s = ((x - sx) * (hy - sy) - (y - sy) * (hx - sx)) / det;
  v = ((vx - sx) * (y - sy) - (vy - sy) * (x - sx)) / det;
  
  return (s >= 0.0 && v >= 0.0 && s + v <= 1.0);
}

/* Computes a value based on the mouse coordinates */
static double
compute_v (MateHSV *hsv,
           gdouble  x,
           gdouble  y)
{
  GtkWidget *widget = GTK_WIDGET (hsv);
  double center_x;
  double center_y;
  double dx, dy;
  double angle;

  center_x = gtk_widget_get_allocated_width (widget) / 2.0;
  center_y = gtk_widget_get_allocated_height (widget) / 2.0;
  dx = x - center_x;
  dy = center_y - y;

  angle = atan2 (dy, dx);
  if (angle < 0.0)
    angle += 2.0 * G_PI;

  return angle / (2.0 * G_PI);
}

/* Event handlers */

static void
set_cross_grab (MateHSV   *hsv,
                GdkDevice *device,
                guint32    time)
{
  MateHSVPrivate *priv = hsv->priv;
  GdkCursor *cursor;

  cursor = gdk_cursor_new_for_display (gtk_widget_get_display (GTK_WIDGET (hsv)),
                                       GDK_CROSSHAIR);
  gdk_seat_grab (gdk_device_get_seat (device),
                 priv->window,
                 GDK_SEAT_CAPABILITY_ALL_POINTING,
                 FALSE,
                 cursor,
                 NULL,
                 NULL,
                 NULL);
  g_object_unref (cursor);
}

static gboolean
mate_hsv_grab_broken (GtkWidget          *widget,
                      GdkEventGrabBroken *event)
{
  MateHSV *hsv = MATE_HSV (widget);
  MateHSVPrivate *priv = hsv->priv;

  priv->mode = DRAG_NONE;

  return TRUE;
}

static gint
mate_hsv_button_press (GtkWidget      *widget,
                       GdkEventButton *event)
{
  MateHSV *hsv = MATE_HSV (widget);
  MateHSVPrivate *priv = hsv->priv;
  double x, y;

  if (priv->mode != DRAG_NONE || event->button != GDK_BUTTON_PRIMARY)
    return FALSE;

  x = event->x;
  y = event->y;

  if (is_in_ring (hsv, x, y))
    {
      priv->mode = DRAG_H;
      set_cross_grab (hsv, gdk_event_get_device ((GdkEvent *) event), event->time);

      mate_hsv_set_color (hsv,
                         compute_v (hsv, x, y),
                         priv->s,
                         priv->v);

      gtk_widget_grab_focus (widget);
      priv->focus_on_ring = TRUE;

      return TRUE;
    }

  if (is_in_triangle (hsv, x, y))
    {
      gdouble s, v;

      priv->mode = DRAG_SV;
      set_cross_grab (hsv, gdk_event_get_device ((GdkEvent *) event), event->time);

      compute_sv (hsv, x, y, &s, &v);
      mate_hsv_set_color (hsv, priv->h, s, v);

      gtk_widget_grab_focus (widget);
      priv->focus_on_ring = FALSE;

      return TRUE;
    }

  return FALSE;
}

static gint
mate_hsv_button_release (GtkWidget      *widget,
                         GdkEventButton *event)
{
  MateHSV *hsv = MATE_HSV (widget);
  MateHSVPrivate *priv = hsv->priv;
  DragMode mode;
  gdouble x, y;

  if (priv->mode == DRAG_NONE || event->button != GDK_BUTTON_PRIMARY)
    return FALSE;

  /* Set the drag mode to DRAG_NONE so that signal handlers for "catched"
   * can see that this is the final color state.
   */
  mode = priv->mode;
  priv->mode = DRAG_NONE;

  x = event->x;
  y = event->y;

  if (mode == DRAG_H)
    {
      mate_hsv_set_color (hsv, compute_v (hsv, x, y), priv->s, priv->v);
    }
  else if (mode == DRAG_SV)
    {
      gdouble s, v;

      compute_sv (hsv, x, y, &s, &v);
      mate_hsv_set_color (hsv, priv->h, s, v);
    }
  else
    {
      g_assert_not_reached ();
    }

  gdk_seat_ungrab (gdk_device_get_seat (gdk_event_get_device ((GdkEvent *) event)));

  return TRUE;
}

static gint
mate_hsv_motion (GtkWidget      *widget,
                 GdkEventMotion *event)
{
  MateHSV *hsv = MATE_HSV (widget);
  MateHSVPrivate *priv = hsv->priv;
  gdouble x, y;

  if (priv->mode == DRAG_NONE)
    return FALSE;

  gdk_event_request_motions (event);
  x = event->x;
  y = event->y;

  if (priv->mode == DRAG_H)
    {
      mate_hsv_set_color (hsv, compute_v (hsv, x, y), priv->s, priv->v);
      return TRUE;
    }
  else if (priv->mode == DRAG_SV)
    {
      gdouble s, v;

      compute_sv (hsv, x, y, &s, &v);
      mate_hsv_set_color (hsv, priv->h, s, v);
      return TRUE;
    }

  g_assert_not_reached ();

  return FALSE;
}


/* Redrawing */

/* Paints the hue ring */
static void
paint_ring (MateHSV *hsv,
            cairo_t *cr)
{
  MateHSVPrivate *priv = hsv->priv;
  GtkWidget *widget = GTK_WIDGET (hsv);
  int xx, yy, width, height;
  gdouble dx, dy, dist;
  gdouble center_x;
  gdouble center_y;
  gdouble inner, outer;
  guint32 *buf, *p;
  gdouble angle;
  gdouble hue;
  gdouble r, g, b;
  cairo_surface_t *source;
  cairo_t *source_cr;
  gint stride;

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  center_x = width / 2.0;
  center_y = height / 2.0;

  outer = priv->size / 2.0;
  inner = outer - priv->ring_width;

  /* Create an image initialized with the ring colors */

  stride = cairo_format_stride_for_width (CAIRO_FORMAT_RGB24, width);
  buf = g_new (guint32, height * stride / 4);

  for (yy = 0; yy < height; yy++)
    {
      p = buf + yy * width;

      dy = -(yy - center_y);

      for (xx = 0; xx < width; xx++)
        {
          dx = xx - center_x;

          dist = dx * dx + dy * dy;
          if (dist < ((inner-1) * (inner-1)) || dist > ((outer+1) * (outer+1)))
            {
              *p++ = 0;
              continue;
            }

          angle = atan2 (dy, dx);
          if (angle < 0.0)
            angle += 2.0 * G_PI;

          hue = angle / (2.0 * G_PI);

          r = hue;
          g = 1.0;
          b = 1.0;
          hsv_to_rgb (&r, &g, &b);

          *p++ = (((int)floor (r * 255 + 0.5) << 16) |
                  ((int)floor (g * 255 + 0.5) << 8) |
                  (int)floor (b * 255 + 0.5));
        }
    }

  source = cairo_image_surface_create_for_data ((unsigned char *)buf,
                                                CAIRO_FORMAT_RGB24,
                                                width, height, stride);

  /* Now draw the value marker onto the source image, so that it
   * will get properly clipped at the edges of the ring
   */
  source_cr = cairo_create (source);
  
  r = priv->h;
  g = 1.0;
  b = 1.0;
  hsv_to_rgb (&r, &g, &b);
  
  if (INTENSITY (r, g, b) > 0.5)
    cairo_set_source_rgb (source_cr, 0., 0., 0.);
  else
    cairo_set_source_rgb (source_cr, 1., 1., 1.);

  cairo_move_to (source_cr, center_x, center_y);
  cairo_line_to (source_cr,
                 center_x + cos (priv->h * 2.0 * G_PI) * priv->size / 2,
                 center_y - sin (priv->h * 2.0 * G_PI) * priv->size / 2);
  cairo_stroke (source_cr);
  cairo_destroy (source_cr);

  /* Draw the ring using the source image */

  cairo_save (cr);
    
  cairo_set_source_surface (cr, source, 0, 0);
  cairo_surface_destroy (source);

  cairo_set_line_width (cr, priv->ring_width);
  cairo_new_path (cr);
  cairo_arc (cr,
             center_x, center_y,
             priv->size / 2. - priv->ring_width / 2.,
             0, 2 * G_PI);
  cairo_stroke (cr);
  
  cairo_restore (cr);
  
  g_free (buf);
}

/* Converts an HSV triplet to an integer RGB triplet */
static void
get_color (gdouble h,
           gdouble s,
           gdouble v,
           gint   *r,
           gint   *g,
           gint   *b)
{
  hsv_to_rgb (&h, &s, &v);
  
  *r = floor (h * 255 + 0.5);
  *g = floor (s * 255 + 0.5);
  *b = floor (v * 255 + 0.5);
}

#define SWAP(a, b, t) ((t) = (a), (a) = (b), (b) = (t))

#define LERP(a, b, v1, v2, i) (((v2) - (v1) != 0)                                       \
                               ? ((a) + ((b) - (a)) * ((i) - (v1)) / ((v2) - (v1)))     \
                               : (a))

/* Number of pixels we extend out from the edges when creating
 * color source to avoid artifacts
 */
#define PAD 3

/* Paints the HSV triangle */
static void
paint_triangle (MateHSV  *hsv,
                cairo_t  *cr,
                gboolean  draw_focus)
{
  MateHSVPrivate *priv = hsv->priv;
  GtkWidget *widget = GTK_WIDGET (hsv);
  gint hx, hy, sx, sy, vx, vy; /* HSV vertices */
  gint x1, y1, r1, g1, b1; /* First vertex in scanline order */
  gint x2, y2, r2, g2, b2; /* Second vertex */
  gint x3, y3, r3, g3, b3; /* Third vertex */
  gint t;
  guint32 *buf, *p, c;
  gint xl, xr, rl, rr, gl, gr, bl, br; /* Scanline data */
  gint xx, yy;
  gint x_interp, y_interp;
  gint x_start, x_end;
  cairo_surface_t *source;
  gdouble r, g, b;
  gint stride;
  int width, height;
  GtkStyleContext *context;

  width = gtk_widget_get_allocated_width (widget); 
  height = gtk_widget_get_allocated_height (widget); 
  /* Compute triangle's vertices */
  
  compute_triangle (hsv, &hx, &hy, &sx, &sy, &vx, &vy);
  
  x1 = hx;
  y1 = hy;
  get_color (priv->h, 1.0, 1.0, &r1, &g1, &b1);

  x2 = sx;
  y2 = sy;
  get_color (priv->h, 1.0, 0.0, &r2, &g2, &b2);

  x3 = vx;
  y3 = vy;
  get_color (priv->h, 0.0, 1.0, &r3, &g3, &b3);

  if (y2 > y3)
    {
      SWAP (x2, x3, t);
      SWAP (y2, y3, t);
      SWAP (r2, r3, t);
      SWAP (g2, g3, t);
      SWAP (b2, b3, t);
    }

  if (y1 > y3)
    {
      SWAP (x1, x3, t);
      SWAP (y1, y3, t);
      SWAP (r1, r3, t);
      SWAP (g1, g3, t);
      SWAP (b1, b3, t);
    }

  if (y1 > y2)
    {
      SWAP (x1, x2, t);
      SWAP (y1, y2, t);
      SWAP (r1, r2, t);
      SWAP (g1, g2, t);
      SWAP (b1, b2, t);
    }

  /* Shade the triangle */

  stride = cairo_format_stride_for_width (CAIRO_FORMAT_RGB24, width);
  buf = g_new (guint32, height * stride / 4);

  for (yy = 0; yy < height; yy++)
    {
      p = buf + yy * width;

      if (yy >= y1 - PAD && yy < y3 + PAD) {
        y_interp = CLAMP (yy, y1, y3);

        if (y_interp < y2)
          {
            xl = LERP (x1, x2, y1, y2, y_interp);

            rl = LERP (r1, r2, y1, y2, y_interp);
            gl = LERP (g1, g2, y1, y2, y_interp);
            bl = LERP (b1, b2, y1, y2, y_interp);
          }
        else
          {
            xl = LERP (x2, x3, y2, y3, y_interp);

            rl = LERP (r2, r3, y2, y3, y_interp);
            gl = LERP (g2, g3, y2, y3, y_interp);
            bl = LERP (b2, b3, y2, y3, y_interp);
          }

        xr = LERP (x1, x3, y1, y3, y_interp);

        rr = LERP (r1, r3, y1, y3, y_interp);
        gr = LERP (g1, g3, y1, y3, y_interp);
        br = LERP (b1, b3, y1, y3, y_interp);

        if (xl > xr)
          {
            SWAP (xl, xr, t);
            SWAP (rl, rr, t);
            SWAP (gl, gr, t);
            SWAP (bl, br, t);
          }

        x_start = MAX (xl - PAD, 0);
        x_end = MIN (xr + PAD, width);
        x_start = MIN (x_start, x_end);

        c = (rl << 16) | (gl << 8) | bl;

        for (xx = 0; xx < x_start; xx++)
          *p++ = c;

        for (; xx < x_end; xx++)
          {
            x_interp = CLAMP (xx, xl, xr);

            *p++ = ((LERP (rl, rr, xl, xr, x_interp) << 16) |
                    (LERP (gl, gr, xl, xr, x_interp) << 8) |
                    LERP (bl, br, xl, xr, x_interp));
          }

        c = (rr << 16) | (gr << 8) | br;

        for (; xx < width; xx++)
          *p++ = c;
      }
    }

  source = cairo_image_surface_create_for_data ((unsigned char *)buf,
                                                CAIRO_FORMAT_RGB24,
                                                width, height, stride);
  
  /* Draw a triangle with the image as a source */

  cairo_set_source_surface (cr, source, 0, 0);
  cairo_surface_destroy (source);
  
  cairo_move_to (cr, x1, y1);
  cairo_line_to (cr, x2, y2);
  cairo_line_to (cr, x3, y3);
  cairo_close_path (cr);
  cairo_fill (cr);
  
  g_free (buf);
  
  /* Draw value marker */
  
  xx = floor (sx + (vx - sx) * priv->v + (hx - vx) * priv->s * priv->v + 0.5);
  yy = floor (sy + (vy - sy) * priv->v + (hy - vy) * priv->s * priv->v + 0.5);
  
  r = priv->h;
  g = priv->s;
  b = priv->v;
  hsv_to_rgb (&r, &g, &b);

  context = gtk_widget_get_style_context (widget);

  gtk_style_context_save (context);

  if (INTENSITY (r, g, b) > 0.5)
    {
      gtk_style_context_add_class (context, "light-area-focus");
      cairo_set_source_rgb (cr, 0., 0., 0.);
    }
  else
    {
      gtk_style_context_add_class (context, "dark-area-focus");
      cairo_set_source_rgb (cr, 1., 1., 1.);
    }

#define RADIUS 4
#define FOCUS_RADIUS 6

  cairo_new_path (cr);
  cairo_arc (cr, xx, yy, RADIUS, 0, 2 * G_PI);
  cairo_stroke (cr);
  
  /* Draw focus outline */

  if (draw_focus && !priv->focus_on_ring)
    {
      gint focus_width;
      gint focus_pad;

      gtk_widget_style_get (widget,
                            "focus-line-width", &focus_width,
                            "focus-padding", &focus_pad,
                            NULL);

      gtk_render_focus (context, cr,
                        xx - FOCUS_RADIUS - focus_width - focus_pad,
                        yy - FOCUS_RADIUS - focus_width - focus_pad,
                        2 * (FOCUS_RADIUS + focus_width + focus_pad),
                        2 * (FOCUS_RADIUS + focus_width + focus_pad));
    }

  gtk_style_context_restore (context);
}

/* Paints the contents of the HSV color selector */
static gboolean
mate_hsv_draw (GtkWidget *widget,
               cairo_t   *cr)
{
  MateHSV *hsv = MATE_HSV (widget);
  MateHSVPrivate *priv = hsv->priv;
  gboolean draw_focus;

  draw_focus = gtk_widget_has_visible_focus (widget);

  paint_ring (hsv, cr);
  paint_triangle (hsv, cr, draw_focus);


  if (draw_focus && priv->focus_on_ring)
    {
      GtkStyleContext *context;

      context = gtk_widget_get_style_context (widget);

      gtk_render_focus (context, cr, 0, 0,
                        gtk_widget_get_allocated_width (widget),
                        gtk_widget_get_allocated_height (widget));
    }

  return FALSE;
}

static gboolean
mate_hsv_focus (GtkWidget       *widget,
                GtkDirectionType dir)
{
  MateHSV *hsv = MATE_HSV (widget);
  MateHSVPrivate *priv = hsv->priv;

  if (!gtk_widget_has_focus (widget))
    {
      if (dir == GTK_DIR_TAB_BACKWARD)
        priv->focus_on_ring = FALSE;
      else
        priv->focus_on_ring = TRUE;

      gtk_widget_grab_focus (GTK_WIDGET (hsv));
      return TRUE;
    }
  
  switch (dir)
    {
    case GTK_DIR_UP:
      if (priv->focus_on_ring)
        return FALSE;
      else
        priv->focus_on_ring = TRUE;
      break;

    case GTK_DIR_DOWN:
      if (priv->focus_on_ring)
        priv->focus_on_ring = FALSE;
      else
        return FALSE;
      break;

    case GTK_DIR_LEFT:
    case GTK_DIR_TAB_BACKWARD:
      if (priv->focus_on_ring)
        return FALSE;
      else
        priv->focus_on_ring = TRUE;
      break;

    case GTK_DIR_RIGHT:
    case GTK_DIR_TAB_FORWARD:
      if (priv->focus_on_ring)
        priv->focus_on_ring = FALSE;
      else
        return FALSE;
      break;
    }

  gtk_widget_queue_draw (GTK_WIDGET (hsv));

  return TRUE;
}

/**
 * mate_hsv_new:
 *
 * Creates a new HSV color selector.
 *
 * Returns: A newly-created HSV color selector.
 */
GtkWidget*
mate_hsv_new (void)
{
  return g_object_new (MATE_TYPE_HSV, NULL);
}

/**
 * mate_hsv_set_color:
 * @hsv: An HSV color selector
 * @h: Hue
 * @s: Saturation
 * @v: Value
 *
 * Sets the current color in an HSV color selector.
 * Color component values must be in the [0.0, 1.0] range.
 */
void
mate_hsv_set_color (MateHSV *hsv,
                    gdouble  h,
                    gdouble  s,
                    gdouble  v)
{
  MateHSVPrivate *priv;

  g_return_if_fail (MATE_IS_HSV (hsv));
  g_return_if_fail (h >= 0.0 && h <= 1.0);
  g_return_if_fail (s >= 0.0 && s <= 1.0);
  g_return_if_fail (v >= 0.0 && v <= 1.0);

  priv = hsv->priv;

  priv->h = h;
  priv->s = s;
  priv->v = v;
  
  g_signal_emit (hsv, hsv_signals[CHANGED], 0);
  
  gtk_widget_queue_draw (GTK_WIDGET (hsv));
}

/**
 * mate_hsv_get_color:
 * @hsv: An HSV color selector
 * @h: (out): Return value for the hue
 * @s: (out): Return value for the saturation
 * @v: (out): Return value for the value
 *
 * Queries the current color in an HSV color selector.
 * Returned values will be in the [0.0, 1.0] range.
 */
void
mate_hsv_get_color (MateHSV *hsv,
                    double  *h,
                    double  *s,
                    double  *v)
{
  MateHSVPrivate *priv;

  g_return_if_fail (MATE_IS_HSV (hsv));

  priv = hsv->priv;

  if (h)
    *h = priv->h;

  if (s)
    *s = priv->s;

  if (v)
    *v = priv->v;
}

/**
 * mate_hsv_set_metrics:
 * @hsv: An HSV color selector
 * @size: Diameter for the hue ring
 * @ring_width: Width of the hue ring
 *
 * Sets the size and ring width of an HSV color selector.
 */
void
mate_hsv_set_metrics (MateHSV *hsv,
                      gint     size,
                      gint     ring_width)
{
  MateHSVPrivate *priv;
  int same_size;

  g_return_if_fail (MATE_IS_HSV (hsv));
  g_return_if_fail (size > 0);
  g_return_if_fail (ring_width > 0);
  g_return_if_fail (2 * ring_width + 1 <= size);

  priv = hsv->priv;

  same_size = (priv->size == size);

  priv->size = size;
  priv->ring_width = ring_width;
  
  if (same_size)
    gtk_widget_queue_draw (GTK_WIDGET (hsv));
  else
    gtk_widget_queue_resize (GTK_WIDGET (hsv));
}

/**
 * mate_hsv_get_metrics:
 * @hsv: An HSV color selector
 * @size: (out): Return value for the diameter of the hue ring
 * @ring_width: (out): Return value for the width of the hue ring
 *
 * Queries the size and ring width of an HSV color selector.
 */
void
mate_hsv_get_metrics (MateHSV *hsv,
                      gint    *size,
                      gint    *ring_width)
{
  MateHSVPrivate *priv;

  g_return_if_fail (MATE_IS_HSV (hsv));

  priv = hsv->priv;

  if (size)
    *size = priv->size;
  
  if (ring_width)
    *ring_width = priv->ring_width;
}

/**
 * mate_hsv_is_adjusting:
 * @hsv: A #MateHSV 
 *
 * An HSV color selector can be said to be adjusting if multiple rapid
 * changes are being made to its value, for example, when the user is 
 * adjusting the value with the mouse. This function queries whether 
 * the HSV color selector is being adjusted or not.
 *
 * Returns: %TRUE if clients can ignore changes to the color value,
 *     since they may be transitory, or %FALSE if they should consider
 *     the color value status to be final.
 */
gboolean
mate_hsv_is_adjusting (MateHSV *hsv)
{
  MateHSVPrivate *priv;

  g_return_val_if_fail (MATE_IS_HSV (hsv), FALSE);

  priv = hsv->priv;

  return priv->mode != DRAG_NONE;
}

static void
mate_hsv_move (MateHSV         *hsv,
               GtkDirectionType dir)
{
  MateHSVPrivate *priv = hsv->priv;
  gdouble hue, sat, val;
  gint hx, hy, sx, sy, vx, vy; /* HSV vertices */
  gint x, y; /* position in triangle */

  hue = priv->h;
  sat = priv->s;
  val = priv->v;

  compute_triangle (hsv, &hx, &hy, &sx, &sy, &vx, &vy);

  x = floor (sx + (vx - sx) * priv->v + (hx - vx) * priv->s * priv->v + 0.5);
  y = floor (sy + (vy - sy) * priv->v + (hy - vy) * priv->s * priv->v + 0.5);

#define HUE_DELTA 0.002
  switch (dir)
    {
    case GTK_DIR_UP:
      if (priv->focus_on_ring)
        hue += HUE_DELTA;
      else
        {
          y -= 1;
          compute_sv (hsv, x, y, &sat, &val);
        }
      break;

    case GTK_DIR_DOWN:
      if (priv->focus_on_ring)
        hue -= HUE_DELTA;
      else
        {
          y += 1;
          compute_sv (hsv, x, y, &sat, &val);
        }
      break;

    case GTK_DIR_LEFT:
      if (priv->focus_on_ring)
        hue += HUE_DELTA;
      else
        {
          x -= 1;
          compute_sv (hsv, x, y, &sat, &val);
        }
      break;

    case GTK_DIR_RIGHT:
      if (priv->focus_on_ring)
        hue -= HUE_DELTA
          ;
      else
        {
          x += 1;
          compute_sv (hsv, x, y, &sat, &val);
        }
      break;

    default:
      /* we don't care about the tab directions */
      break;
    }

  /* Wrap */
  if (hue < 0.0)
    hue = 1.0;
  else if (hue > 1.0)
    hue = 0.0;
  
  mate_hsv_set_color (hsv, hue, sat, val);
}

