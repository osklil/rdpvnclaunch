/* vnclaunch.c - Main functions for vnclaunch
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
#include <wincrypt.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "rdpvnclaunch.h"
#include "d3des.h"

#define DEFAULT_TMPFILE_TEMPLATE L"vnclaunch-XXXXXX.vnc"
#define DEFAULT_VNC_TEMPLATE_FILE  L"template.vnc"
#define DEFAULT_PORT_STR L"5900"
#define VNC_MAX_PASSWORD_LEN 8
#define VNC_CONFIG_REGISTRY_KEY "VncViewer.Config\\shell\\open\\command"

const char *program_name = "vnclaunch";
const wchar_t *program_name_w = L"vnclaunch";

#define XDIGIT_LCHAR(x) ((x) <= 9 ? '0'+(x) : 'a'+(x)-10)

static wchar_t *get_default_vncviewer_command(void)
{
	HKEY key;
	DWORD error;
    DWORD len;
    DWORD type;
    wchar_t *data;
    wchar_t *place;

	error = RegOpenKeyEx(HKEY_CLASSES_ROOT, VNC_CONFIG_REGISTRY_KEY, 0, KEY_QUERY_VALUE, &key);
	if (error != ERROR_SUCCESS)
		die("Cannot open registry key `HKEY_CLASSES_ROOT\\%s': %s", VNC_CONFIG_REGISTRY_KEY, system_errstr_error(error));
    error = RegQueryValueExW(key, NULL, NULL, &type, NULL, &len);
	if (error != ERROR_SUCCESS)
		die("Cannot read registry value `HKEY_CLASSES_ROOT\\%s': %s", VNC_CONFIG_REGISTRY_KEY, system_errstr_error(error));
    if (type != REG_SZ)
		die("Invalid type of registry value `HKEY_CLASSES_ROOT\\%s', expected REG_SZ", VNC_CONFIG_REGISTRY_KEY);
    data = xmalloc(len + sizeof(wchar_t) * 8); // Replace %1 with @TMPFILE@, 1 extra for terminating null
    error = RegQueryValueExW(key, NULL, NULL, NULL, (BYTE *) data, &len);
	if (error != ERROR_SUCCESS)
		die("Cannot read registry value `HKEY_CLASSES_ROOT\\%s': %s", VNC_CONFIG_REGISTRY_KEY, system_errstr_error(error));
    data[len/sizeof(wchar_t)] = '\0';

    place = wcsstr(data, L"%1");
    if (place == NULL)
        die("Missing `%%1' from value in registry `HKEY_CLASSES_ROOT\\%s'", VNC_CONFIG_REGISTRY_KEY);
    memmove(place+9, place+2, (wcslen(place+2) + 1) * sizeof(wchar_t));
    memcpy(place, L"@TMPFILE@", 9 * sizeof(wchar_t));
    
    return data;
}

static wchar_t *encrypt_password_for_vnc_connection(const wchar_t *plain_passwd)
{
    static unsigned char vnc_fixed_key[8] = { 23, 82, 107, 6, 35, 78, 88, 7 };
    unsigned char enc_passwd[VNC_MAX_PASSWORD_LEN+1];
	wchar_t *hash;
    
    /* Discard everything but the first 8 (VNC_MAX_PASSWORD_LEN) characters. */
    /* FIXME does VNC support passwords longer than 8 characters? */
    memset(enc_passwd, 0, VNC_MAX_PASSWORD_LEN);
    wcstombs((char *) enc_passwd, plain_passwd, VNC_MAX_PASSWORD_LEN);
    enc_passwd[VNC_MAX_PASSWORD_LEN] = '\0';
    deskey(vnc_fixed_key, EN0);
    des(enc_passwd, enc_passwd);

    hash = xmalloc(sizeof(wchar_t) * (VNC_MAX_PASSWORD_LEN * 2 + 1));
    for (size_t c = 0; c < VNC_MAX_PASSWORD_LEN; c++) {
        hash[c*2 + 0] = XDIGIT_LCHAR(enc_passwd[c] >> 4);
        hash[c*2 + 1] = XDIGIT_LCHAR(enc_passwd[c] & 0xF);
    }
    hash[VNC_MAX_PASSWORD_LEN*2] = '\0';
    
	return hash;
}

