#!/bin/sh

CC=gcc
CFLAGS="-g -std=gnu99 -pedantic -Wall -Wextra"

build_Skribist() {
	mkdir -p build
	for SRC in source/*.c; do
		$CC $CFLAGS -c $SRC -o build/$(basename $SRC .c).o
	done
	ar -rcs libSkribist.a build/*.o
}

build_examples() {
	$CC $CFLAGS examples/bmp/bmp_example.c -o examples/bmp/bmp_example -Isource libSkribist.a -lm
}

build_all() {
	build_Skribist
	build_examples
}

time build_all
