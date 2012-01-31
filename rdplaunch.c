/* rdplaunch.c - Main functions for rdplaunch
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

#define DEFAULT_TMPFILE_TEMPLATE L"rdplaunch-XXXXXX.rdp"
#define DEFAULT_RDP_TEMPLATE_FILE  L"template.rdp"
#define DEFAULT_MSTSC_COMMAND  L"mstsc.exe \"@TMPFILE@\" /w:@WIDTH@ /h:@CLIENTHEIGHT@"
#define DEFAULT_PORT_STR L"3389"
#define MSTSC_LOCAL_DEVICES_REGISTRY_KEY "Software\\Microsoft\\Terminal Server Client\\LocalDevices"
#define MSTSC_SERVERS_REGISTRY_KEY_FMT L"Software\\Microsoft\\Terminal Server Client\\Servers\\%s"

const char *program_name = "rdplaunch";
const wchar_t *program_name_w = L"rdplaunch";
const char version_etc_copyright[] = "Copyright (C) 2012 Oskar Liljeblad";

static wchar_t *encrypt_password_for_rdp_connection(const wchar_t *password)
{
	wchar_t *hash;
	DATA_BLOB data_in;
	DATA_BLOB data_out;
	size_t c;

	data_in.cbData = wcslen(password) * sizeof(wchar_t);
	data_in.pbData = (BYTE *) password;
	if (!CryptProtectData(&data_in, L"psw", NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN|CRYPTPROTECT_LOCAL_MACHINE, &data_out))
		die("Cannot encrypt password: %s", system_errstr());

	hash = xmalloc((data_out.cbData*2 + 1) * sizeof(wchar_t));
	for (c = 0; c < data_out.cbData; c++) {
		hash[c*2] = NIBBLE_TO_UCHAR(data_out.pbData[c] >> 4);
		hash[c*2+1] = NIBBLE_TO_UCHAR(data_out.pbData[c] & 15);
	}
	hash[data_out.cbData*2] = '\0';

	LocalFree(data_out.pbData);
	return hash;
}

static void prepare_registry_for_rdp_connection (const wchar_t *hostname)
{
	wchar_t *key_name;
	HKEY key;
	DWORD error;
	DWORD value = 0x4c;
	BYTE bin_value[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	/* This gets rid of the first warning:
	 * "This remote connection could harm your local or remote computer."
	 */
	error = RegCreateKeyEx(HKEY_CURRENT_USER, MSTSC_LOCAL_DEVICES_REGISTRY_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, NULL);
	if (error != ERROR_SUCCESS)
		die("Cannot create registry key `HKEY_CURRENT_USER\\%s': %s", MSTSC_LOCAL_DEVICES_REGISTRY_KEY, system_errstr_error(error));
	error = RegSetValueExW(key, hostname, 0,  REG_DWORD, (BYTE *) &value, sizeof(value));
	if (error != ERROR_SUCCESS)
		die("Cannot set registry value `HKEY_CURRENT_USER\\%s\\%ls': %s", MSTSC_LOCAL_DEVICES_REGISTRY_KEY, hostname, system_errstr_error(error));
	RegCloseKey(key); /* Ignore errors */

	/* This is supposed to get rid of the second warning:
	 * "The remote computer could not be authenticated due to problems with its security certificate."
	 * However, setting CertHash to all zeroes will not work for Windows 2008 R2 unless
	 * "authentication level" in the .rdp file is 0.
	 */
	key_name = xaswprintf(MSTSC_SERVERS_REGISTRY_KEY_FMT, hostname);
	error = RegCreateKeyExW(HKEY_CURRENT_USER, key_name, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, NULL);
	if (error != ERROR_SUCCESS)
		die("Cannot create registry key `HKEY_CURRENT_USER\\%ls': %s", key_name, system_errstr_error(error));
	free(key_name);
	if (RegQueryValueEx(key, "CertHash", NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
		error = RegSetValueEx(key, "CertHash", 0, REG_BINARY, bin_value, sizeof(bin_value)/sizeof(*bin_value));
		if (error != ERROR_SUCCESS)
			die("Cannot set registry value: %s", system_errstr_error(error));
	}
}

