#! /bin/sh

# This script handles checking multiple .ok files. It will make it
# easy to add such tests in the future.

awkprog=$1
testprog=$2
srcdir=$3

if [ "$GAWKLOCALE" ]
then
	export LC_ALL=$GAWKLOCALE
fi

# Note the output redirection on the if ... fi !
if [ -f $srcdir/$testprog.in ]
then
	$awkprog -f $srcdir/$testprog.awk < $srcdir/$testprog.in
else
	$awkprog -f $srcdir/$testprog.awk
fi > _$testprog

for ok in $srcdir/$testprog.ok*
do
	if cmp -s _$testprog $ok
	then
		rm -f _$testprog
		exit 0
	fi
done

exit 1
