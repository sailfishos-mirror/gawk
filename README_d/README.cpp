Wed Oct 16 10:26:37 AM IDT 2024
===============================

This branch can be compiled with a C++ compiler. The steps, however,
are manual.  Basically, you do a regular build:

	./bootstrap.sh && ./configure && make -j
	rm *.o		# top level directory only
	make CC=g++	# or clang++
	make check

Some of the changes should be propogated back into the mainline
version, but I haven't had time for that.

I also haven't decided if I want to move the rest of the changes
into the mainline version; in some ways they make the code less clear,
due to different scoping rules between C and C++

Arnold Robbins
