/* wgetdelim.c - Wide character implementation of getdelim
 *
 * Copyright (C) 2012 Oskar Liljeblad
 *
 * This file may contain code from Gnulib module getdelim. Please see the
 * Gnulib source code for copyright and license information.
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
#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include "rdpvnclaunch.h"

/* XXX: this seems to be necessary with 64 bit MinGW */
#ifndef SSIZE_MAX
#define SSIZE_MAX LONG_MAX
#endif

ssize_t
wgetdelim (wchar_t **lineptr, size_t *n, int delimiter, FILE *fp)
{
  ssize_t result;
  size_t cur_len = 0;

  if (lineptr == NULL || n == NULL || fp == NULL)
    {
      errno = EINVAL;
      return -1;
    }

  if (*lineptr == NULL || *n == 0)
    {
      wchar_t *new_lineptr;
      *n = 120;
      new_lineptr = (wchar_t *) realloc (*lineptr, *n * sizeof(wchar_t));
      if (new_lineptr == NULL)
        return -1;
      *lineptr = new_lineptr;
    }

  for (;;)
    {
      wint_t i;

      i = getwc (fp);
      if (i == WEOF)
        {
          result = -1;
          break;
        }

      /* Make enough space for len+1 (for final NUL) bytes.  */
      if (cur_len + 1 >= *n)
        {
          size_t needed_max =
            SSIZE_MAX < SIZE_MAX ? (size_t) SSIZE_MAX + 1 : SIZE_MAX;
          size_t needed = 2 * *n + 1;   /* Be generous. */
          wchar_t *new_lineptr;

          if (needed_max < needed)
            needed = needed_max;
          if (cur_len + 1 >= needed)
            {
              errno = EOVERFLOW;
              return -1;
            }

          new_lineptr = (wchar_t *) realloc (*lineptr, needed * sizeof(wchar_t));
          if (new_lineptr == NULL)
			return -1;

          *lineptr = new_lineptr;
          *n = needed;
        }

      (*lineptr)[cur_len] = i;
      cur_len++;

      if (i == delimiter)
        break;
    }
  (*lineptr)[cur_len] = '\0';
  return cur_len ? cur_len : result;
}

ssize_t
wgetline (wchar_t **lineptr, size_t *n, FILE *stream)
{
  return wgetdelim (lineptr, n, '\n', stream);
}
