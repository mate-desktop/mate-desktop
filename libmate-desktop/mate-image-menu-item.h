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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __MATE_IMAGE_MENU_ITEM_H__
#define __MATE_IMAGE_MENU_ITEM_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MATE_TYPE_IMAGE_MENU_ITEM            (mate_image_menu_item_get_type ())

G_DECLARE_FINAL_TYPE (MateImageMenuItem, mate_image_menu_item, MATE, IMAGE_MENU_ITEM, GtkMenuItem);

GtkWidget* mate_image_menu_item_new               (void);

GtkWidget* mate_image_menu_item_new_with_label    (const gchar       *label);

GtkWidget* mate_image_menu_item_new_with_mnemonic (const gchar       *label);

void       mate_image_menu_item_set_image         (MateImageMenuItem *image_menu_item,
                                                   GtkWidget         *image);

GtkWidget* mate_image_menu_item_get_image         (MateImageMenuItem *image_menu_item);

G_END_DECLS

#endif /* __GTK_IMAGE_MENU_ITEM_H__ */
