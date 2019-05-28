/* vi: set sw=4 ts=4 wrap ai: */
/*
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * */

#include <stdio.h>
#include <locale.h>
#define MATE_DESKTOP_USE_UNSTABLE_API
#include "mate-languages.h"

void test_one_locale(const gchar *locale);
void test_locales(void);

void test_one_locale(const gchar *locale)
{
    char *lang, *country, *norm_locale;
    char *language_code, *country_code, *codeset, *modifier;

    lang = mate_get_language_from_locale (locale, locale);
    country = mate_get_country_from_locale (locale, locale);
    norm_locale = mate_normalize_locale (locale);

    printf("Current locale: %s\n", locale);
    printf("[locale]:\t\tlang=%s, country=%s, locale=%s\n", lang, country, norm_locale);
    g_free(lang);
    g_free(country);
    g_free(norm_locale);

    if (mate_parse_locale (locale, &language_code, &country_code, &codeset, &modifier)) {
        lang = mate_get_language_from_code (language_code, locale);
        country = mate_get_country_from_code (country_code, locale);
        if (mate_language_has_translations(language_code)) {
            printf("[mate_parse_locale]:\tlang_code=%s, country_code=%s, code=%s, modifier=%s\n"
                    "[code]:\t\t\tlang=%s, country=%s, Has translation\n",
                    language_code, country_code, codeset, modifier,
                    lang, country);
        } else {
            printf("[mate_parse_locale]:\tlang_code=%s, country_code=%s, code=%s, modifier=%s\n"
                    "[code]:\t\t\tlang=%s, country=%s\n",
                    language_code, country_code, codeset, modifier,
                    lang, country);
        }
        g_free(lang);
        g_free(country);
    }
    putchar('\n');

    g_free(language_code);
    g_free(country_code);
    g_free(codeset);
    g_free(modifier);
}

void test_locales(void)
{
    char **all;
    guint i, len;

    all= mate_get_all_locales ();
    len = g_strv_length (all);

    for (i =0; i < len; i++) {
        test_one_locale(all[i]);
    }
    g_strfreev(all);
}

int main(int argc, char **argv)
{
    if (argc == 2) {
        test_one_locale(argv[1]);
    } else {
        test_locales();
    }
    return 0;
}
