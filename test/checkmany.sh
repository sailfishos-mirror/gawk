#! /bin/sh

# This script handles checking multiple .ok files. It will make it
# easy to add such tests in the future.

testprog=$1
srcdir=$2
CMP=${CMP:-cmp -s}

for ok in ./$testprog.ok* $srcdir/$testprog.ok*
do
	if test -f $ok && ${CMP} _$testprog $ok
	then
		rm -f _$testprog
		exit 0
	fi
done

exit 1
