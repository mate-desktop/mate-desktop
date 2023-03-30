/*
 * Copyright (C) 2023 zhuyaliang.
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "mate-image-menu-item.h"
static void
test_menu_cb (GtkMenuItem *menuitem,
              gpointer     user_data)
{
    GtkWidget  *image;
    const char *icon_name;

    image = mate_image_menu_item_get_image (MATE_IMAGE_MENU_ITEM (menuitem));
    gtk_image_get_icon_name (GTK_IMAGE (image), &icon_name, NULL);
    g_print ("menu item icon is %s\r\n", icon_name);
}

int main (int argc, char **argv)
{
    GtkWidget *window;
    GtkWidget *box;
    GtkWidget *button;
    GtkWidget *menu;
    GtkWidget *menuitem;
    GtkWidget *label;
    GtkWidget *image;

    gtk_init (&argc, &argv);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size (GTK_WINDOW (window), 350, 220);
    g_signal_connect ((window), "delete_event", G_CALLBACK(gtk_main_quit), NULL);

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_add (GTK_CONTAINER (window), box);

    button = gtk_menu_button_new ();
    gtk_widget_set_valign (button, GTK_ALIGN_CENTER);
    gtk_widget_set_halign (button, GTK_ALIGN_CENTER);

    gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 6);

    menu = gtk_menu_new ();
    gtk_menu_button_set_popup (GTK_MENU_BUTTON (button), menu);

    menuitem = mate_image_menu_item_new ();
    label = gtk_label_new ("test image_menu_item_new");
    gtk_container_add (GTK_CONTAINER (menuitem), label);
    image = gtk_image_new_from_icon_name ("edit-copy", GTK_ICON_SIZE_MENU);
    mate_image_menu_item_set_image (MATE_IMAGE_MENU_ITEM (menuitem), image);
    gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem);
    g_signal_connect (menuitem, "activate", G_CALLBACK (test_menu_cb), NULL);

    menuitem = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem);

    menuitem = mate_image_menu_item_new_with_label ("test mate_new_with_label");
    image = gtk_image_new_from_icon_name ("edit-clear-all", GTK_ICON_SIZE_MENU);
    mate_image_menu_item_set_image (MATE_IMAGE_MENU_ITEM (menuitem), image);
    gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem);
    g_signal_connect (menuitem, "activate", G_CALLBACK (test_menu_cb), NULL);

    menuitem = mate_image_menu_item_new_with_mnemonic ("test mate_new_with_mnemonic");
    image = gtk_image_new_from_icon_name ("edit-find-replace", GTK_ICON_SIZE_MENU);
    mate_image_menu_item_set_image (MATE_IMAGE_MENU_ITEM (menuitem), image);
    gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem);
    g_signal_connect (menuitem, "activate", G_CALLBACK (test_menu_cb), NULL);

    gtk_widget_show_all (menu);
    gtk_widget_show_all (window);

    gtk_main();

    return 0;
}
