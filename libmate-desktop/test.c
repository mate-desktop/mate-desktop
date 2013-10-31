/*
 * test.c: general tests for libmate-desktop
 *
 * Copyright (C) 2013 Stefano Karapetsas
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Authors:
 *  Stefano Karapetsas <stefano@karapetsas.com>
 */

#include <config.h>
#include "mate-desktop.h"

int
main (int argc, char **argv)
{
#if MATE_DESKTOP_CHECK_VERSION (1, 7, 2)
    return 0;
#else
    g_warning ("Old mate-desktop version!");
    return 1;
#endif
}
