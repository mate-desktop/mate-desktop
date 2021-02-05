/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2011 Perberos <perberos@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "mate-about.h"

static void mate_about_on_activate(GtkApplication* app)
{
    GList* list;

    list = gtk_application_get_windows(app);

    if (list)
    {
        gtk_window_present(GTK_WINDOW(list->data));
    }
    else
    {
        mate_about_run();
    }
}

void mate_about_run(void)
{
    mate_about_dialog = (GtkAboutDialog*) gtk_about_dialog_new();

    gtk_window_set_default_icon_name(icon);
    gtk_about_dialog_set_logo_icon_name(mate_about_dialog, icon);

    // name
    gtk_about_dialog_set_program_name(mate_about_dialog, _(program_name));

    // version
    gtk_about_dialog_set_version(mate_about_dialog, version);

    // credits and website
    gtk_about_dialog_set_copyright(mate_about_dialog, _(copyright));
    gtk_about_dialog_set_website(mate_about_dialog, website);

    /**
     * This generate a random message.
     * The comments index must not be more than comments_count - 1
     */
    gtk_about_dialog_set_comments(mate_about_dialog, gettext(comments_array[g_random_int_range(0, comments_count - 1)]));

    gtk_about_dialog_set_authors(mate_about_dialog, authors);
    gtk_about_dialog_set_artists(mate_about_dialog, artists);
    gtk_about_dialog_set_documenters(mate_about_dialog, documenters);
    /* Translators should localize the following string which will be
     * displayed in the about box to give credit to the translator(s). */
    gtk_about_dialog_set_translator_credits(mate_about_dialog, _("translator-credits"));

    gtk_window_set_application(GTK_WINDOW(mate_about_dialog), mate_about_application);

    // start and destroy
    gtk_dialog_run((GtkDialog*) mate_about_dialog);
    gtk_widget_destroy((GtkWidget*) mate_about_dialog);
}

int main(int argc, char** argv)
{
    int status = 0;

    bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    GOptionContext* context = g_option_context_new(NULL);
    g_option_context_add_main_entries(context, command_entries, GETTEXT_PACKAGE);
    g_option_context_add_group(context, gtk_get_option_group(TRUE));
    g_option_context_parse(context, &argc, &argv, NULL);
    g_option_context_free(context);

    if (mate_about_nogui == TRUE)
    {
        printf("%s %s\n", gettext(program_name), version);
    }
    else
    {
        gtk_init(&argc, &argv);

        mate_about_application = gtk_application_new("org.mate.about", 0);
        g_signal_connect(mate_about_application, "activate", G_CALLBACK(mate_about_on_activate), NULL);

        status = g_application_run(G_APPLICATION(mate_about_application), argc, argv);

        g_object_unref(mate_about_application);
    }

    return status;
}
