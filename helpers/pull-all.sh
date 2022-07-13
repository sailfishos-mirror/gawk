#! /bin/bash

for i in $( git branch | awk '{ print $NF }')
do
	git checkout $i && git pull
done

git checkout master
