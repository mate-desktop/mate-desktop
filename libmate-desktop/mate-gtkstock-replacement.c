/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* mate-gtkstock-replacement.c - MATE Desktop button management for dialog

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

#include <gtk/gtk.h>

/* Add a button in a GtkDialog window
 * 
 * Avoid use of GtkStock deprecated macros
 * 
 * GtkDialog   *dialog: the dialog where add the button
 * const gchar *button_text: the text used for the button
 * const gchar *icon_name: the name of the icon to draw on the button, NULL for button without image
 * gint   response_id: the id of the response returned by the button
 * 
 * return: the button just created
 */
GtkWidget*
mate_dialog_add_button (GtkDialog   *dialog,
                        const gchar *button_text,
                        const gchar *icon_name,
                        gint   response_id)
{
    GtkWidget *button;

    button = gtk_button_new_with_mnemonic (button_text);

    // Set button image only if icon_name is set
    if (icon_name != NULL) {
        gtk_button_set_image (GTK_BUTTON (button), gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON));
    }

    gtk_button_set_use_underline (GTK_BUTTON (button), TRUE);
    gtk_style_context_add_class (gtk_widget_get_style_context (button), "text-button");
    gtk_widget_set_can_default (button, TRUE);
    gtk_widget_show (button);
    gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, response_id);

    return button;
}
