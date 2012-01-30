/* rdpvnclaunch.h - Common definitions
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
#include <string.h>

#define EOVERFLOW 2006
#define NIBBLE_TO_UCHAR(x) ((x) < 10 ? '0'+(x) : 'A'+(x)-10)

#define DEFAULT_PROXY_PORT L"1080"

#define PACKAGE_BUGREPORT L"oskar@osk.mine.nu"

typedef struct {
    wchar_t *data;
    size_t size;
    ssize_t len;
} wcsbuf_t;

/* rdplaunch.c / vnclaunch.c */
extern const char *program_name;
extern const wchar_t *program_name_w;

/* proxy.c */
extern uint16_t prepare_proxy (const wchar_t *proxy_host, const wchar_t *port, const wchar_t *connect_host, const wchar_t *connect_port);
extern void handle_proxy (void);

/* cfggen.c */
extern void expand_line(wcsbuf_t *buf, wchar_t **search_replace);
extern wchar_t *set_replacement(wchar_t **search_replace, const wchar_t *key, wchar_t *value);
extern wchar_t *get_replacement(wchar_t **search_replace, const wchar_t *key);
extern wchar_t *get_temp_file_expanded(const wchar_t *template, wchar_t **search_replace);
extern void chomp_string(wchar_t *str);

/* wow64.c */
extern BOOL is_running_in_wow64(void);

/* wgetdelim.c */
extern ssize_t wgetdelim (wchar_t **lineptr, size_t *n, int delimiter, FILE *fp);
extern ssize_t wgetline (wchar_t **lineptr, size_t *n, FILE *stream);

/* xvasprintf.c */
extern char *xasprintf (const char *format, ...) __attribute__ ((__format__ (__printf__, 1, 2)));
extern char *xvasprintf (const char *format, va_list args) __attribute__ ((__format__ (__printf__, 1, 0)));
extern int vasprintf (char **sptr, const char *fmt, va_list argv) __attribute__ ((format (printf, 2, 0)));
extern int asprintf (char **sptr, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));

/* xvaswprintf.c */
extern wchar_t *xaswprintf (const wchar_t *format, ...);
extern wchar_t *xvaswprintf (const wchar_t *format, va_list args);
extern int vaswprintf (wchar_t **sptr, const wchar_t *fmt, va_list argv);
extern int aswprintf (wchar_t **sptr, const wchar_t *fmt, ...);

/* error.c */
extern void vwarn (char *fmt, va_list argv) __attribute__ ((format (printf, 1, 0)));
extern void warn (char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
extern void die (char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
extern void xalloc_die (void);
extern char *errno_errstr (void);
extern char *system_errstr_error(DWORD error);
extern char *system_errstr (void);
extern void inform (char *fmt, ...);

/* werror.c */
extern void vwwarn (wchar_t *fmt, va_list argv);
extern void wwarn (wchar_t *fmt, ...);
extern void wdie (wchar_t *fmt, ...);

/* xmalloc.c */
extern void *xmalloc (size_t size);
extern void *xrealloc (void *ptr, size_t size);

/* wcsbuf.c */
extern void wcsbuf_assure(wcsbuf_t *buf, size_t assurelen);
extern void wcsbuf_set_wcs(wcsbuf_t *buf, wchar_t *str);
extern wcsbuf_t *wcsbuf_new(void);
extern wchar_t *xwcsdup(const wchar_t *str);
extern wchar_t *wcsbuf_free_to_wcs(wcsbuf_t *buf);
extern void wcsbuf_free(wcsbuf_t *buf);
extern void wcsbuf_append_wcsbuf(wcsbuf_t *buf, wcsbuf_t *sourcebuf);
extern void wcsbuf_append_wchar(wcsbuf_t *buf, wchar_t chr);
extern void wcsbuf_append_wcs(wcsbuf_t *buf, const wchar_t *str);
extern wcsbuf_t *wcsbuf_clone(wcsbuf_t *sourcebuf);
extern wchar_t *wcsbuf_clone_buf(wcsbuf_t *buf);
