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

#include "config.h"
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include "mate-image-menu-item.h"

struct _MateImageMenuItemPrivate
{
    GtkWidget *image;
};

enum {
    PROP_0,
    PROP_IMAGE,
};

G_DEFINE_TYPE_WITH_PRIVATE (MateImageMenuItem, mate_image_menu_item, GTK_TYPE_MENU_ITEM)

static void
mate_image_menu_item_destroy (GtkWidget *widget)
{
    MateImageMenuItem *image_menu_item = MATE_IMAGE_MENU_ITEM (widget);
    MateImageMenuItemPrivate *priv = image_menu_item->priv;

    if (priv->image)
        gtk_container_remove (GTK_CONTAINER (image_menu_item),
                              priv->image);

    GTK_WIDGET_CLASS (mate_image_menu_item_parent_class)->destroy (widget);
}

static void
mate_image_menu_item_get_preferred_width (GtkWidget *widget,
                                          gint      *minimum,
                                          gint      *natural)
{
    MateImageMenuItem *image_menu_item = MATE_IMAGE_MENU_ITEM (widget);
    MateImageMenuItemPrivate *priv = image_menu_item->priv;
    GtkPackDirection pack_dir;
    GtkWidget *parent;

    parent = gtk_widget_get_parent (widget);

    if (GTK_IS_MENU_BAR (parent))
        pack_dir = gtk_menu_bar_get_child_pack_direction (GTK_MENU_BAR (parent));
    else
        pack_dir = GTK_PACK_DIRECTION_LTR;

    GTK_WIDGET_CLASS (mate_image_menu_item_parent_class)->get_preferred_width (widget, minimum, natural);

    if ((pack_dir == GTK_PACK_DIRECTION_TTB || pack_dir == GTK_PACK_DIRECTION_BTT) &&
        priv->image &&
        gtk_widget_get_visible (priv->image))
    {
        gint child_minimum, child_natural;

        gtk_widget_get_preferred_width (priv->image, &child_minimum, &child_natural);

        *minimum = MAX (*minimum, child_minimum);
        *natural = MAX (*natural, child_natural);
    }
}

static void
mate_image_menu_item_get_preferred_height (GtkWidget *widget,
                                           gint      *minimum,
                                           gint      *natural)
{
    MateImageMenuItem *image_menu_item = MATE_IMAGE_MENU_ITEM (widget);
    MateImageMenuItemPrivate *priv = image_menu_item->priv;
    gint child_height = 0;
    GtkPackDirection pack_dir;
    GtkWidget *parent;

    parent = gtk_widget_get_parent (widget);

    if (GTK_IS_MENU_BAR (parent))
        pack_dir = gtk_menu_bar_get_child_pack_direction (GTK_MENU_BAR (parent));
    else
        pack_dir = GTK_PACK_DIRECTION_LTR;

    if (priv->image && gtk_widget_get_visible (priv->image))
    {
        GtkRequisition child_requisition;

        gtk_widget_get_preferred_size (priv->image, &child_requisition, NULL);

        child_height = child_requisition.height;
    }

    GTK_WIDGET_CLASS (mate_image_menu_item_parent_class)->get_preferred_height (widget, minimum, natural);

    if (pack_dir == GTK_PACK_DIRECTION_RTL || pack_dir == GTK_PACK_DIRECTION_LTR)
    {
        *minimum = MAX (*minimum, child_height);
        *natural = MAX (*natural, child_height);
    }
}

static void
mate_image_menu_item_get_preferred_height_for_width (GtkWidget *widget,
                                                     gint       width,
                                                     gint      *minimum,
                                                     gint      *natural)
{
    MateImageMenuItem *image_menu_item = MATE_IMAGE_MENU_ITEM (widget);
    MateImageMenuItemPrivate *priv = image_menu_item->priv;
    gint child_height = 0;
    GtkPackDirection pack_dir;
    GtkWidget *parent;

    parent = gtk_widget_get_parent (widget);

    if (GTK_IS_MENU_BAR (parent))
        pack_dir = gtk_menu_bar_get_child_pack_direction (GTK_MENU_BAR (parent));
    else
        pack_dir = GTK_PACK_DIRECTION_LTR;

    if (priv->image && gtk_widget_get_visible (priv->image))
    {
        GtkRequisition child_requisition;

        gtk_widget_get_preferred_size (priv->image, &child_requisition, NULL);

        child_height = child_requisition.height;
    }

    GTK_WIDGET_CLASS (mate_image_menu_item_parent_class)->get_preferred_height_for_width (widget, width, minimum, natural);

    if (pack_dir == GTK_PACK_DIRECTION_RTL || pack_dir == GTK_PACK_DIRECTION_LTR)
    {
        *minimum = MAX (*minimum, child_height);
        *natural = MAX (*natural, child_height);
    }
}

