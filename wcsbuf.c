/* wcsbuf.c - Wide string buffer
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

#include <string.h>
#include <stdio.h>
#include "rdpvnclaunch.h"

/* wcsbuf_assure:
 * Assure that line will hold at least assurelen characters.
 * assurelen need not include the terminating null character.
 */
void
wcsbuf_assure(wcsbuf_t *buf, size_t assurelen)
{
	if (assurelen >= buf->size) {
		buf->size = assurelen + 1;
		buf->data = xrealloc(buf->data, buf->size * sizeof(wchar_t));
	}
}

void
wcsbuf_free(wcsbuf_t *buf)
{
	free(buf->data);
	free(buf);
}

wchar_t *
wcsbuf_free_to_wcs(wcsbuf_t *buf)
{
	wchar_t *data = buf->data;
	free(buf);
	return data;
}

void
wcsbuf_set_wcs(wcsbuf_t *buf, wchar_t *source)
{
	buf->len = wcslen(source);
	wcsbuf_assure(buf, buf->len);
	memcpy(buf->data, source, (buf->len+1) * sizeof(wchar_t));
}

wcsbuf_t *
wcsbuf_new(void)
{
	wcsbuf_t *buf = xmalloc(sizeof(wcsbuf_t));
	buf->data = NULL;
	buf->size = 0;
	buf->len = 0;
	return buf;
}

wchar_t *
xwcsdup(const wchar_t *str)
{
	wchar_t *out = wcsdup(str);
	if (out == NULL)
		xalloc_die();
	return out;
}

void
wcsbuf_append_wcsbuf(wcsbuf_t *buf, wcsbuf_t *sourcebuf)
{
	wcsbuf_assure(buf, buf->len + sourcebuf->len);
	memcpy(buf->data + buf->len, sourcebuf->data, (sourcebuf->len + 1) * sizeof(wchar_t));
	buf->len += sourcebuf->len;
}

void
wcsbuf_append_wchar(wcsbuf_t *buf, wchar_t chr)
{
	wcsbuf_assure(buf, buf->len + 1);
	buf->data[buf->len++] = chr;
	buf->data[buf->len] = '\0';
}

void
wcsbuf_append_wcs(wcsbuf_t *buf, const wchar_t *str)
{
	size_t len = wcslen(str);
	wcsbuf_assure(buf, buf->len + len);
	memcpy(buf->data + buf->len, str, (len + 1) * sizeof(wchar_t));
	buf->len += len;
}

wcsbuf_t *
wcsbuf_clone(wcsbuf_t *sourcebuf)
{
	wcsbuf_t *buf = wcsbuf_new();
	buf->data = xmalloc((sourcebuf->len + 1) * sizeof(wchar_t));
	memcpy(buf->data, sourcebuf->data, (sourcebuf->len + 1) * sizeof(wchar_t));
	return buf;
}

wchar_t *
wcsbuf_clone_buf(wcsbuf_t *buf)
{
	wchar_t *str;
	str = xmalloc((buf->len + 1) * sizeof(wchar_t));
	memcpy(str, buf->data, (buf->len + 1) * sizeof(wchar_t));
	return str;
}
