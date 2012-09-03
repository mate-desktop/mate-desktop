/* mate-rr-labeler.h - Utility to label monitors to identify them
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
 * write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 * Author: Federico Mena-Quintero <federico@novell.com>
 */

#ifndef MATE_RR_LABELER_H
#define MATE_RR_LABELER_H

#ifndef MATE_DESKTOP_USE_UNSTABLE_API
#error    MateRR is unstable API. You must define MATE_DESKTOP_USE_UNSTABLE_API before including materr.h
#endif

#include <libmateui/mate-rr-config.h>

#define MATE_TYPE_RR_LABELER            (mate_rr_labeler_get_type ())
#define MATE_RR_LABELER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_RR_LABELER, MateRRLabeler))
#define MATE_RR_LABELER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  MATE_TYPE_RR_LABELER, MateRRLabelerClass))
#define MATE_IS_RR_LABELER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_RR_LABELER))
#define MATE_IS_RR_LABELER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  MATE_TYPE_RR_LABELER))
#define MATE_RR_LABELER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  MATE_TYPE_RR_LABELER, MateRRLabelerClass))

typedef struct _MateRRLabeler MateRRLabeler;
typedef struct _MateRRLabelerClass MateRRLabelerClass;

GType mate_rr_labeler_get_type (void);

MateRRLabeler *mate_rr_labeler_new (MateRRConfig *config);

void mate_rr_labeler_hide (MateRRLabeler *labeler);

void mate_rr_labeler_get_color_for_output (MateRRLabeler *labeler, MateOutputInfo *output, GdkColor *color_out);

#endif
