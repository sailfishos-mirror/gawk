#! /bin/bash

for i in $( grep ^diff /tmp/BIGDIFF | egrep '(LINGUAS|\.pot?)$' | gawk '{ print $NF }' | sed 's;.*/;;' )
do
	echo echo Extracting po/$i
	echo "cat << \\EOF > po/$i"
	cat $i
	echo EOF
done
