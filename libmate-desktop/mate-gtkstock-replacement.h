/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* mate-gtkstock-replacement.h - MATE Desktop button management for dialog

   Copyright (C) 2019, ZenWalker
   All rights reserved.

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
 */

#ifndef MATE_GTKSTOCK_REPLACEMENT_H
#define MATE_GTKSTOCK_REPLACEMENT_H

#include <gtk/gtk.h>

GtkWidget*
mate_dialog_add_button (GtkDialog   *dialog,
                        const gchar *button_text,
                        const gchar *icon_name,
                        gint   response_id);

#endif /* MATE_GTKSTOCK_REPLACEMENT_H */
