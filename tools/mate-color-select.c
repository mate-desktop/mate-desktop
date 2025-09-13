/*
 * mate-color.c: MATE color selection tool
 *
 * Copyright (C) 2014 Stefano Karapetsas
 * Copyright (C) 2014-2025 MATE Developers
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
is_enter (GdkEvent *event)
{
  switch (event->type)
    {
    case GDK_BUTTON_RELEASE:
      return TRUE;

    case GDK_KEY_RELEASE:
      switch (event->key.keyval)
        {
        case GDK_KEY_space:
        case GDK_KEY_Return:
        case GDK_KEY_ISO_Enter:
        case GDK_KEY_KP_Enter:
        case GDK_KEY_KP_Space:
          return TRUE;
        }

    default:
      return FALSE;
    }
}

static gboolean
save (GtkWidget          *widget,
      GdkEvent           *event,
      MateColorSelection *color_selection)
{
  if (is_enter(event))
    mate_color_selection_palette_save ();

  return FALSE;
}

static gboolean
load (GtkWidget          *widget,
      GdkEvent           *event,
      MateColorSelection *color_selection)
{
  if (is_enter(event))
    mate_color_selection_palette_load (color_selection);

  return FALSE;
}

static gboolean
reset (GtkWidget          *widget,
       GdkEvent           *event,
       MateColorSelection *color_selection)
{
  if (is_enter(event))
    mate_color_selection_palette_set (color_selection, NULL);

  return FALSE;
}

static gboolean
copy (GtkWidget          *widget,
      GdkEvent           *event,
      MateColorSelection *color_selection)
{
  gchar *color_string;

  if (is_enter(event))
    {
      g_object_get (color_selection, "hex-string", &color_string, NULL);
      gtk_clipboard_set_text (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD), color_string, -1);

      g_free (color_string);
    }
  return FALSE;
}

static void
close2 (GtkWidget          *widget,
        GdkEvent           *event,
        MateColorSelection *color_selection)
{
  if (is_enter(event))
    gtk_main_quit();
}

static void
build_button (GtkDialog          *gtk_dialog,
              MateColorSelection *color_selection,
              const char         *button_name,
              const char         *icon_name,
              gpointer            callback)
{
  GtkWidget *widget;
  GtkWidget *image;

  widget = gtk_button_new_with_mnemonic (button_name);
  image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (widget), image);

  gtk_dialog_add_action_widget (gtk_dialog, widget, GTK_RESPONSE_ACCEPT);
  g_signal_connect (widget, "button-release-event", G_CALLBACK (callback), color_selection);
  g_signal_connect (widget, "key-release-event",    G_CALLBACK (callback), color_selection);
}

int
main (int argc, char **argv)
{
    GtkWidget *color_dialog = NULL;
    GtkWidget *color_selection;

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

    build_button (GTK_DIALOG (color_dialog), MATE_COLOR_SELECTION (color_selection), "Save",  "go-up",        (gpointer) save);
    build_button (GTK_DIALOG (color_dialog), MATE_COLOR_SELECTION (color_selection), "Load",  "go-down",      (gpointer) load);
    build_button (GTK_DIALOG (color_dialog), MATE_COLOR_SELECTION (color_selection), "Reset", "go-jump",      (gpointer) reset);
    build_button (GTK_DIALOG (color_dialog), MATE_COLOR_SELECTION (color_selection), "Copy",  "edit-copy",    (gpointer) copy);
    build_button (GTK_DIALOG (color_dialog), MATE_COLOR_SELECTION (color_selection), "Close", "window-close", (gpointer) close2);

    gtk_widget_show_all (color_dialog);

    /* start application */
    gtk_main ();
    return 0;
}