static void
mate_image_menu_item_size_allocate (GtkWidget     *widget,
                                    GtkAllocation *allocation)
{
    MateImageMenuItem *image_menu_item = MATE_IMAGE_MENU_ITEM (widget);
    MateImageMenuItemPrivate *priv = image_menu_item->priv;
    GtkAllocation widget_allocation;
    GtkRequisition image_requisition;
    GtkPackDirection pack_dir;
    GtkTextDirection text_dir;
    GtkAllocation image_allocation;
    GtkStyleContext *context;
    GtkStateFlags state;
    GtkBorder padding;
    GtkWidget *parent;
    gint toggle_size;
    gint x;
    gint y;

    parent = gtk_widget_get_parent (widget);

    if (GTK_IS_MENU_BAR (parent))
        pack_dir = gtk_menu_bar_get_child_pack_direction (GTK_MENU_BAR (parent));
    else
        pack_dir = GTK_PACK_DIRECTION_LTR;

    GTK_WIDGET_CLASS (mate_image_menu_item_parent_class)->size_allocate (widget, allocation);

    if (!priv->image || !gtk_widget_get_visible (priv->image))
        return;

    gtk_widget_get_allocation (widget, &widget_allocation);
    gtk_widget_get_preferred_size (priv->image, &image_requisition, NULL);

    context = gtk_widget_get_style_context (widget);
    state = gtk_style_context_get_state (context);
    gtk_style_context_get_padding (context, state, &padding);

    toggle_size = 0;
    gtk_menu_item_toggle_size_request (GTK_MENU_ITEM (image_menu_item), &toggle_size);

    text_dir = gtk_widget_get_direction (widget);

    if (pack_dir == GTK_PACK_DIRECTION_LTR || pack_dir == GTK_PACK_DIRECTION_RTL)
    {
        if ((text_dir == GTK_TEXT_DIR_LTR) == (pack_dir == GTK_PACK_DIRECTION_LTR))
        {
            x = padding.left + (toggle_size - image_requisition.width) / 2;
        }
        else
        {
            x = widget_allocation.width - padding.right - toggle_size +
                (toggle_size - image_requisition.width) / 2;
        }

        y = (widget_allocation.height - image_requisition.height) / 2;
    }
    else
    {
        if ((text_dir == GTK_TEXT_DIR_LTR) == (pack_dir == GTK_PACK_DIRECTION_TTB))
        {
            y = padding.top + (toggle_size - image_requisition.height) / 2;
        }
        else
        {
            y = widget_allocation.height - padding.bottom - toggle_size +
                (toggle_size - image_requisition.height) / 2;
        }

        x = (widget_allocation.width - image_requisition.width) / 2;
    }

    image_allocation.x = widget_allocation.x + MAX (0, x);
    image_allocation.y = widget_allocation.y + MAX (0, y);
    image_allocation.width = image_requisition.width;
    image_allocation.height = image_requisition.height;

    gtk_widget_size_allocate (priv->image, &image_allocation);
}

static void
mate_image_menu_item_add (GtkContainer *container,
                          GtkWidget    *widget)
{
    GTK_CONTAINER_CLASS (mate_image_menu_item_parent_class)->add (container, widget);
}

static void
mate_image_menu_item_forall (GtkContainer   *container,
                             gboolean        include_internals,
                             GtkCallback     callback,
                             gpointer        callback_data)
{
    MateImageMenuItem *image_menu_item = MATE_IMAGE_MENU_ITEM (container);
    MateImageMenuItemPrivate *priv = image_menu_item->priv;

    GTK_CONTAINER_CLASS (mate_image_menu_item_parent_class)->forall (container,
                                                                     include_internals,
                                                                     callback,
                                                                     callback_data);

    if (include_internals && priv->image)
        (* callback) (priv->image, callback_data);
}

