#!/bin/sh

ENV_ERR="You have to select a C compiler first."

[ -z $CC ] && { echo $ENV_ERR; exit 1; }

CFLAGS="-std=gnu99 -pedantic -Wall -Wextra -lm -msse4.1"

build_all() {
	# examples
	$CC $CFLAGS -g -O0 examples/bmp/bmp_example.c -o examples/bmp/bmp_example -Iinclude source/Skribist.c &
	# stress tests
	$CC $CFLAGS     -O3 stress/stress.c -o stress/stress.op -Iinclude source/Skribist.c &
	$CC $CFLAGS -g  -O0 stress/stress.c -o stress/stress.dg -Iinclude source/Skribist.c &
	$CC $CFLAGS -pg -O0 stress/stress.c -o stress/stress.pg -Iinclude source/Skribist.c &
	wait
}

time build_all
