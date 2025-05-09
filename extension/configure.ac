dnl
dnl configure.ac --- autoconf input file for gawk
dnl
dnl Copyright (C) 2012-2021, 2023-2025 the Free Software Foundation, Inc.
dnl
dnl This file is part of GAWK, the GNU implementation of the
dnl AWK Programming Language.
dnl
dnl GAWK is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl GAWK is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
dnl

dnl Process this file with autoconf to produce a configure script.

AC_INIT([GNU Awk Bundled Extensions],[5.3.2],[bug-gawk@gnu.org],[gawk-extensions])

AC_PREREQ([2.71])

AC_CONFIG_MACRO_DIR([../m4])
AC_CONFIG_AUX_DIR([../build-aux])

AM_INIT_AUTOMAKE([1.16 -Wall -Werror])

SKIP_PERSIST_MALLOC=no
AC_ARG_ENABLE([pma],
	[AS_HELP_STRING([--disable-pma],[do not build gawk with the persistent memory allocator])],
	if test "$enableval" = no
	then
		SKIP_PERSIST_MALLOC=yes
	fi
)

GAWK_CANONICAL_HOST
AC_USE_SYSTEM_EXTENSIONS
AC_ZOS_USS

INSTALL="$ac_aux_dir/install-sh -c"
export INSTALL

SKIP_MPFR=no
AC_ARG_ENABLE([mpfr],
	[AS_HELP_STRING([--disable-mpfr],[do not check for MPFR])],
	if test "$enableval" = no
	then
		SKIP_MPFR=yes
	fi
)

dnl Set the programming language for checks. Fortunately,
dnl this only needs to be set once, since everything is in C.
AC_LANG([C])

dnl initialize GNU gettext
dnl this seems to help with finding MPFR on some systems, too.
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.20.2])
AM_LANGINFO_CODESET
gt_LC_MESSAGES

dnl checks for structure members
AC_CHECK_MEMBERS([struct stat.st_blksize])

AM_PROG_AR
AC_SYS_LARGEFILE
GAWK_USE_PERSISTENT_MALLOC
LT_INIT([win32-dll disable-static])
dnl AC_PROG_INSTALL

dnl use the same definition as the main configure script.
dnl EXTENSIONDIR is exported if it was set there.
dnl N.B. We must use ${libdir}/gawk instead of $pkglibdir because $pkglibdir
dnl is set to ${libdir}/gawk-extensions, which is not what we want.
AC_SUBST([pkgextensiondir], ['${libdir}/gawk'${EXTENSIONDIR}])

if test "$GCC" = yes
then
	CFLAGS="$CFLAGS -Wall"	# Don't add -Wextra, hurts older gcc
fi

AC_MSG_CHECKING([for special development options])
if test -f $srcdir/../.developing
then
	if test "$GCC" = yes
	then
		CFLAGS="$CFLAGS -Wall -fno-builtin"
	fi
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
	CFLAGS="$CFLAGS -DNDEBUG"	# turn off assertions
fi

AC_CHECK_HEADERS(fnmatch.h limits.h sys/mkdev.h sys/param.h sys/select.h \
		sys/statvfs.h sys/sysmacros.h sys/time.h)

AC_HEADER_DIRENT
AC_CHECK_HEADERS_ONCE([sys/time.h])


dnl check for mpfr support
case $SKIP_MPFR in
no)	GNUPG_CHECK_MPFR
	;;
esac

AC_SEARCH_LIBS(fmod, m)
AC_CHECK_FUNCS(fdopendir fmod fnmatch getdtablesize \
		clock_gettime gettimeofday \
		nanosleep select statvfs \
		GetSystemTimeAsFileTime strptime)

GAWK_FUNC_DIRFD
GAWK_PREREQ_DIRFD

dnl checks for compiler characteristics
AC_C_INLINE

AC_CONFIG_HEADERS([config.h:configh.in])
AH_BOTTOM([#include "ext_custom.h"])

AC_CONFIG_FILES(Makefile po/Makefile.in)
AC_OUTPUT
if test "$GCC" = yes &&
   test -f $srcdir/../.developing &&
   grep -i debug $srcdir/../.developing > /dev/null
then
	sed -e '/-O2/s///' -e '/^CFLAGS = /s//&${DEBUG} /' Makefile > foo
	mv foo Makefile
fi
