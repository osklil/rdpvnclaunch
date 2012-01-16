rdpvnclaunch
============

Introduction
------------

rdpvnclaunch consists of two separate programs - rdplaunch and vnclaunch -
that make it possible to start RDP and VNC sessions without having to type
the password. They also provide transparent SOCKS4 proxying support for
these applications by implementing a proxy client. Connection details are
specified using template files as well as command line options.

rdpvnclaunch is written in C and compiles with MinGW.

Author and Feedback
-------------------

rdpvnclaunch is written by Oskar Liljeblad <oskar@osk.mine.nu>.

This software is a work in progress and there are probably many ways it can
still be improved. If you'd like to contribute patches, ideas, or bug
reports, please send me an email!

Copyright and License
---------------------

rdpvnclaunch is copyright (C) 2012 Oskar Liljeblad.

  This program is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation, either version 3 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.

rdpvnclaunch may contain code from Gnulib
<http://www.gnu.org/software/gnulib/>. Such code is licensed under the
terms of the GNU Lesser General Public License.

Gnulib is opyright (C) Free Software Foundation, Inc.

  This program is free software: you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

rdpvnclaunch contains a 3DES implementation by Richard
Outerbridge. This code is in the public domain:

  D3DES (V5.09)

  A portable, public domain, version of the Data Encryption Standard.

  Written with Symantec's THINK (Lightspeed) C by Richard Outerbridge.
  Thanks to: Dan Hoey for his excellent Initial and Inverse permutation
  code;  Jim Gillogly & Phil Karn for the DES key schedule code; Dennis
  Ferguson, Eric Young and Dana How for comparing notes; and Ray Lau,
  for humouring me on.
  
  Copyright (c) 1988,1989,1990,1991,1992 by Richard Outerbridge.
  (GEnie : OUTER; CIS : [71755,204]) Graven Imagery, 1992.

Download
--------

The latest code for rdpvnclaunch can be downloaded from GitHub:

 * <https://github.com/osklil/rdpvnclaunch/zipball/master>

Requirements
------------

Build requirements:

 * MinGW
   http://mingw.org/

   * gcc from GNU Compiler Collection

   * GNU make

Runtime requirements:

 * Microsoft Remote Desktop Connection (mstsc.exe)
   Included with Windows XP and later by default.

 * VNC Viewer (vncviewer.exe)
   TightVNC is recommended.
   http://www.tightvnc.org/

Build Instructions
------------------

Run 'mingw32-make' from the source code directory.

Usage
-----

FIXME

Future
------

Please see the TODO file.

References
----------

  * Remote Desktop Protocol settings in Windows Server 2003 and in Windows XP.
    <http://support.microsoft.com/kb/885187>

  * RDP Settings for Remote Desktop Services in Windows Server 2008 R2.
    <http://technet.microsoft.com/en-us/library/ff393699(WS.10).aspx>

  * GNU portability library (Gnulib).
    <http://www.gnu.org/software/gnulib/>
