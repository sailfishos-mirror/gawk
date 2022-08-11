dnl Decide whether or not to use the persistent memory allocator

# Copyright (C) 2022 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

AC_DEFUN([GAWK_USE_PERSISTENT_MALLOC],
[
AC_CHECK_SIZEOF([void *])
use_persistent_malloc=no
if test "$SKIP_PERSIST_MALLOC" = no && test $ac_cv_sizeof_void_p -eq 8
then
	AC_CHECK_FUNC([mmap])
	AC_CHECK_FUNC([munmap])
	if test $ac_cv_func_mmap = yes && test $ac_cv_func_munmap = yes
	then
		use_persistent_malloc=yes
		case $host_os in
		linux-*)
			case $CC in
			gcc | clang)
				LDFLAGS="${LDFLAGS} -no-pie"
				export LDFLAGS
				;;
			*)
				# tinycc and pcc don't support -no-pie flag
				# their executables are non-PIE automatically
				# so no need to do anything
				;;
			esac
			;;
		*darwin*)
			LDFLAGS="${LDFLAGS} -Xlinker -no_pie"
			export LDFLAGS
			;;
		*cygwin* | *CYGWIN* | *solaris2.11* )
			true	# nothing do, Cygwin and Solaris exes are not PIE
			;;
		# Other OS's go here...
		*)
			# For now, play it safe
			use_persistent_malloc=no
			;;
		esac
	else
		use_persistent_malloc=no
	fi
fi

AM_CONDITIONAL([USE_PERSISTENT_MALLOC], [test "$use_persistent_malloc" = "yes"])

if test "$use_persistent_malloc" = "yes"
then
	AC_DEFINE(USE_PERSISTENT_MALLOC, 1, [Define to 1 if we can use the pma allocator])
fi
])