static void
mate_image_menu_item_remove (GtkContainer *container,
                             GtkWidget    *child)
{
    MateImageMenuItem *image_menu_item = MATE_IMAGE_MENU_ITEM (container);
    MateImageMenuItemPrivate *priv = image_menu_item->priv;

    if (child == priv->image)
    {
        gboolean widget_was_visible;

        widget_was_visible = gtk_widget_get_visible (child);

        gtk_widget_unparent (child);
        priv->image = NULL;

        if (widget_was_visible &&
            gtk_widget_get_visible (GTK_WIDGET (container)))
            gtk_widget_queue_resize (GTK_WIDGET (container));

        g_object_notify (G_OBJECT (image_menu_item), "image");
    }
    else
    {
        GTK_CONTAINER_CLASS (mate_image_menu_item_parent_class)->remove (container, child);
    }
}

static void
mate_image_menu_item_toggle_size_request (GtkMenuItem *menu_item,
                                          gint        *requisition)
{
    MateImageMenuItem *image_menu_item = MATE_IMAGE_MENU_ITEM (menu_item);
    MateImageMenuItemPrivate *priv = image_menu_item->priv;
    GtkPackDirection pack_dir;
    GtkWidget *parent;
    GtkWidget *widget = GTK_WIDGET (menu_item);

    parent = gtk_widget_get_parent (widget);

    if (GTK_IS_MENU_BAR (parent))
        pack_dir = gtk_menu_bar_get_child_pack_direction (GTK_MENU_BAR (parent));
    else
        pack_dir = GTK_PACK_DIRECTION_LTR;

    *requisition = 0;

    if (priv->image && gtk_widget_get_visible (priv->image))
    {
        GtkRequisition image_requisition;
        guint toggle_spacing;

        gtk_widget_get_preferred_size (priv->image, &image_requisition, NULL);

        gtk_widget_style_get (GTK_WIDGET (menu_item),
                             "toggle-spacing", &toggle_spacing,
                              NULL);

        if (pack_dir == GTK_PACK_DIRECTION_LTR || pack_dir == GTK_PACK_DIRECTION_RTL)
        {
            if (image_requisition.width > 0)
              *requisition = image_requisition.width + toggle_spacing;
        }
        else
        {
            if (image_requisition.height > 0)
              *requisition = image_requisition.height + toggle_spacing;
        }
    }
}

