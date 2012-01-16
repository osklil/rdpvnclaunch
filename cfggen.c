/* cfggen.c - Functions related to configuration files
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

#include <wchar.h>
#include <string.h>
#include "rdpvnclaunch.h"

void expand_line(wcsbuf_t *buf, wchar_t **search_replace)
{
	wchar_t *p0 = buf->data;
	wchar_t *p1;

	while ((p0 = wcschr(p0, '@')) != NULL && (p1 = wcschr(p0+1, '@')) != NULL) {
		int c;

		for (c = 0; search_replace[c] != NULL; c += 2) {
			wchar_t *search = search_replace[c];
			if (p1 - p0 - 1 == wcslen(search) && wcsncmp(p0+1, search, p1-p0-1) == 0) {
				wchar_t *replacement = search_replace[c+1];
				size_t newlen = wcslen(replacement);
				size_t oldlen = p1 - p0 + 1;

				if (buf->len + newlen - oldlen >= buf->size) {
					wchar_t *newline;
					buf->size = buf->len + newlen - oldlen + 1;
					newline = xrealloc(buf->data, sizeof(wchar_t) * buf->size);
					p0 = newline + (p0 - buf->data);
					p1 = newline + (p1 - buf->data);
					buf->data = newline;
				}
				memmove(p1 + newlen - oldlen + 1, p1 + 1, sizeof(wchar_t) * (buf->len - (p1 + 1 - buf->data) + 1));
				memcpy(p0, replacement, sizeof(wchar_t) * newlen);
				buf->len += newlen - oldlen;
				p1 += newlen - oldlen;
				break;
			}
		}
		p0 = p1 + 1;
	}
}

/* XXX race condition? need to create file instead of returning its name */
wchar_t *get_temp_file_expanded(const wchar_t *template, wchar_t **search_replace)
{
	wcsbuf_t *path;
	wchar_t *unique;
	size_t baselen;

	path = wcsbuf_new();
	while ((path->len = GetTempPathW(path->size, path->data)) > path->size) {
		/* Not sure if "+ 1" is necessary according to API docs, but it doesn't hurt. */
		/* + 1 is implicit with wcsbuf */
		wcsbuf_assure(path, path->len);
	}
	if (path->len == 0)
		die("Cannot get temporary file path: %s", system_errstr());

	/* Assure there is space for possible backslash and template. */
	wcsbuf_assure(path, path->len + 1 + wcslen(template));
	if (path->data[path->len-1] != L'\\')
		wcsbuf_append_wchar(path, L'\\');
	baselen = path->len;
	wcsbuf_append_wcs(path, template);

	/* XXX: allow fewer XXXs, perhaps just one? */
	unique = wcsstr(template, L"XXXXXX");
	if (unique != NULL) {
		for (int try = 10; try > 0; try--) {
			wchar_t tmp[7];
			wchar_t *orig_path;

			swprintf(tmp, L"%06d", rand() % 1000000);
			memcpy(path->data + baselen + (unique-template), tmp, sizeof(wchar_t) * 6);
			orig_path = wcsbuf_clone_buf(path);
			expand_line(path, search_replace);
			for (int c = baselen; c < path->len; c++) {
				if (path->data[c] <= 31 || wcschr(L"/\\:*?\"<>|", path->data[c]) != NULL)
					path->data[c] = '_';
			}
			if (GetFileAttributesW(path->data) == INVALID_FILE_ATTRIBUTES) {
				free(orig_path);
				return wcsbuf_free_to_wcs(path);
			}
			wcsbuf_set_wcs(path, orig_path);
			free(orig_path);
		}
		die("No free temporary file name found.");
	}

	expand_line(path, search_replace);
	return wcsbuf_free_to_wcs(path);
}

wchar_t *get_replacement(wchar_t **search_replace, const wchar_t *key)
{
	for (int c = 0; search_replace[c] != NULL; c += 2) {
		if (wcscmp(search_replace[c], key) == 0)
			return search_replace[c+1];
	}

	return NULL;
}

wchar_t *set_replacement(wchar_t **search_replace, const wchar_t *key, wchar_t *value)
{
	for (int c = 0; search_replace[c] != NULL; c += 2) {
		if (wcscmp(search_replace[c], key) == 0) {
			if (search_replace[c+1] != NULL)
				free(search_replace[c+1]);
			search_replace[c+1] = value;
			return value;
		}
	}

    return NULL;
}

void chomp_string(wchar_t *str)
{
	size_t len = wcslen(str);

	if (len >= 2 && str[len-2] == '\r' && str[len-1] == '\n') {
		str[len-2] = '\0';
	} else if (len >= 1 && str[len-1] == '\n') {
		str[len-1] = '\0';
	}
}
