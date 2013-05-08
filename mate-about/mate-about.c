/* -*- Mode: C; tab-width: 4; indent-tabs-mode: yes; c-basic-offset: 4 -*-
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

#ifndef __MATE_ABOUT_C__
#define __MATE_ABOUT_C__

#include "mate-about.h"

/* get text macro, this should be on the common macros. or not?
 */
#ifndef mate_gettext
#define mate_gettext(package, locale, codeset) \
	bindtextdomain(package, locale); \
	bind_textdomain_codeset(package, codeset); \
	textdomain(package);
#endif

//class mate-about <- pseudo-class!
//{
	//translate me:
	// El codigo escrito aqui muestra un poco el desorden de lo que genera las
	// deprecations de las librerias, segun van evolucionando en sus diferentes
	// versiones.	

	#ifdef GNUCAT_ENABLED
		// this function allow to display an animated image
		// Thanks! http://www.gtkforums.com/viewtopic.php?t=1639
		typedef struct iter_arg_s {
			GtkWidget* widget;
			GdkPixbufAnimation* animation;
			GdkPixbufAnimationIter* iter;
		} iter_arg_t;

		gboolean
		on_animation_frame(iter_arg_t* object)
		{
			if (object->widget == NULL)
			{
				return FALSE;
			}

			static gint frame = 1;

			if (gdk_pixbuf_animation_iter_advance(object->iter, NULL))
			{
				frame++;

				gtk_about_dialog_set_logo((GtkAboutDialog*) object->widget, gdk_pixbuf_animation_iter_get_pixbuf(object->iter));
			}

			return TRUE;
		}

    #endif

	static void
	mate_about_on_activate(
	#if GTK_CHECK_VERSION(3, 0, 0) && !defined(UNIQUE)
	                       GtkApplication* app
	#elif GLIB_CHECK_VERSION(2, 26, 0) && !defined(UNIQUE)
	                       GApplication* app
	#endif
	                      )
	{
		#if GTK_CHECK_VERSION(3, 0, 0) && !defined(UNIQUE)

			GList* list;
			GtkWidget* window;

			list = gtk_application_get_windows(app);

			if (list)
			{
				gtk_window_present(GTK_WINDOW(list->data));
			}
			else
			{
				mate_about_run();
			}
		
		#elif GLIB_CHECK_VERSION(2, 26, 0) && !defined(UNIQUE)
		
			if (!mate_about_dialog)
			{
				mate_about_run();
			}
			else
			{
				gtk_window_present(GTK_WINDOW(mate_about_dialog));
			}
			
		#endif
	}

	void
	mate_about_run(void)
	{
		mate_about_dialog = (GtkAboutDialog*) gtk_about_dialog_new();

		gtk_window_set_default_icon_name(icon);


		#ifdef GNUCAT_ENABLED

			/* check if it's christmas, to show a different image */
			gboolean is_christmas = FALSE;

			GDate* d = g_date_new();
			g_date_set_time_t(d, (time_t) time(NULL));

			if (g_date_get_month(d) == G_DATE_DECEMBER)
			{
				GDateDay day = g_date_get_day(d);

				if (day >= 24 && day <=25)
				{
					is_christmas = TRUE;
				}
			}

			g_date_free(d);

			if (is_christmas == TRUE)
			{
				GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(PIXMAPS_DIR "gnu-cat_navideno_v3.png", NULL);
				gtk_about_dialog_set_logo(mate_about_dialog, pixbuf);
				g_object_unref(pixbuf);
			}
			else
			{
				iter_arg_t animation_object;
				GdkPixbufAnimation* animation;
				GdkPixbufAnimationIter *iter;
				GtkWidget* image;

				animation = gdk_pixbuf_animation_new_from_file(PIXMAPS_DIR "gnu-cat.gif", NULL);

				if (animation != NULL)
				{
					iter = gdk_pixbuf_animation_get_iter(animation, NULL);

					animation_object.animation = animation;
					animation_object.iter = iter;
					animation_object.widget = (GtkWidget*) mate_about_dialog;

					gtk_about_dialog_set_logo(mate_about_dialog, gdk_pixbuf_animation_iter_get_pixbuf(iter));

					g_timeout_add(gdk_pixbuf_animation_iter_get_delay_time(iter), (GSourceFunc) on_animation_frame, (gpointer) &animation_object);
				}
			}

		#elif GTK_CHECK_VERSION(3, 0, 0) || GTK_CHECK_VERSION(2, 6, 0)

			gtk_about_dialog_set_logo_icon_name(mate_about_dialog, icon);

		#else

			GtkIconTheme* icon_theme = gtk_icon_theme_get_default();

			if (gtk_icon_theme_has_icon(icon_theme, icon))
			{
				GdkPixbuf* pixbuf = gtk_icon_theme_load_icon(icon_theme, icon, 64, 0, NULL);
				gtk_about_dialog_set_logo(mate_about_dialog, pixbuf);
				g_object_unref(pixbuf);
			}

		#endif

		// name
		#if GTK_CHECK_VERSION(3, 0, 0) || GTK_CHECK_VERSION(2, 12, 0)
			gtk_about_dialog_set_program_name(mate_about_dialog, gettext(program_name));
		#else
			gtk_about_dialog_set_name(mate_about_dialog, gettext(program_name));
		#endif

		// version
		gtk_about_dialog_set_version(mate_about_dialog, version);

		// credits and website
		gtk_about_dialog_set_copyright(mate_about_dialog, copyright);
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

		#if GTK_CHECK_VERSION(3, 0, 0)
			gtk_about_dialog_set_license_type(mate_about_dialog, GTK_LICENSE_GPL_3_0);
			gtk_about_dialog_set_wrap_license(mate_about_dialog, TRUE);
		#endif

		#ifdef USE_UNIQUE
			unique_app_watch_window(mate_about_application, (GtkWindow*) mate_about_dialog);
		#elif GTK_CHECK_VERSION(3, 0, 0) && !defined(UNIQUE)
			gtk_window_set_application(GTK_WINDOW(mate_about_dialog), mate_about_application);
		#endif

		// start and destroy
		gtk_dialog_run((GtkDialog*) mate_about_dialog);
		gtk_widget_destroy((GtkWidget*) mate_about_dialog);
	}

	int
	main(int argc, char** argv)
	{
		int status = 0;

		mate_gettext(GETTEXT_PACKAGE, LOCALE_DIR, "UTF-8");

		#if !GLIB_CHECK_VERSION(2, 36, 0)
			// g_type_init has been deprecated since version 2.36 and should not
			// be used in newly-written code. the type system is now initialised
			// automatically
			g_type_init();
		#endif

		/* http://www.gtk.org/api/2.6/glib/glib-Commandline-option-parser.html */
		GOptionContext* context = g_option_context_new(NULL);
		g_option_context_add_main_entries(context, command_entries, GETTEXT_PACKAGE);
		g_option_context_add_group(context, gtk_get_option_group(TRUE));
		g_option_context_parse(context, &argc, &argv, NULL);

		/* Not necesary at all, program just run and die.
		 * But it free a little memory. */
		g_option_context_free(context);

		if (mate_about_nogui == TRUE)
		{
			printf("%s %s\n", gettext(program_name), version);
		}
		else
		{
			gtk_init(&argc, &argv);

			/**
			 * Examples taken from:
			 * http://developer.gnome.org/gtk3/3.0/gtk-migrating-GtkApplication.html
			 */
			#ifdef USE_UNIQUE

				mate_about_application = unique_app_new("org.mate.about", NULL);

				if (unique_app_is_running(mate_about_application))
				{
					UniqueResponse response = unique_app_send_message(mate_about_application, UNIQUE_ACTIVATE, NULL);

					if (response != UNIQUE_RESPONSE_OK)
					{
						status = 1;
					}
				}
				else
				{
					mate_about_run();
				}

			#elif GTK_CHECK_VERSION(3, 0, 0) && !defined(USE_UNIQUE)

				mate_about_application = gtk_application_new("org.mate.about", 0);
				g_signal_connect(mate_about_application, "activate", G_CALLBACK(mate_about_on_activate), NULL);

				status = g_application_run(G_APPLICATION(mate_about_application), argc, argv);

				g_object_unref(mate_about_application);

			#elif GLIB_CHECK_VERSION(2, 26, 0) && !defined(USE_UNIQUE)

				mate_about_application = g_application_new("org.mate.about", G_APPLICATION_FLAGS_NONE);
				g_signal_connect(mate_about_application, "activate", G_CALLBACK(mate_about_on_activate), NULL);

				status = g_application_run(G_APPLICATION(mate_about_application), argc, argv);

				g_object_unref(mate_about_application);

			#else

				mate_about_run();

			#endif
		}

		return status;
	}
//}

#endif
