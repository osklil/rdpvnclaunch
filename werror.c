/* werror.c - Error reporting (wide characters)
 *
 * Copyright (C) 2012 Oskar Liljeblad
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <windows.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "rdpvnclaunch.h"

void vwwarn (wchar_t *fmt, va_list argv)
{
	wchar_t *msg;

    if (vaswprintf(&msg, fmt, argv) < 0) {
        MessageBoxW(NULL, L"Cannot allocate memory during error message generation.", program_name_w, MB_OK|MB_ICONSTOP);
		return;
    }
	MessageBoxW(NULL, msg, program_name_w, MB_OK|MB_ICONSTOP);
	free(msg);
}

void wwarn (wchar_t *fmt, ...)
{
    va_list argv;

    va_start(argv, fmt);
	vwwarn(fmt, argv);
    va_end(argv);
}

void wdie (wchar_t *fmt, ...)
{
    va_list argv;

    va_start(argv, fmt);
	vwwarn(fmt, argv);
    va_end(argv);
    exit(1);
}
