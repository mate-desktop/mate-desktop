/*
 * Copyright (C) 2009-2010 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __MATE_DMI_H
#define __MATE_DMI_H

#ifndef MATE_DESKTOP_USE_UNSTABLE_API
#error    This is unstable API. You must define MATE_DESKTOP_USE_UNSTABLE_API before including mate-dmi.h
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define MATE_TYPE_DMI            (mate_dmi_get_type ())
G_DECLARE_FINAL_TYPE (MateDmi, mate_dmi, MATE, DMI, GObject)

MateDmi      *mate_dmi_new                 (void);

const gchar  *mate_dmi_get_name       (MateDmi *dmi);
const gchar  *mate_dmi_get_vendor     (MateDmi *dmi);
const gchar  *mate_dmi_get_version    (MateDmi *dmi);

G_END_DECLS

#endif /* __MATE_DMI_H */
