#! /bin/bash

make distclean >/dev/null 2>&1 || true
rm -fr autom4te.cache
total_files=$(find * -type f | wc -l)

rm -f command.c awkgram.c
line_count=$(wc -l *.[chy] */*.[ch] */*.pc */*.vms | grep total | awk '{ print $1 }')

git checkout command.c awkgram.c

echo Total files: $total_files
echo Total lines: $line_count
