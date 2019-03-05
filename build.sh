#!/bin/sh

ENV_ERR="You have to set up a build environment first."

[ -z $CC ] && { echo $ENV_ERR; exit 1; }

CFLAGS="$CFLAGS -std=gnu99 -pedantic -Wall -Wextra"

build_Skribist() {
	$CC $CFLAGS -c source/Skribist.c -o Skribist.o -Iinclude
	ar -rcs libSkribist.a Skribist.o
}

build_examples() {
	$CC $CFLAGS examples/bmp/bmp_example.c -o examples/bmp/bmp_example -Iinclude libSkribist.a -lm
}

build_all() {
	build_Skribist
	build_examples
}

time build_all
