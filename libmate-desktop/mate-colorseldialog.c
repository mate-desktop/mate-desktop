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
#include "config.h"
#include "private.h"
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include "mate-colorsel.h"
#include "mate-colorseldialog.h"

enum {
  PROP_0,
  PROP_COLOR_SELECTION,
  PROP_OK_BUTTON,
  PROP_CANCEL_BUTTON,
  PROP_HELP_BUTTON
};


/***************************/
/* MateColorSelectionDialog */
/***************************/

static void mate_color_selection_dialog_buildable_interface_init     (GtkBuildableIface *iface);
static GObject * mate_color_selection_dialog_buildable_get_internal_child (GtkBuildable *buildable,
									  GtkBuilder   *builder,
									  const gchar  *childname);

G_DEFINE_TYPE_WITH_CODE (MateColorSelectionDialog, mate_color_selection_dialog,
           GTK_TYPE_DIALOG,
           G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE,
                      mate_color_selection_dialog_buildable_interface_init))

static GtkBuildableIface *parent_buildable_iface;

static void
mate_color_selection_dialog_get_property (GObject         *object,
					 guint            prop_id,
					 GValue          *value,
					 GParamSpec      *pspec)
{
  MateColorSelectionDialog *colorsel;

  colorsel = MATE_COLOR_SELECTION_DIALOG (object);

  switch (prop_id)
    {
    case PROP_COLOR_SELECTION:
      g_value_set_object (value, colorsel->colorsel);
      break;
    case PROP_OK_BUTTON:
      g_value_set_object (value, colorsel->ok_button);
      break;
    case PROP_CANCEL_BUTTON:
      g_value_set_object (value, colorsel->cancel_button);
      break;
    case PROP_HELP_BUTTON:
      g_value_set_object (value, colorsel->help_button);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
mate_color_selection_dialog_class_init (MateColorSelectionDialogClass *klass)
{
  GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = mate_color_selection_dialog_get_property;

  g_object_class_install_property (gobject_class,
				   PROP_COLOR_SELECTION,
				   g_param_spec_object ("color-selection",
						     _("Color Selection"),
						     _("The color selection embedded in the dialog."),
						     GTK_TYPE_WIDGET,
						     G_PARAM_READABLE));
  g_object_class_install_property (gobject_class,
				   PROP_OK_BUTTON,
				   g_param_spec_object ("ok-button",
						     _("OK Button"),
						     _("The OK button of the dialog."),
						     GTK_TYPE_WIDGET,
						     G_PARAM_READABLE));
  g_object_class_install_property (gobject_class,
				   PROP_CANCEL_BUTTON,
				   g_param_spec_object ("cancel-button",
						     _("Cancel Button"),
						     _("The cancel button of the dialog."),
						     GTK_TYPE_WIDGET,
						     G_PARAM_READABLE));
  g_object_class_install_property (gobject_class,
				   PROP_HELP_BUTTON,
				   g_param_spec_object ("help-button",
						     _("Help Button"),
						     _("The help button of the dialog."),
						     GTK_TYPE_WIDGET,
						     G_PARAM_READABLE));
}

static void
mate_color_selection_dialog_init (MateColorSelectionDialog *colorseldiag)
{
  GtkDialog *dialog = GTK_DIALOG (colorseldiag);

#if GTK_CHECK_VERSION (3, 12, 0)
  gint use_header_bar;

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (dialog)),
                "gtk-dialogs-use-header", &use_header_bar,
                NULL);
  g_object_set (colorseldiag, "use-header-bar", use_header_bar, NULL);
#endif

  _mate_desktop_init_i18n ();

  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (dialog)), 2); /* 2 * 5 + 2 = 12 */
  gtk_container_set_border_width (GTK_CONTAINER (gtk_dialog_get_action_area (dialog)), 5);
  gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_action_area (dialog)), 6);

  colorseldiag->colorsel = mate_color_selection_new ();
  gtk_container_set_border_width (GTK_CONTAINER (colorseldiag->colorsel), 5);
  mate_color_selection_set_has_palette (MATE_COLOR_SELECTION(colorseldiag->colorsel), FALSE); 
  mate_color_selection_set_has_opacity_control (MATE_COLOR_SELECTION(colorseldiag->colorsel), FALSE);
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (colorseldiag))), colorseldiag->colorsel);
  gtk_widget_show (colorseldiag->colorsel);
  
  colorseldiag->cancel_button = gtk_dialog_add_button (GTK_DIALOG (colorseldiag),
                                                       GTK_STOCK_CANCEL,
                                                       GTK_RESPONSE_CANCEL);

  colorseldiag->ok_button = gtk_dialog_add_button (GTK_DIALOG (colorseldiag),
                                                   GTK_STOCK_OK,
                                                   GTK_RESPONSE_OK);
                                                   
  gtk_widget_grab_default (colorseldiag->ok_button);
  
  colorseldiag->help_button = gtk_dialog_add_button (GTK_DIALOG (colorseldiag),
                                                     GTK_STOCK_HELP,
                                                     GTK_RESPONSE_HELP);

  gtk_widget_hide (colorseldiag->help_button);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (colorseldiag),
					   GTK_RESPONSE_OK,
					   GTK_RESPONSE_CANCEL,
					   GTK_RESPONSE_HELP,
					   -1);

  gtk_window_set_title (GTK_WINDOW (colorseldiag),
                        _("Color Selection"));

  //_gtk_dialog_set_ignore_separator (dialog, TRUE);
}

GtkWidget*
mate_color_selection_dialog_new (const gchar *title)
{
  MateColorSelectionDialog *colorseldiag;
  
  colorseldiag = g_object_new (MATE_TYPE_COLOR_SELECTION_DIALOG, NULL);

  if (title)
    gtk_window_set_title (GTK_WINDOW (colorseldiag), title);

  gtk_window_set_resizable (GTK_WINDOW (colorseldiag), FALSE);
  
  return GTK_WIDGET (colorseldiag);
}

/**
 * mate_color_selection_dialog_get_color_selection:
 * @colorsel: a #MateColorSelectionDialog
 *
 * Retrieves the #MateColorSelection widget embedded in the dialog.
 *
 * Returns: (transfer none): the embedded #MateColorSelection
 *
 * Since: 1.9.1
 **/
GtkWidget*
mate_color_selection_dialog_get_color_selection (MateColorSelectionDialog *colorsel)
{
  g_return_val_if_fail (MATE_IS_COLOR_SELECTION_DIALOG (colorsel), NULL);

  return colorsel->colorsel;
}

static void
mate_color_selection_dialog_buildable_interface_init (GtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->get_internal_child = mate_color_selection_dialog_buildable_get_internal_child;
}

static GObject *
mate_color_selection_dialog_buildable_get_internal_child (GtkBuildable *buildable,
							 GtkBuilder   *builder,
							 const gchar  *childname)
{
    if (strcmp(childname, "ok_button") == 0)
	return G_OBJECT (MATE_COLOR_SELECTION_DIALOG (buildable)->ok_button);
    else if (strcmp(childname, "cancel_button") == 0)
	return G_OBJECT (MATE_COLOR_SELECTION_DIALOG (buildable)->cancel_button);
    else if (strcmp(childname, "help_button") == 0)
	return G_OBJECT (MATE_COLOR_SELECTION_DIALOG(buildable)->help_button);
    else if (strcmp(childname, "color_selection") == 0)
	return G_OBJECT (MATE_COLOR_SELECTION_DIALOG(buildable)->colorsel);

    return parent_buildable_iface->get_internal_child (buildable, builder, childname);
}
