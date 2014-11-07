/* mate-rr-config.h
 * -*- c-basic-offset: 4 -*-
 *
 * Copyright 2007, 2008, Red Hat, Inc.
 * Copyright 2010 Giovanni Campagna
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
 * Boston, MA  02110-1301, USA.
 * 
 * Author: Soren Sandmann <sandmann@redhat.com>
 */
#ifndef MATE_RR_CONFIG_H
#define MATE_RR_CONFIG_H

#ifndef MATE_DESKTOP_USE_UNSTABLE_API
#error   mate-rr-config.h is unstable API. You must define MATE_DESKTOP_USE_UNSTABLE_API before including mate-rr-config.h
#endif

#include "mate-rr.h"
#include <glib.h>
#include <glib-object.h>

typedef struct MateRROutputInfoPrivate MateRROutputInfoPrivate;
typedef struct MateRRConfigPrivate MateRRConfigPrivate;

typedef struct
{
    GObject parent;

    /*< private >*/
    MateRROutputInfoPrivate *priv;
} MateRROutputInfo;

typedef struct
{
    GObjectClass parent_class;
} MateRROutputInfoClass;

#define MATE_TYPE_RR_OUTPUT_INFO                  (mate_rr_output_info_get_type())
#define MATE_RR_OUTPUT_INFO(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_RR_OUTPUT_INFO, MateRROutputInfo))
#define MATE_IS_RR_OUTPUT_INFO(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_RR_OUTPUT_INFO))
#define MATE_RR_OUTPUT_INFO_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_RR_OUTPUT_INFO, MateRROutputInfoClass))
#define MATE_IS_RR_OUTPUT_INFO_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_RR_OUTPUT_INFO))
#define MATE_RR_OUTPUT_INFO_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_RR_OUTPUT_INFO, MateRROutputInfoClass))

GType mate_rr_output_info_get_type (void);

char *mate_rr_output_info_get_name (MateRROutputInfo *self);

gboolean mate_rr_output_info_get_active (MateRROutputInfo *self);
void     mate_rr_output_info_set_active (MateRROutputInfo *self, gboolean active);


void mate_rr_output_info_get_geometry (MateRROutputInfo *self, int *x, int *y, int *width, int *height);
void mate_rr_output_info_set_geometry (MateRROutputInfo *self, int  x, int  y, int  width, int  height);

int  mate_rr_output_info_get_refresh_rate (MateRROutputInfo *self);
void mate_rr_output_info_set_refresh_rate (MateRROutputInfo *self, int rate);

MateRRRotation mate_rr_output_info_get_rotation (MateRROutputInfo *self);
void            mate_rr_output_info_set_rotation (MateRROutputInfo *self, MateRRRotation rotation);

gboolean mate_rr_output_info_get_connected    (MateRROutputInfo *self);
void     mate_rr_output_info_get_vendor       (MateRROutputInfo *self, gchar* vendor);
guint    mate_rr_output_info_get_product      (MateRROutputInfo *self);
guint    mate_rr_output_info_get_serial       (MateRROutputInfo *self);
double   mate_rr_output_info_get_aspect_ratio (MateRROutputInfo *self);
char    *mate_rr_output_info_get_display_name (MateRROutputInfo *self);

gboolean mate_rr_output_info_get_primary (MateRROutputInfo *self);
void     mate_rr_output_info_set_primary (MateRROutputInfo *self, gboolean primary);

int mate_rr_output_info_get_preferred_width  (MateRROutputInfo *self);
int mate_rr_output_info_get_preferred_height (MateRROutputInfo *self);

typedef struct
{
    GObject parent;

    /*< private >*/
    MateRRConfigPrivate *priv;
} MateRRConfig;

typedef struct
{
    GObjectClass parent_class;
} MateRRConfigClass;

#define MATE_TYPE_RR_CONFIG                  (mate_rr_config_get_type())
#define MATE_RR_CONFIG(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_RR_CONFIG, MateRRConfig))
#define MATE_IS_RR_CONFIG(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_RR_CONFIG))
#define MATE_RR_CONFIG_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_RR_CONFIG, MateRRConfigClass))
#define MATE_IS_RR_CONFIG_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_RR_CONFIG))
#define MATE_RR_CONFIG_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_RR_CONFIG, MateRRConfigClass))

GType               mate_rr_config_get_type     (void);

MateRRConfig      *mate_rr_config_new_current  (MateRRScreen  *screen,
						  GError        **error);
MateRRConfig      *mate_rr_config_new_stored   (MateRRScreen  *screen,
						  GError        **error);
gboolean                mate_rr_config_load_current (MateRRConfig  *self,
						      GError        **error);
gboolean                mate_rr_config_load_filename (MateRRConfig  *self,
						       const gchar    *filename,
						       GError        **error);
gboolean            mate_rr_config_match        (MateRRConfig  *config1,
						  MateRRConfig  *config2);
gboolean            mate_rr_config_equal	 (MateRRConfig  *config1,
						  MateRRConfig  *config2);
gboolean            mate_rr_config_save         (MateRRConfig  *configuration,
						  GError        **error);
void                mate_rr_config_sanitize     (MateRRConfig  *configuration);
gboolean            mate_rr_config_ensure_primary (MateRRConfig  *configuration);

gboolean	    mate_rr_config_apply_with_time (MateRRConfig  *configuration,
						     MateRRScreen  *screen,
						     guint32         timestamp,
						     GError        **error);

gboolean            mate_rr_config_apply_from_filename_with_time (MateRRScreen  *screen,
								   const char     *filename,
								   guint32         timestamp,
								   GError        **error);

gboolean            mate_rr_config_applicable   (MateRRConfig  *configuration,
						  MateRRScreen  *screen,
						  GError        **error);

gboolean            mate_rr_config_get_clone    (MateRRConfig  *configuration);
void                mate_rr_config_set_clone    (MateRRConfig  *configuration, gboolean clone);
MateRROutputInfo **mate_rr_config_get_outputs  (MateRRConfig  *configuration);

char *mate_rr_config_get_backup_filename (void);
char *mate_rr_config_get_intended_filename (void);

#endif