int WINAPI WinMain (HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int cmdShow)
{
    	wchar_t *proxy_host = NULL;
        wchar_t *proxy_port;
        wchar_t *template_file;
	wchar_t *search_replace[] = {
		L"PASSWORD", NULL,
		L"HOSTNAME", NULL,
		L"PORT", NULL,
		L"TMPFILE", NULL,
		NULL
	};

	srand(time(NULL));
	template_file = xwcsdup(DEFAULT_VNC_TEMPLATE_FILE);
	proxy_port = xwcsdup(DEFAULT_PROXY_PORT);

	int argc;
	wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argv == NULL)
		die("Cannot parse command line arguments: %s", system_errstr());
	if (argc > 1) {
		for (int c = 1; c < argc; c++) {
			if (argv[c][0] == '-') {
				switch (argv[c][1]) {
				case 'h':
					if (c+1 >= argc)
						die("Missing required parameter for option -%c.", argv[c][1]);
					set_replacement(search_replace, L"HOSTNAME", xwcsdup(argv[++c]));
					break;
				case 'p':
					if (c+1 >= argc)
						die("Missing required parameter for option -%c.", argv[c][1]);
					set_replacement(search_replace, L"PASSWORD", xwcsdup(argv[++c]));
					break;
				case 'P':
					if (c+1 >= argc)
						die("Missing required parameter for option -%c.", argv[c][1]);
					set_replacement(search_replace, L"PORT", xwcsdup(argv[++c]));
					break;
				case 'T':
					if (c+1 >= argc)
						die("Missing required parameter for option -%c.", argv[c][1]);
					free(template_file);
					template_file = xwcsdup(argv[++c]);
					break;
                case 's':
                    if (c+1 >= argc)
                                                die("Missing required parameter for option -%c.", argv[c][1]);
                    free(proxy_host);
                    proxy_host = xwcsdup(argv[++c]);
                    break;
                case 'S':
                    if (c+1 >= argc)
                                                die("Missing required parameter for option -%c.", argv[c][1]);
                    free(proxy_port);
                    proxy_port = xwcsdup(argv[++c]);
                    break;
                case 'H':
                    inform(
                            "Usage: %s [OPTION]...\n"
                            "Start VNC sessions using vncviewer with password specified on command line.\n"
                            "\n"
                            "Options:\n"
                            "  -h HOST\n"
                            "    Name or address of host to connect to. Required.\n"
                            "  -p PASSWORD\n"
                            "    Password to log in automatically with.\n"
                            "  -P PORT\n"
                            "    Port number to connect to. Default is %ls.\n"
                            "  -T FILE\n"
                            "    Path of an alternate template file. Default is %ls.\n"
                            "  -s HOST\n"
                            "    Name or address of a SOCKS4 proxy to connect through.\n"
                            "  -S PORT\n"
                            "    Port number of SOCKS4 proxy. Default is %ls.\n"
                            "\n"
                            "Report bugs to <%ls>.\n",
                            program_name, DEFAULT_PORT_STR, DEFAULT_VNC_TEMPLATE_FILE, DEFAULT_PROXY_PORT, PACKAGE_BUGREPORT);
                    exit(0);
				default:
					die("Invalid options -%c", argv[c][1]);
				}
			}
		}
	}
	LocalFree(argv);

	wchar_t *hostname = get_replacement(search_replace, L"HOSTNAME");
	if (hostname == NULL)
		die("Missing hostname.");
	wchar_t *port = get_replacement(search_replace, L"PORT");
	if (port == NULL)
		port = set_replacement(search_replace, L"PORT", xwcsdup(DEFAULT_PORT_STR));

	wchar_t *password = get_replacement(search_replace, L"PASSWORD");
	if (password == NULL)
		die("Missing password.");

	set_replacement(search_replace, L"PASSWORD", encrypt_password_for_vnc_connection(password));

    if (proxy_host != NULL) {
        int listen_port;

        listen_port = prepare_proxy(proxy_host, proxy_port, hostname, get_replacement(search_replace, L"PORT"));
        hostname = set_replacement(search_replace, L"HOSTNAME", xwcsdup(L"127.0.0.1"));
        port = set_replacement(search_replace, L"PORT", xaswprintf(L"%d", listen_port));
    }

	wchar_t *command = NULL;
	wchar_t *tmpfile_template = xwcsdup(DEFAULT_TMPFILE_TEMPLATE);

	FILE *in_fh;
	wcsbuf_t *inbuf = wcsbuf_new();
	wcsbuf_t *outbuf = wcsbuf_new();
	if ((in_fh = _wfopen(template_file, L"r, ccs=UNICODE")) == NULL)
		die("Cannot open file `%ls' for reading: %s", template_file, errno_errstr());
	while ((inbuf->len = wgetline(&inbuf->data, &inbuf->size, in_fh)) >= 0) {
		if (inbuf->data[0] != '#' && inbuf->data[0] != '\n' && !(inbuf->data[0] == '\r' && inbuf->data[1] == '\n')) {
			if (wcsncmp(inbuf->data, L"tmpfile_template=", 17) == 0) {
				chomp_string(inbuf->data + 17);
				free(tmpfile_template);
				tmpfile_template = xwcsdup(inbuf->data + 17);
			} else if (wcsncmp(inbuf->data, L"command_line=", 13) == 0) {
				chomp_string(inbuf->data+13);
				free(command); /* command may be NULL */
				command = xwcsdup(inbuf->data+13);
			} else {
				expand_line(inbuf, search_replace);
				wcsbuf_append_wcsbuf(outbuf, inbuf);
			}
		}
	}
	if (ferror(in_fh))
		die("Cannot read from file `%ls': %s", template_file, errno_errstr());
	fclose(in_fh);
	free(template_file);

	FILE *out_fh;
	wchar_t *tmpfile = get_temp_file_expanded(tmpfile_template, search_replace);
	set_replacement(search_replace, L"TMPFILE", tmpfile);
	if ((out_fh = _wfopen(tmpfile, L"wT, ccs=UNICODE")) == NULL)
		die("Cannot open file `%ls' for writing: %s", tmpfile, errno_errstr());
	if (fwrite(outbuf->data, outbuf->len * sizeof(wchar_t), 1, out_fh) < 0)
		die("Cannot write to file `%ls': %s", tmpfile, errno_errstr());
	if (fclose(out_fh) < 0)
		die("Cannot close file `%ls': %s", tmpfile, errno_errstr());
	wcsbuf_free(outbuf);

	if (command == NULL)
		command = get_default_vncviewer_command();
	wcsbuf_set_wcs(inbuf, command);
	free(command);
	expand_line(inbuf, search_replace);

	STARTUPINFOW startupinfo;
	PROCESS_INFORMATION procinfo;
	memset(&startupinfo, 0, sizeof(startupinfo));
	startupinfo.cb = sizeof(startupinfo);
	if (!CreateProcessW(NULL, inbuf->data, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &startupinfo, &procinfo))
		die("Cannot start application: %s", system_errstr());

	if (proxy_host != NULL)
            handle_proxy();

	/* It seems waiting on 64-bit windows doesn't quite work as expected.
	 * WaitForSingleObject returns immediately. See
	 * <http://social.msdn.microsoft.com/Forums/en/wpf/thread/22c10140-a502-4aa1-98d3-3607b8b573e8>.
	 * Of the two suggested solutions, only one works:
	 *  - use 64-bit version of rdplaunch (works)
	 *  - prepend C:\WINDOWS\system32\ to mstsc.exe command line
	 *    (does not work)
	 * A workaround is to sleep some amount of time before removing
	 * the temporary file.
	 */
	if (WaitForSingleObject(procinfo.hProcess, INFINITE) == WAIT_FAILED)
		die("Cannot wait for application to finish: %s", system_errstr());
	if (is_running_in_wow64())
		Sleep(10000);

	if (!DeleteFileW(tmpfile))
		die("Cannot delete temporary file `%ls': %s", tmpfile, system_errstr());

	for (int c = 0; search_replace[c] != NULL; c += 2)
		free(search_replace[c+1]);
	wcsbuf_free(inbuf);
	return 0;
}
