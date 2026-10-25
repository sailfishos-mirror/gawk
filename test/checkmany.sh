#! /bin/sh

# This script handles checking multiple .ok files. It will make it
# easy to add such tests in the future.

testprog=$1
srcdir=$2
CMP=${CMP:-cmp -s}

for ok in ./$testprog.ok* $srcdir/$testprog.ok*
do
	if test -f $ok
	then
		if /bin/sh -c "${CMP} $ok _$testprog"
		then
			rm -f _$testprog
			exit 0
		fi
	fi
done

exit 1
