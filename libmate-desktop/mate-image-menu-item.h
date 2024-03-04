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
#define MATE_IMAGE_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_IMAGE_MENU_ITEM, MateImageMenuItem))
#define MATE_IMAGE_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_IMAGE_MENU_ITEM, MateImageMenuItemClass))
#define MATE_IS_IMAGE_MENU_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_IMAGE_MENU_ITEM))
#define MATE_IS_IMAGE_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_IMAGE_MENU_ITEM))
#define MATE_IMAGE_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_IMAGE_MENU_ITEM, MateImageMenuItemClass))

typedef struct _MateImageMenuItem              MateImageMenuItem;
typedef struct _MateImageMenuItemPrivate       MateImageMenuItemPrivate;
typedef struct _MateImageMenuItemClass         MateImageMenuItemClass;

struct _MateImageMenuItem
{
    GtkMenuItem menu_item;

  /*< private >*/
    MateImageMenuItemPrivate *priv;
};

/**
 * MateImageMenuItemClass:
 * @parent_class: The parent class.
 */
struct _MateImageMenuItemClass
{
    GtkMenuItemClass parent_class;
};

GType      mate_image_menu_item_get_type          (void) G_GNUC_CONST;

GtkWidget* mate_image_menu_item_new               (void);

GtkWidget* mate_image_menu_item_new_with_label    (const gchar       *label);

GtkWidget* mate_image_menu_item_new_with_mnemonic (const gchar       *label);

void       mate_image_menu_item_set_image         (MateImageMenuItem *image_menu_item,
                                                   GtkWidget         *image);

GtkWidget* mate_image_menu_item_get_image         (MateImageMenuItem *image_menu_item);

G_END_DECLS

#endif /* __GTK_IMAGE_MENU_ITEM_H__ */
