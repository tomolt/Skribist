#!/bin/sh

CC=gcc
CFLAGS="-g -std=gnu99 -pedantic -Wall -Wextra"

build_Skribist() {
	$CC $CFLAGS -c source/Skribist.c -o Skribist.o
	ar -rcs libSkribist.a Skribist.o
}

build_examples() {
	$CC $CFLAGS examples/bmp/bmp_example.c -o examples/bmp/bmp_example -Isource libSkribist.a -lm
}

build_all() {
	build_Skribist
	build_examples
}

time build_all
