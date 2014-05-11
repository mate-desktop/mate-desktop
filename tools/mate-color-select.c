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

#define mate_gettext(package, locale, codeset) \
    bindtextdomain(package, locale); \
    bind_textdomain_codeset(package, codeset); \
    textdomain(package);

int
main (int argc, char **argv)
{
    GtkWidget *color_dialog = NULL;

    mate_gettext (GETTEXT_PACKAGE, LOCALE_DIR, "UTF-8");

    /* initialize GTK+ */
    gtk_init (&argc, &argv);

    color_dialog = mate_color_selection_dialog_new (_("MATE Color Selection"));

    /* quit signal */
    g_signal_connect (color_dialog, "destroy", gtk_main_quit, NULL);

    gtk_widget_show_all (color_dialog);
    gtk_widget_hide (MATE_COLOR_SELECTION_DIALOG (color_dialog)->ok_button);
    gtk_widget_hide (MATE_COLOR_SELECTION_DIALOG (color_dialog)->cancel_button);
    gtk_widget_hide (MATE_COLOR_SELECTION_DIALOG (color_dialog)->help_button);

    /* start application */
    gtk_main ();
}
