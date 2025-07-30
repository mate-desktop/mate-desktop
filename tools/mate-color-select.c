/*
 * mate-color.c: MATE color selection tool
 *
 * Copyright (C) 2014 Stefano Karapetsas
 * Copyright (C) 2014-2021 MATE Developers
 * Copyright (C) 2025 linuxCowboy
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Authors:
 *  Stefano Karapetsas <stefano@karapetsas.com>
 */

#include <config.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <libmate-desktop/mate-colorseldialog.h>
#include <libmate-desktop/mate-colorsel.h>

static gboolean
save (GtkWidget *widget, GdkEvent *event, MateColorSelectionDialog *color_dialog)
{
    mate_color_selection_palette_save ();
    return 0;
}

static gboolean
load (GtkWidget *widget, GdkEvent *event, MateColorSelectionDialog *color_dialog)
{
    mate_color_selection_palette_load (MATE_COLOR_SELECTION (color_dialog->colorsel));
    return 0;
}

static gboolean
reset (GtkWidget *widget, GdkEvent *event, MateColorSelectionDialog *color_dialog)
{
    mate_color_selection_palette_set (MATE_COLOR_SELECTION (color_dialog->colorsel), NULL);
    return 0;
}

static gboolean
copy_color (GtkWidget *widget, GdkEvent *event, MateColorSelectionDialog *color_dialog)
{
    GdkRGBA color;
    gchar *color_string;

    mate_color_selection_get_current_rgba (MATE_COLOR_SELECTION (color_dialog->colorsel), &color);
    g_object_get (color_dialog->colorsel, "hex-string", &color_string, NULL);

    gtk_clipboard_set_text (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD), color_string, -1);

    g_free (color_string);
    return 0;
}

int
main (int argc, char **argv)
{
    GtkWidget *color_dialog = NULL;
    GtkWidget *color_selection;
    GtkWidget *widget;
    GtkWidget *image;

    bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    /* initialize GTK+ */
    gtk_init (&argc, &argv);
    gtk_window_set_default_icon_name ("gtk-select-color");

    color_dialog = mate_color_selection_dialog_new (_("MATE Color Selection"));
    color_selection = MATE_COLOR_SELECTION_DIALOG (color_dialog)->colorsel;
    mate_color_selection_set_has_palette (MATE_COLOR_SELECTION (color_selection), TRUE);

    /* quit signal */
    g_signal_connect (color_dialog, "destroy", gtk_main_quit, NULL);

    widget = gtk_button_new_with_mnemonic (_("_Save"));
    image = gtk_image_new_from_icon_name ("go-up", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image (GTK_BUTTON (widget), image);
    gtk_dialog_add_action_widget (GTK_DIALOG (color_dialog), widget, GTK_RESPONSE_ACCEPT);
    g_signal_connect (widget, "button-release-event", G_CALLBACK (save), color_dialog);

    widget = gtk_button_new_with_mnemonic (_("_Load"));
    image = gtk_image_new_from_icon_name ("go-down", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image (GTK_BUTTON (widget), image);
    gtk_dialog_add_action_widget (GTK_DIALOG (color_dialog), widget, GTK_RESPONSE_ACCEPT);
    g_signal_connect (widget, "button-release-event", G_CALLBACK (load), color_dialog);

    widget = gtk_button_new_with_mnemonic (_("_Reset"));
    image = gtk_image_new_from_icon_name ("go-jump", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image (GTK_BUTTON (widget), image);
    gtk_dialog_add_action_widget (GTK_DIALOG (color_dialog), widget, GTK_RESPONSE_ACCEPT);
    g_signal_connect (widget, "button-release-event", G_CALLBACK (reset), color_dialog);

    widget = gtk_button_new_with_mnemonic (_("_Copy"));
    image = gtk_image_new_from_icon_name ("edit-copy", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image (GTK_BUTTON (widget), image);
    gtk_dialog_add_action_widget (GTK_DIALOG (color_dialog), widget, GTK_RESPONSE_ACCEPT);
    g_signal_connect (widget, "button-release-event", G_CALLBACK (copy_color), color_dialog);

    widget = gtk_button_new_with_mnemonic (_("_Close"));
    image = gtk_image_new_from_icon_name ("window-close", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image (GTK_BUTTON (widget), image);
    gtk_dialog_add_action_widget (GTK_DIALOG (color_dialog), widget, GTK_RESPONSE_CLOSE);
    g_signal_connect (widget, "button-release-event", gtk_main_quit, NULL);

    gtk_widget_show_all (color_dialog);

    /* start application */
    gtk_main ();
    return 0;
}
