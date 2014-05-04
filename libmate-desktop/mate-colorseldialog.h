/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __MATE_COLOR_SELECTION_DIALOG_H__
#define __MATE_COLOR_SELECTION_DIALOG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MATE_TYPE_COLOR_SELECTION_DIALOG            (mate_color_selection_dialog_get_type ())
#define MATE_COLOR_SELECTION_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_COLOR_SELECTION_DIALOG, MateColorSelectionDialog))
#define MATE_COLOR_SELECTION_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_COLOR_SELECTION_DIALOG, MateColorSelectionDialogClass))
#define MATE_IS_COLOR_SELECTION_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_COLOR_SELECTION_DIALOG))
#define MATE_IS_COLOR_SELECTION_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_COLOR_SELECTION_DIALOG))
#define MATE_COLOR_SELECTION_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_COLOR_SELECTION_DIALOG, MateColorSelectionDialogClass))


typedef struct _MateColorSelectionDialog       MateColorSelectionDialog;
typedef struct _MateColorSelectionDialogClass  MateColorSelectionDialogClass;


struct _MateColorSelectionDialog
{
  GtkDialog parent_instance;

  GtkWidget *GSEAL (colorsel);
  GtkWidget *GSEAL (ok_button);
  GtkWidget *GSEAL (cancel_button);
  GtkWidget *GSEAL (help_button);
};

struct _MateColorSelectionDialogClass
{
  GtkDialogClass parent_class;

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};


/* ColorSelectionDialog */
GType      mate_color_selection_dialog_get_type            (void) G_GNUC_CONST;
GtkWidget* mate_color_selection_dialog_new                 (const gchar *title);
GtkWidget* mate_color_selection_dialog_get_color_selection (MateColorSelectionDialog *colorsel);


G_END_DECLS

#endif /* __MATE_COLOR_SELECTION_DIALOG_H__ */
