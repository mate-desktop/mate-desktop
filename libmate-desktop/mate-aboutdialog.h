/*
 * mate-aboutdialog.h: Traditional GTK+ About Dialog
 *
 * Copyright (C) 2001 CodeFactory AB
 * Copyright (C) 2001 Anders Carlsson <andersca@codefactory.se>
 * Copyright (C) 2003, 2004 Matthias Clasen <mclasen@redhat.com>
 * Copyright (C) 2014 Stefano Karapetsas <stefano@karapetsas.com>
 *
 * This file is part of the Mate Library.
 *
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 * Author: Stefano Karapetsas <stefano@karapetsas.com>
*/

#ifndef __MATE_ABOUT_DIALOG_H__
#define __MATE_ABOUT_DIALOG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MATE_TYPE_ABOUT_DIALOG            (mate_about_dialog_get_type ())
#define MATE_ABOUT_DIALOG(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), MATE_TYPE_ABOUT_DIALOG, MateAboutDialog))
#define MATE_ABOUT_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_ABOUT_DIALOG, MateAboutDialogClass))
#define MATE_IS_ABOUT_DIALOG(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), MATE_TYPE_ABOUT_DIALOG))
#define MATE_IS_ABOUT_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_ABOUT_DIALOG))
#define MATE_ABOUT_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_ABOUT_DIALOG, MateAboutDialogClass))

typedef struct _MateAboutDialog        MateAboutDialog;
typedef struct _MateAboutDialogClass   MateAboutDialogClass;

/**
 * MateAboutDialog:
 *
 * The <structname>MateAboutDialog</structname> struct contains
 * only private fields and should not be directly accessed.
 */
struct _MateAboutDialog 
{
  GtkDialog parent_instance;

  /*< private >*/
  GtkAboutDialogPrivate *private_data;
};

struct _MateAboutDialogClass
{
  GtkDialogClass parent_class;

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
};

GType                  mate_about_dialog_get_type               (void) G_GNUC_CONST;
GtkWidget             *mate_about_dialog_new                    (void);
void                   mate_show_about_dialog                   (GtkWindow       *parent,
                                                                 const gchar     *first_property_name,
                                                                 ...) G_GNUC_NULL_TERMINATED;

const gchar *          mate_about_dialog_get_program_name       (MateAboutDialog  *about);
void                   mate_about_dialog_set_program_name       (MateAboutDialog  *about,
                                                                 const gchar      *name);
const gchar *          mate_about_dialog_get_version            (MateAboutDialog  *about);
void                   mate_about_dialog_set_version            (MateAboutDialog  *about,
                                                                 const gchar      *version);
const gchar *          mate_about_dialog_get_copyright          (MateAboutDialog  *about);
void                   mate_about_dialog_set_copyright          (MateAboutDialog  *about,
                                                                 const gchar      *copyright);
const gchar *          mate_about_dialog_get_comments           (MateAboutDialog  *about);
void                   mate_about_dialog_set_comments           (MateAboutDialog  *about,
                                                                 const gchar      *comments);
const gchar *          mate_about_dialog_get_license            (MateAboutDialog  *about);
void                   mate_about_dialog_set_license            (MateAboutDialog  *about,
                                                                 const gchar      *license);

gboolean               mate_about_dialog_get_wrap_license       (MateAboutDialog  *about);
void                   mate_about_dialog_set_wrap_license       (MateAboutDialog  *about,
                                                                 gboolean          wrap_license);

const gchar *          mate_about_dialog_get_website            (MateAboutDialog  *about);
void                   mate_about_dialog_set_website            (MateAboutDialog  *about,
                                                                 const gchar      *website);
const gchar *          mate_about_dialog_get_website_label      (MateAboutDialog  *about);
void                   mate_about_dialog_set_website_label      (MateAboutDialog  *about,
                                                                 const gchar      *website_label);
const gchar* const *   mate_about_dialog_get_authors            (MateAboutDialog  *about);
void                   mate_about_dialog_set_authors            (MateAboutDialog  *about,
                                                                 const gchar     **authors);
const gchar* const *   mate_about_dialog_get_documenters        (MateAboutDialog  *about);
void                   mate_about_dialog_set_documenters        (MateAboutDialog  *about,
                                                                 const gchar     **documenters);
const gchar* const *   mate_about_dialog_get_artists            (MateAboutDialog  *about);
void                   mate_about_dialog_set_artists            (MateAboutDialog  *about,
                                                                 const gchar     **artists);
const gchar *          mate_about_dialog_get_translator_credits (MateAboutDialog  *about);
void                   mate_about_dialog_set_translator_credits (MateAboutDialog  *about,
                                                                 const gchar      *translator_credits);
GdkPixbuf             *mate_about_dialog_get_logo               (MateAboutDialog  *about);
void                   mate_about_dialog_set_logo               (MateAboutDialog  *about,
                                                                 GdkPixbuf        *logo);
const gchar *          mate_about_dialog_get_logo_icon_name     (MateAboutDialog  *about);
void                   mate_about_dialog_set_logo_icon_name     (MateAboutDialog  *about,
                                                                 const gchar      *icon_name);

G_END_DECLS

#endif /* __MATE_ABOUT_DIALOG_H__ */


