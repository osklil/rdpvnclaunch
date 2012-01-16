/* xvasprintf.c - Safe vasprintf
 *
 * Copyright (C) 2012 Oskar Liljeblad
 *
 * This file may contain code from Gnulib modules vasprintf and xvasprintf. 
 * Please see the Gnulib source code for copyright and license information.
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

#include <wchar.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "rdpvnclaunch.h"

int
vasprintf (char **sptr, const char *fmt, va_list argv)
{
    int wanted = vsnprintf(*sptr = NULL, 0, fmt, argv);
    if (wanted < 0 || (*sptr = malloc(1 + wanted)) == NULL)
        return -1;
    return vsprintf(*sptr, fmt, argv);
}

char *
xvasprintf (const char *format, va_list args)
{
  char *result;

  if (vasprintf (&result, format, args) < 0)
    {
      if (errno == ENOMEM)
        xalloc_die ();
      return NULL;
    }

  return result;
}

char *
xasprintf (const char *format, ...)
{
  va_list args;
  char *result;

  va_start (args, format);
  result = xvasprintf (format, args);
  va_end (args);

  return result;
}

int
asprintf (char **sptr, const char *fmt, ...)
{
    int retval;
    va_list argv;
    va_start(argv, fmt);
    retval = vasprintf(sptr, fmt, argv);
    va_end(argv);
    return retval;
}
