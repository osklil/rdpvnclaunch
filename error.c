/* error.c - Error reporting
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

void vwarn (char *fmt, va_list argv)
{
	char *msg;

    if (vasprintf(&msg, fmt, argv) < 0) {
        MessageBox(NULL, "Cannot allocate memory during error message generation.", program_name, MB_OK|MB_ICONSTOP);
		return;
    }
	MessageBox(NULL, msg, program_name, MB_OK|MB_ICONSTOP);
	free(msg);
}

void warn (char *fmt, ...)
{
    va_list argv;

    va_start(argv, fmt);
	vwarn(fmt, argv);
    va_end(argv);
}

void die (char *fmt, ...)
{
    va_list argv;

    va_start(argv, fmt);
	vwarn(fmt, argv);
    va_end(argv);
    exit(1);
}

void xalloc_die (void)
{
	die("Cannot allocate memory.");
}

char *errno_errstr (void)
{
    return strerror(errno);
}

char *system_errstr_error(DWORD error)
{
    char *sysmsg;
    char *msg;

    if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, (LPTSTR) &sysmsg, 0, NULL))
        die("Cannot get system error when generating error message");
    msg = strdup(sysmsg);
    if (msg == NULL)
        die("Cannot allocate memory when generating error message");
    LocalFree(sysmsg);
    return msg;
}

char *system_errstr (void)
{
    return system_errstr_error(GetLastError());
}
