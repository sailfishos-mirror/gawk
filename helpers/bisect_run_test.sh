#!/bin/sh

# freshen these to avoid attempting to remake them
touch awkgram.c command.c

#echo "LC_ALL = $LC_ALL"
#echo "LANG = $LANG"
echo Building
make clean > /dev/null
rm -f gawk
make > /dev/null 2>/dev/null
ls -l gawk
if ./gawk -f /tmp/test.awk 2>&1 | diff /tmp/test.out - ; then
   echo Success
   rc=0
else
   echo Failure
   rc=1
fi
git reset --hard
exit $rc