static void
mate_image_menu_item_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
    MateImageMenuItem *image_menu_item = MATE_IMAGE_MENU_ITEM (object);

    switch (prop_id)
    {
        case PROP_IMAGE:
            mate_image_menu_item_set_image (image_menu_item, (GtkWidget *) g_value_get_object (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mate_image_menu_item_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
    MateImageMenuItem *image_menu_item = MATE_IMAGE_MENU_ITEM (object);

    switch (prop_id)
    {
        case PROP_IMAGE:
            g_value_set_object (value, mate_image_menu_item_get_image (image_menu_item));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mate_image_menu_item_class_init (MateImageMenuItemClass *class)
{
    GObjectClass *gobject_class = (GObjectClass*) class;
    GtkWidgetClass *widget_class = (GtkWidgetClass*) class;
    GtkMenuItemClass *menu_item_class = (GtkMenuItemClass*) class;
    GtkContainerClass *container_class = (GtkContainerClass*) class;

    widget_class->destroy = mate_image_menu_item_destroy;
    widget_class->get_preferred_width = mate_image_menu_item_get_preferred_width;
    widget_class->get_preferred_height = mate_image_menu_item_get_preferred_height;
    widget_class->get_preferred_height_for_width = mate_image_menu_item_get_preferred_height_for_width;
    widget_class->size_allocate = mate_image_menu_item_size_allocate;

    container_class->add    = mate_image_menu_item_add;
    container_class->forall = mate_image_menu_item_forall;
    container_class->remove = mate_image_menu_item_remove;

    menu_item_class->toggle_size_request = mate_image_menu_item_toggle_size_request;

    gobject_class->set_property = mate_image_menu_item_set_property;
    gobject_class->get_property = mate_image_menu_item_get_property;

  /**
   * MateImageMenuItem:image:
   *
   * Child widget to appear next to the menu text.
   *
   */
    g_object_class_install_property (gobject_class,
                                     PROP_IMAGE,
                                     g_param_spec_object ("image",
                                                          _("Image widget"),
                                                          _("Child widget to appear next to the menu text"),
                                                          GTK_TYPE_WIDGET,
                                                          G_PARAM_READWRITE | G_PARAM_DEPRECATED));
}

static void
mate_image_menu_item_init (MateImageMenuItem *image_menu_item)
{
    MateImageMenuItemPrivate *priv;

    image_menu_item->priv = mate_image_menu_item_get_instance_private (image_menu_item);
    priv = image_menu_item->priv;

    priv->image = NULL;
}

/**
 * mate_image_menu_item_new:
 *
 * Creates a new #MateImageMenuItem with an empty label.
 *
 * Returns: a new #MateImageMenuItem
 *
 */
GtkWidget*
mate_image_menu_item_new (void)
{
    return g_object_new (MATE_TYPE_IMAGE_MENU_ITEM, NULL);
}

/**
 * mate_image_menu_item_new_with_label:
 * @label: the text of the menu item.
 *
 * Creates a new #MateImageMenuItem containing a label.
 *
 * Returns: a new #MateImageMenuItem.
 *
 */
GtkWidget*
mate_image_menu_item_new_with_label (const gchar *label)
{
    return g_object_new (MATE_TYPE_IMAGE_MENU_ITEM,
                         "label", label,
                         NULL);
}

/**
 * mate_image_menu_item_new_with_mnemonic:
 * @label: the text of the menu item, with an underscore in front of the
 *         mnemonic character
 *
 * Creates a new #MateImageMenuItem containing a label. The label
 * will be created using gtk_label_new_with_mnemonic(), so underscores
 * in @label indicate the mnemonic for the menu item.
 *
 * Returns: a new #MateImageMenuItem
 *
 */
GtkWidget*
mate_image_menu_item_new_with_mnemonic (const gchar *label)
{
    return g_object_new (MATE_TYPE_IMAGE_MENU_ITEM,
                         "use-underline", TRUE,
                         "label", label,
                         NULL);
}

/**
 * mate_image_menu_item_set_image:
 * @image_menu_item: a #MateImageMenuItem.
 * @image: (allow-none): a widget to set as the image for the menu item.
 *
 * Sets the image of @image_menu_item to the given widget.
 * Note that it depends on the show-menu-images setting whether
 * the image will be displayed or not.
 *
 */
void
mate_image_menu_item_set_image (MateImageMenuItem *image_menu_item,
                                GtkWidget         *image)
{
    MateImageMenuItemPrivate *priv;

    g_return_if_fail (MATE_IS_IMAGE_MENU_ITEM (image_menu_item));

    priv = image_menu_item->priv;

    if (image == priv->image)
        return;

    if (priv->image)
        gtk_container_remove (GTK_CONTAINER (image_menu_item),
                              priv->image);

    priv->image = image;

    if (image == NULL)
        return;

    gtk_widget_set_parent (image, GTK_WIDGET (image_menu_item));
    g_object_set (image,
                  "visible", TRUE,
                  "no-show-all", TRUE,
                  NULL);
    gtk_image_set_pixel_size (GTK_IMAGE (image), 16);

    g_object_notify (G_OBJECT (image_menu_item), "image");
}

/**
 * mate_image_menu_item_get_image:
 * @image_menu_item: a #MateImageMenuItem
 *
 * Gets the widget that is currently set as the image of @image_menu_item.
 * See mate_image_menu_item_set_image().
 *
 * Returns: (transfer none): the widget set as image of @image_menu_item
 *
 **/
GtkWidget*
mate_image_menu_item_get_image (MateImageMenuItem *image_menu_item)
{
    g_return_val_if_fail (MATE_IS_IMAGE_MENU_ITEM (image_menu_item), NULL);

    return image_menu_item->priv->image;
}
