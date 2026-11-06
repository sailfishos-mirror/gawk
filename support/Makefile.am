#
# Makefile.am --- automake input file for gawk
#
# Copyright (C) 2000-2016, 2018, 2019, 2021, 2022, 2023, 2024, 2026
# the Free Software Foundation, Inc.
#
# This file is part of GAWK, the GNU implementation of the
# AWK Programming Language.
#
# GAWK is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# GAWK is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

## process this file with automake to produce Makefile.in

AM_CFLAGS  = @CFLAGS@
AM_LDFLAGS = @LDFLAGS@

AR = @AR@

# Stuff to include in the dist that doesn't need it's own
# Makefile.am files
EXTRA_DIST = \
        ChangeLog.0 \
	Makefile.am \
	Makefile.in \
	charset.c \
	charset.h \
	pma.h \
	pma.c

# what to make and install
noinst_LIBRARIES = libsupport.a
libsupport_a_SOURCES = \
	getopt.c \
	getopt.h \
	getopt1.c \
	getopt_int.h \
	minrx.c \
	minrx.h \
	random.c \
	random.h

if USE_PERSISTENT_MALLOC
libsupport_a_SOURCES += pma.c pma.h
endif

# For some make's, e.g. OpenBSD, that don't define this
RM = rm -f

# For debugging
DEBUG = -gdwarf-4 -g3

DEFS = -DGAWK -DHAVE_CONFIG_H -I"$(srcdir)/.."

distclean-local:
	rm -fr .deps malloc/.deps
