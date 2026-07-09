#! /bin/bash

# This script is only useful for the maintainer ...
#
# It updates all the support/* files from current GNULIB.
# We don't bother to print any messages about what we copied,
# as Git will tell us what, if anything, changed.

(cd /usr/local/src/Gnu/gnulib && git pull)

GL=/usr/local/src/Gnu/gnulib/lib

FILE_LIST="cdefs.h
dfa.c
dfa.h
dynarray.h
flexmember.h
idx.h
intprops.h
intprops-internal.h
libc-config.h
localeinfo.c
localeinfo.h
regcomp.c
regex.c
regexec.c
regex.h
regex_internal.c
regex_internal.h
verify.h
malloc/dynarray_at_failure.c
malloc/dynarray_emplace_enlarge.c
malloc/dynarray_finalize.c
malloc/dynarray.h
malloc/dynarray_resize.c
malloc/dynarray_resize_clear.c
malloc/dynarray-skeleton.c"

for i in $FILE_LIST
do
	if [ -f $GL/$i ] && [ -f support/$i ]
	then
		cp $GL/$i support/$i
	fi
done

cd support
rm -f pma.c pma.h
wget --no-check-certificate https://web.eecs.umich.edu/~tpkelly/pma/latest/pma.h
wget --no-check-certificate https://web.eecs.umich.edu/~tpkelly/pma/latest/pma.c
cd ..


M4_FILE_LIST="
codeset.m4
host-cpu-c-abi.m4
iconv.m4
intlmacosx.m4
lib-ld.m4
lib-link.m4
lib-prefix.m4
longlong.m4
nls.m4
printf-posix.m4
progtest.m4
size_max.m4
stdint_h.m4
wint_t.m4
"
for i in $M4_FILE_LIST
do
	if [ -f $GL/../m4/$i ] && [ -f m4/$i ]
	then
		cp $GL/../m4/$i m4/$i
	fi
done
