/* wow64.c - Utility functions for 64-bit support in Windows
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
#include "rdpvnclaunch.h"

/* FIXME do we need this? */
typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

BOOL is_running_in_wow64(void)
{
	LPFN_ISWOW64PROCESS isWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");
	BOOL isWow64 = FALSE;

	if (isWow64Process != NULL && !isWow64Process(GetCurrentProcess(), &isWow64))
		die("Cannot determine if running in 64-bit Windows: %s", system_errstr());

	return isWow64;
}
