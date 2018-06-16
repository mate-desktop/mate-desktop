/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright 2008 Red Hat, Inc.
 * Copyright 2007 William Jon McCann <mccann@jhu.edu>
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Written by: Ray Strode
 *             William Jon McCann
 */

#ifndef __MATE_LANGUAGES_H
#define __MATE_LANGUAGES_H

#ifndef MATE_DESKTOP_USE_UNSTABLE_API
#error    This is unstable API. You must define MATE_DESKTOP_USE_UNSTABLE_API before including mate-languages.h
#endif

G_BEGIN_DECLS

char *        mate_get_language_from_locale    (const char *locale,
                                                 const char *translation);
char *        mate_get_country_from_locale     (const char *locale,
                                                 const char *translation);
char **       mate_get_all_locales             (void);
gboolean      mate_parse_locale                (const char *locale,
                                                 char      **language_codep,
                                                 char      **country_codep,
                                                 char      **codesetp,
                                                 char      **modifierp);
char *        mate_normalize_locale            (const char *locale);
gboolean      mate_language_has_translations   (const char *code);
char *        mate_get_language_from_code      (const char *code,
                                                 const char *translation);
char *        mate_get_country_from_code       (const char *code,
                                                 const char *translation);

G_END_DECLS

#endif /* __MATE_LANGUAGES_H */
