#!/bin/sh

ENV_ERR="You have to select a C compiler first."

[ -z $CC ] && { echo $ENV_ERR; exit 1; }

CFLAGS="-std=gnu99 -pedantic -Wall -Wextra"

build_all() {
	# examples
	$CC $CFLAGS -g -O0 examples/bmp/bmp_example.c -o examples/bmp/bmp_example -Iinclude source/Skribist.c -lm &
	# stress tests
	$CC $CFLAGS     -O3 stress/stress.c -o stress/stress.op -Iinclude source/Skribist.c -lm &
	$CC $CFLAGS -g  -O0 stress/stress.c -o stress/stress.dg -Iinclude source/Skribist.c -lm &
	$CC $CFLAGS -pg -O0 stress/stress.c -o stress/stress.pg -Iinclude source/Skribist.c -lm &
	wait
}

time build_all
