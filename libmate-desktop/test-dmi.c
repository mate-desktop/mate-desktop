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
#define MATE_DESKTOP_USE_UNSTABLE_API
#include "mate-dmi.h"

int main(int argc, char **argv)
{
    MateDmi *dmi;
    dmi = mate_dmi_new ();
    printf("dmi name: %s\n", mate_dmi_get_name (dmi));
    printf("dmi version: %s\n", mate_dmi_get_version (dmi));
    printf("dmi vendor: %s\n", mate_dmi_get_vendor (dmi));
    return 0;
}
