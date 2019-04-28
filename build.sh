#!/bin/sh

build_all() {
	# examples
	$CC $CFLAGS -g -O0 examples/bitmap.c -o examples/bitmap -Iinclude source/Skribist.c &
	# stress tests
	$CC $CFLAGS     -O3 stress/stress.c -o stress/stress.op -Iinclude source/Skribist.c &
	$CC $CFLAGS -g  -O0 stress/stress.c -o stress/stress.dg -Iinclude source/Skribist.c &
	$CC $CFLAGS -pg -O0 stress/stress.c -o stress/stress.pg -Iinclude source/Skribist.c &
	wait
}

source ./environ.sh
time build_all
