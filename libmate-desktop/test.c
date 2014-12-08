/*
 * test.c: general tests for libmate-desktop
 *
 * Copyright (C) 2013-2014 Stefano Karapetsas
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
#include "mate-desktop.h"
#include "mate-colorbutton.h"

int
main (int argc, char **argv)
{
    GtkWindow *window = NULL;
    GtkWidget *widget = NULL;

    /* initialize GTK+ */
    gtk_init (&argc, &argv);

    /* create window */
    window = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));

    gtk_window_set_title (window, "MATE Desktop Test");

    /* create a MateColorButton */
    widget = mate_color_button_new ();

    /* add MateColorButton to window */
    gtk_container_add (GTK_CONTAINER (window), widget);

    /* quit signal */
    g_signal_connect (GTK_WIDGET (window), "destroy", gtk_main_quit, NULL);

    gtk_widget_show_all (GTK_WIDGET (window));

    /* start application */
    gtk_main ();
    return 0;
}
