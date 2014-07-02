/*
 * mate-color.c: MATE color selection tool
 *
 * Copyright (C) 2014 Stefano Karapetsas
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

#define mate_gettext(package, locale, codeset) \
    bindtextdomain(package, locale); \
    bind_textdomain_codeset(package, codeset); \
    textdomain(package);

gboolean
copy_color (GtkWidget *widget, GdkEvent  *event, MateColorSelectionDialog *color_dialog)
{
    GdkColor color;
    gchar *color_string;

    mate_color_selection_get_current_color (color_dialog->colorsel, &color);
    g_object_get (color_dialog->colorsel, "hex-string", &color_string, NULL);

    gtk_clipboard_set_text (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD), color_string, -1);

    g_free (color_string);
}

int
main (int argc, char **argv)
{
    GtkWidget *color_dialog = NULL;
    GtkWidget *widget;

    mate_gettext (GETTEXT_PACKAGE, LOCALE_DIR, "UTF-8");

    /* initialize GTK+ */
    gtk_init (&argc, &argv);

#if GTK_CHECK_VERSION (3, 12, 0)
    color_dialog = mate_color_selection_dialog_new_with_header_bar (_("MATE Color Selection"));
    gtk_header_bar_set_show_close_button (gtk_dialog_get_header_bar (GTK_DIALOG (color_dialog)), TRUE);
#else
    color_dialog = mate_color_selection_dialog_new (_("MATE Color Selection"));
#endif

    mate_color_selection_set_has_palette (MATE_COLOR_SELECTION_DIALOG (color_dialog)->colorsel, TRUE);

    /* quit signal */
    g_signal_connect (color_dialog, "destroy", gtk_main_quit, NULL);

    widget = gtk_button_new_from_stock (GTK_STOCK_COPY);
    gtk_container_add (gtk_dialog_get_action_area (GTK_DIALOG (color_dialog)), widget);
    g_signal_connect (widget, "button-release-event", copy_color, color_dialog);

    widget = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
    gtk_container_add (gtk_dialog_get_action_area (GTK_DIALOG (color_dialog)), widget);
    g_signal_connect (widget, "button-release-event", gtk_main_quit, NULL);

    gtk_widget_show_all (color_dialog);
    gtk_widget_hide (MATE_COLOR_SELECTION_DIALOG (color_dialog)->ok_button);
    gtk_widget_hide (MATE_COLOR_SELECTION_DIALOG (color_dialog)->cancel_button);
    gtk_widget_hide (MATE_COLOR_SELECTION_DIALOG (color_dialog)->help_button);

    /* start application */
    gtk_main ();
}
