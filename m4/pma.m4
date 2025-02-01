dnl Decide whether or not to use the persistent memory allocator
dnl
dnl Copyright (C) 2022, 2023, 2025 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([GAWK_USE_PERSISTENT_MALLOC],
[
AC_REQUIRE([AX_CHECK_COMPILE_FLAG])
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
			true	# On Linux we no longer need -no-pie
			;;
 		*darwin*)
			true	# On macos we no longer need -no-pie
			;;
		*cygwin* | *CYGWIN* | *solaris2.11* | freebsd13.* | openbsd7.* )
			true	# nothing to do, exes on these systems are not PIE
			;;
		# Other OS's go here...
		*)
			# For now, play it safe
			use_persistent_malloc=no

			# Allow override for testing on new systems
			if test "$REALLY_USE_PERSIST_MALLOC" != ""
			then
				use_persistent_malloc=yes
			fi
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
