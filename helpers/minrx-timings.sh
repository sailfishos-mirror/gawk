#! /bin/bash

echo 1. Create a large file

for ((i = 1; i <= 500; i++))
do	cat doc/gawk.texi 
done > BIGFILE

echo ls -l  BIGFILE
ls -l  BIGFILE
echo

echo ls -lh BIGFILE
ls -lh BIGFILE

echo 2. Fill the buffer cache:

cat BIGFILE > /dev/null

echo 3. Time the dfa matcher:

time ./gawk -G '/[nmg]?awk/' BIGFILE > /dev/null

echo 4. Time GNU regex:

time GAWK_NO_DFA=1 ./gawk -G '/[nmg]?awk/' BIGFILE > /dev/null

echo 5. Time the "'main'" + charset version of minrx:

time ./gawk '/[nmg]?awk/' BIGFILE > /dev/null

printf "Do you want to test a new version? [y/n] "
read answer

case $answer in
[yY]*)	printf "Please build the new version in another window and press ENTER when done: "
	read junk
	echo
	echo 6. Time new version of minrx:
	time ./gawk '/[nmg]?awk/' BIGFILE > /dev/null
	;;
esac

rm -f BIGFILE
