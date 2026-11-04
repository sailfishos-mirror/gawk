#! /bin/bash

# This script is only useful for the maintainer ...
#
# It updates all the support/* files from current GNULIB.
# We don't bother to print any messages about what we copied,
# as Git will tell us what, if anything, changed.

cd support
rm -f pma.c pma.h
wget --no-check-certificate https://web.eecs.umich.edu/~tpkelly/pma/latest/pma.h
wget --no-check-certificate https://web.eecs.umich.edu/~tpkelly/pma/latest/pma.c
cd ..

(cd /usr/local/src/Gnu/gnulib && git pull)

GL=/usr/local/src/Gnu/gnulib/lib

M4_FILE_LIST="
host-cpu-c-abi.m4
intlmacosx.m4
lib-ld.m4
lib-link.m4
lib-prefix.m4
nls.m4
progtest.m4
"

for i in $M4_FILE_LIST
do
	if [ -f $GL/../m4/$i ] && [ -f m4/$i ]
	then
		cp $GL/../m4/$i m4/$i
	fi
done
