# Makefile for rdpvnclaunch
#
# Copyright (C) 2012 Oskar Liljeblad
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#

CC32=gcc
CC64=x86_64-w64-mingw32-gcc
CC=$(CC32)
EXT=.exe
CFLAGS=-std=gnu99 -Wall
LDFLAGS=-Wl,-subsystem,windows

all: rdplaunch$(EXT) vnclaunch$(EXT)

clean:
	del *.o rdplaunch$(EXT) vnclaunch$(EXT)

rdplaunch$(EXT): xvaswprintf.o xvasprintf.o wgetdelim.o xmalloc.o werror.o error.o wcsbuf.o cfggen.o wow64.o proxy.o rdplaunch.o
	$(CC) $(LDFLAGS) $(CFLAGS) -I. -o $@ $^ -lcrypt32 -ladvapi32 -lws2_32

vnclaunch$(EXT): xvaswprintf.o xvasprintf.o wgetdelim.o xmalloc.o werror.o error.o wcsbuf.o cfggen.o wow64.o proxy.o d3des.o vnclaunch.o
	$(CC) $(LDFLAGS) $(CFLAGS) -I. -o $@ $^ -lcrypt32 -ladvapi32 -lws2_32

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
