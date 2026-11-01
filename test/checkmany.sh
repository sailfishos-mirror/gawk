#! /bin/sh

# This script handles checking multiple .ok files. It will make it
# easy to add such tests in the future.

testprog=$1
srcdir=$2
CMP=${CMP:-cmp -s}

dirlist="$srcdir"
if [ "$srcdir" != "." ]
then
	dirlist="$dirlist ."
fi

for dir in $dirlist
do
	for ok in $dir/$testprog.ok*
	do
		if test -f $ok
		then
			# Don't print output, let make diffout handle it
			if /bin/sh -c "${CMP} $ok _$testprog" > /dev/null 2>&2
			then
				rm -f _$testprog
				exit 0
			fi
		fi
	done
done

exit 1