int WINAPI WinMain (HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int cmdShow)
{
	BOOL admin_mode = FALSE;
	wchar_t *template_file;
    wchar_t *proxy_host = NULL;
    wchar_t *proxy_port;
	wchar_t *search_replace[] = {
		L"USERNAME", NULL,
		L"PASSWORD", NULL,
		L"HOSTNAME", NULL,
		L"PORT", NULL,
		L"WIDTH", NULL,
		L"INNERWIDTH", NULL,
		L"CLIENTHEIGHT", NULL,		/* Height of screen excluding task bar */
		L"INNERHEIGHT", NULL,		/* Height of screen excluding task bar and top and bottom window frames */
		L"TMPFILE", NULL,
		L"ADMINMODE", NULL,		/* "1" if admin_mode, otherwise "0" */
		L"TITLE", NULL,
		NULL
	};

	srand(time(NULL));
	template_file = xwcsdup(DEFAULT_RDP_TEMPLATE_FILE);
    proxy_port = xwcsdup(DEFAULT_PROXY_PORT);

	int argc;
	wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argv == NULL)
		die("Cannot parse command line arguments: %s", system_errstr());
	if (argc > 1) {
		for (int c = 1; c < argc; c++) {
			if (argv[c][0] == '-') {
				switch (argv[c][1]) {
				case 'a':
					admin_mode = TRUE;
					break;
				case 'h':
					if (c+1 >= argc)
						die("Missing required parameter for option -%c.", argv[c][1]);
					set_replacement(search_replace, L"HOSTNAME", xwcsdup(argv[++c]));
					break;
				case 'u':
					if (c+1 >= argc)
						die("Missing required parameter for option -%c.", argv[c][1]);
					set_replacement(search_replace, L"USERNAME", xwcsdup(argv[++c]));
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
				case 't':
					if (c+1 >= argc)
						die("Missing required parameter for option -%c.", argv[c][1]);
					set_replacement(search_replace, L"TITLE", xwcsdup(argv[++c]));
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
                            "Start RDP sessions using mstsc with credentials specified on command line.\n"
                            "\n"
                            "Options:\n"
                            "  -h HOST\n"
                            "    Name or address of host to connect to. Required.\n"
                            "  -u USERNAME\n"
                            "    Username to log in automatically with.\n"
                            "  -p PASSWORD\n"
                            "    Password to log in automatically with.\n"
                            "  -P PORT\n"
                            "    Port number to connect to. Default is %ls.\n"
                            "  -t TITLE\n"
                            "    Title of Remote Desktop window.\n"
                            "  -T FILE\n"
                            "    Path of an alternate template file. Default is %ls.\n"
                            "  -s HOST\n"
                            "    Name or address of a SOCKS4 proxy to connect through.\n"
                            "  -S PORT\n"
                            "    Port number of SOCKS4 proxy. Default is %ls.\n"
                            "  -a\n"
                            "    Connect to administrative (console) session.\n"
                            "  -H\n"
                            "    Display this help and exit.\n"
                            "  -V\n"
                            "    Display version information and exit.\n"
                            "\n"
                            "Report bugs to <%ls>.\n",
                            program_name, DEFAULT_PORT_STR, DEFAULT_RDP_TEMPLATE_FILE, DEFAULT_PROXY_PORT, PACKAGE_BUGREPORT);
                    exit(0);
                case 'V':
                    inform(
                        "%s (%ls) %ls\n"
                        "%s\n"
                        "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n"
                        "This is free software: you are free to change and redistribute it.\n"
                        "There is NO WARRANTY, to the extent permitted by law.\n\n"
                        "Written by Oskar Liljeblad.",
                            program_name, PACKAGE_NAME, PACKAGE_VERSION, version_etc_copyright);
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

	if (get_replacement(search_replace, L"USERNAME") == NULL)
		die("Missing username.");
	wchar_t *password = get_replacement(search_replace, L"PASSWORD");
	if (password == NULL)
		die("Missing password.");

	RECT workarea;
	if (!SystemParametersInfo(SPI_GETWORKAREA, 0, &workarea, 0))
		die("Cannot get screen size: %s", system_errstr());
	LONG width = workarea.right - workarea.left;
	set_replacement(search_replace, L"WIDTH", xaswprintf(L"%lu", width));

	LONG framewidth;
	if ((framewidth = GetSystemMetrics(SM_CXSIZEFRAME)) == 0)
		die("Cannot get window frame width: no error message provided");
	set_replacement(search_replace, L"INNERWIDTH", xaswprintf(L"%lu", width - framewidth*2));

	LONG height;
	if ((height = GetSystemMetrics(SM_CYFULLSCREEN)) == 0)
		die("Cannot get screen height: no error message provided");
	set_replacement(search_replace, L"CLIENTHEIGHT", xaswprintf(L"%lu", height));
	LONG frameheight;
	if ((frameheight = GetSystemMetrics(SM_CYSIZEFRAME)) == 0)
		die("Cannot get window frame height: no error message provided");
	LONG captionheight;
	if ((captionheight = GetSystemMetrics(SM_CYCAPTION)) == 0)
		die("Cannot get window caption height: no error message provided");
	set_replacement(search_replace, L"INNERHEIGHT", xaswprintf(L"%lu", height - frameheight - captionheight));

	set_replacement(search_replace, L"ADMINMODE", xwcsdup(admin_mode ? L"1" : L"0"));

	if (get_replacement(search_replace, L"TITLE") == NULL)
		set_replacement(search_replace, L"TITLE", xwcsdup(hostname));

    if (proxy_host != NULL) {
        int listen_port;

        listen_port = prepare_proxy(proxy_host, proxy_port, hostname, get_replacement(search_replace, L"PORT"));
        hostname = set_replacement(search_replace, L"HOSTNAME", xwcsdup(L"127.0.0.1"));
        port = set_replacement(search_replace, L"PORT", xaswprintf(L"%d", listen_port));
    }
    prepare_registry_for_rdp_connection(hostname);
	set_replacement(search_replace, L"PASSWORD", encrypt_password_for_rdp_connection(password));

	wchar_t *command = xwcsdup(DEFAULT_MSTSC_COMMAND);
	wchar_t *tmpfile_template = xwcsdup(DEFAULT_TMPFILE_TEMPLATE);

	FILE *in_fh;
	wcsbuf_t *inbuf = wcsbuf_new();
	wcsbuf_t *outbuf = wcsbuf_new();
	if ((in_fh = _wfopen(template_file, L"r, ccs=UNICODE")) == NULL)
		die("Cannot open file `%ls' for reading: %s", template_file, errno_errstr());
	while ((inbuf->len = wgetline(&inbuf->data, &inbuf->size, in_fh)) >= 0) {
		if (inbuf->data[0] != '#' && inbuf->data[0] != '\n' && !(inbuf->data[0] == '\r' && inbuf->data[1] == '\n')) {
			if (wcsncmp(inbuf->data, L"tmpfile template:s:", 19) == 0) {
				chomp_string(inbuf->data + 19);
				free(tmpfile_template);
				tmpfile_template = xwcsdup(inbuf->data + 19);
			} else if (wcsncmp(inbuf->data, L"command line:s:", 15) == 0) {
				chomp_string(inbuf->data+15);
				free(command);
				command = xwcsdup(inbuf->data+15);
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
	if ((out_fh = _wfopen(tmpfile, L"wT, ccs=UTF-16LE")) == NULL)
		die("Cannot open file `%ls' for writing: %s", tmpfile, errno_errstr());
	if (fwrite(outbuf->data, outbuf->len * sizeof(wchar_t), 1, out_fh) < 0)
		die("Cannot write to file `%ls': %s", tmpfile, errno_errstr());
	if (fclose(out_fh) < 0)
		die("Cannot close file `%ls': %s", tmpfile, errno_errstr());
	wcsbuf_free(outbuf);

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
