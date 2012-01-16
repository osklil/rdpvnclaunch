/* xwvasprintf.c - Safe wide character vasprintf
 *
 * Copyright (C) 2012 Oskar Liljeblad
 *
 * This file may contain code from Gnulib modules vasprintf and xvasprintf.
 * Please see the Gnulib source code for copyright and license qinformation.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "rdpvnclaunch.h"

int
vaswprintf (wchar_t **sptr, const wchar_t *fmt, va_list argv)
{
    int wanted = sizeof(wchar_t) * vsnwprintf(*sptr = NULL, 0, fmt, argv);
    if (wanted < 0 || (*sptr = malloc(1 + wanted)) == NULL)
        return -1;
    return vswprintf(*sptr, fmt, argv);
}

wchar_t *
xvaswprintf (const wchar_t *format, va_list args)
{
  wchar_t *result;

  if (vaswprintf (&result, format, args) < 0)
    {
      if (errno == ENOMEM)
        xalloc_die ();
      return NULL;
    }

  return result;
}

wchar_t *
xaswprintf (const wchar_t *format, ...)
{
  va_list args;
  wchar_t *result;

  va_start (args, format);
  result = xvaswprintf (format, args);
  va_end (args);

  return result;
}

int
aswprintf (wchar_t **sptr, const wchar_t *fmt, ...)
{
    int retval;
    va_list argv;
    va_start(argv, fmt);
    retval = vaswprintf(sptr, fmt, argv);
    va_end(argv);
    return retval;
}
