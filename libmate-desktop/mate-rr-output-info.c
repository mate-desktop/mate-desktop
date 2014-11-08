/* mate-rr-output-info.c
 * -*- c-basic-offset: 4 -*-
 *
 * Copyright 2010 Giovanni Campagna
 *
 * This file is part of the Mate Desktop Library.
 *
 * The Mate Desktop Library is free software; you can redistribute it and/or
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
 * License along with the Mate Desktop Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#define MATE_DESKTOP_USE_UNSTABLE_API

#include <config.h>

#include "mate-rr-config.h"

#include "edid.h"
#include "mate-rr-private.h"

G_DEFINE_TYPE (MateRROutputInfo, mate_rr_output_info, G_TYPE_OBJECT)

static void
mate_rr_output_info_init (MateRROutputInfo *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MATE_TYPE_RR_OUTPUT_INFO, MateRROutputInfoPrivate);

    self->priv->name = NULL;
    self->priv->on = FALSE;
    self->priv->display_name = NULL;
}

static void
mate_rr_output_info_finalize (GObject *gobject)
{
    MateRROutputInfo *self = MATE_RR_OUTPUT_INFO (gobject);

    g_free (self->priv->name);
    g_free (self->priv->display_name);

    G_OBJECT_CLASS (mate_rr_output_info_parent_class)->finalize (gobject);
}

static void
mate_rr_output_info_class_init (MateRROutputInfoClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (MateRROutputInfoPrivate));

    gobject_class->finalize = mate_rr_output_info_finalize;
}

/**
 * mate_rr_output_info_get_name:
 *
 * Returns: (transfer none): the output name
 */
char *mate_rr_output_info_get_name (MateRROutputInfo *self)
{
    g_return_val_if_fail (MATE_IS_RR_OUTPUT_INFO (self), NULL);

    return self->priv->name;
}

/**
 * mate_rr_output_info_is_active:
 *
 * Returns: whether there is a CRTC assigned to this output (i.e. a signal is being sent to it)
 */
gboolean mate_rr_output_info_is_active (MateRROutputInfo *self)
{
    g_return_val_if_fail (MATE_IS_RR_OUTPUT_INFO (self), FALSE);

    return self->priv->on;
}

void mate_rr_output_info_set_active (MateRROutputInfo *self, gboolean active)
{
    g_return_if_fail (MATE_IS_RR_OUTPUT_INFO (self));

    self->priv->on = active;
}

/**
 * mate_rr_output_info_get_geometry:
 *
 * @self: a #MateRROutputInfo
 * @x: (out) (allow-none):
 * @y: (out) (allow-none):
 * @width: (out) (allow-none):
 * @height: (out) (allow-none):
 */
void mate_rr_output_info_get_geometry (MateRROutputInfo *self, int *x, int *y, int *width, int *height)
{
    g_return_if_fail (MATE_IS_RR_OUTPUT_INFO (self));

    if (x)
	*x = self->priv->x;
    if (y)
	*y = self->priv->y;
    if (width)
	*width = self->priv->width;
    if (height)
	*height = self->priv->height;
}

void mate_rr_output_info_set_geometry (MateRROutputInfo *self, int  x, int  y, int  width, int  height)
{
    g_return_if_fail (MATE_IS_RR_OUTPUT_INFO (self));

    self->priv->x = x;
    self->priv->y = y;
    self->priv->width = width;
    self->priv->height = height;
}

int mate_rr_output_info_get_refresh_rate (MateRROutputInfo *self)
{
    g_return_val_if_fail (MATE_IS_RR_OUTPUT_INFO (self), 0);

    return self->priv->rate;
}

void mate_rr_output_info_set_refresh_rate (MateRROutputInfo *self, int rate)
{
    g_return_if_fail (MATE_IS_RR_OUTPUT_INFO (self));

    self->priv->rate = rate;
}

MateRRRotation mate_rr_output_info_get_rotation (MateRROutputInfo *self)
{
    g_return_val_if_fail (MATE_IS_RR_OUTPUT_INFO (self), MATE_RR_ROTATION_0);

    return self->priv->rotation;
}

void mate_rr_output_info_set_rotation (MateRROutputInfo *self, MateRRRotation rotation)
{
    g_return_if_fail (MATE_IS_RR_OUTPUT_INFO (self));

    self->priv->rotation = rotation;
}

/**
 * mate_rr_output_info_is_connected:
 *
 * Returns: whether the output is physically connected to a monitor
 */
gboolean mate_rr_output_info_is_connected (MateRROutputInfo *self)
{
    g_return_val_if_fail (MATE_IS_RR_OUTPUT_INFO (self), FALSE);

    return self->priv->connected;
}

/**
 * mate_rr_output_info_get_vendor:
 *
 * @self: a #MateRROutputInfo
 * @vendor: (out caller-allocates) (array fixed-size=4):
 */
void mate_rr_output_info_get_vendor (MateRROutputInfo *self, gchar* vendor)
{
    g_return_if_fail (MATE_IS_RR_OUTPUT_INFO (self));
    g_return_if_fail (vendor != NULL);

    vendor[0] = self->priv->vendor[0];
    vendor[1] = self->priv->vendor[1];
    vendor[2] = self->priv->vendor[2];
    vendor[3] = self->priv->vendor[3];
}

guint mate_rr_output_info_get_product (MateRROutputInfo *self)
{
    g_return_val_if_fail (MATE_IS_RR_OUTPUT_INFO (self), 0);

    return self->priv->product;
}

guint mate_rr_output_info_get_serial (MateRROutputInfo *self)
{
    g_return_val_if_fail (MATE_IS_RR_OUTPUT_INFO (self), 0);

    return self->priv->serial;
}

double mate_rr_output_info_get_aspect_ratio (MateRROutputInfo *self)
{
    g_return_val_if_fail (MATE_IS_RR_OUTPUT_INFO (self), 0);

    return self->priv->aspect;
}

/**
 * mate_rr_output_info_get_display_name:
 *
 * Returns: (transfer none): the display name of this output
 */
char *mate_rr_output_info_get_display_name (MateRROutputInfo *self)
{
    g_return_val_if_fail (MATE_IS_RR_OUTPUT_INFO (self), NULL);

    return self->priv->display_name;
}

gboolean mate_rr_output_info_get_primary (MateRROutputInfo *self)
{
    g_return_val_if_fail (MATE_IS_RR_OUTPUT_INFO (self), FALSE);

    return self->priv->primary;
}

void mate_rr_output_info_set_primary (MateRROutputInfo *self, gboolean primary)
{
    g_return_if_fail (MATE_IS_RR_OUTPUT_INFO (self));

    self->priv->primary = primary;
}

int mate_rr_output_info_get_preferred_width (MateRROutputInfo *self)
{
    g_return_val_if_fail (MATE_IS_RR_OUTPUT_INFO (self), 0);

    return self->priv->pref_width;
}

int mate_rr_output_info_get_preferred_height (MateRROutputInfo *self)
{
    g_return_val_if_fail (MATE_IS_RR_OUTPUT_INFO (self), 0);

    return self->priv->pref_height;
}
