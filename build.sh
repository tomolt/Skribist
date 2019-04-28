#!/bin/sh

build_all() {
	# examples
	$CC $CFLAGS -g -O0 examples/bmp/bmp_example.c -o examples/bmp/bmp_example -Iinclude source/Skribist.c -lm &
	# stress tests
	$CC $CFLAGS     -O3 stress/stress.c -o stress/stress.op -Iinclude source/Skribist.c -lm &
	$CC $CFLAGS -g  -O0 stress/stress.c -o stress/stress.dg -Iinclude source/Skribist.c -lm &
	$CC $CFLAGS -pg -O0 stress/stress.c -o stress/stress.pg -Iinclude source/Skribist.c -lm &
	wait
}

source ./environ.sh
time build_all
