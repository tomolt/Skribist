#!/bin/sh

set -e

source ./environ.sh

build_variant() {
	mkdir -p build/$1
	nasm -felf64 source/Platform.S -o build/$1/Platform.o
	for src in source/*.c; do
		obj=build/$1/$(basename $src .c).o
		$CC $CFLAGS $SKR_CFLAGS $2 $src -c -o $obj -Iinclude
	done
	ar rcs build/$1/libSkribist.a build/$1/*.o
}

build_all() {
	build_variant "d0" "-g -O0"
	build_variant "d2" "-g -O2"
	build_variant "p2" "-pg -O2"
	build_variant "o3" "-O3"
	$CC $CFLAGS -g -O0 examples/bitmap.c -c -o build/bitmap.o -Iinclude
	$CC $CFLAGS -g -O0 stress/stress.c -c -o build/stress.o -Iinclude
	$CC build/bitmap.o build/d0/libSkribist.a -o examples/bitmap -lm
	$CC build/stress.o build/d2/libSkribist.a -o stress/stress.d2 -lm
	$CC build/stress.o build/p2/libSkribist.a -o stress/stress.p2 -lm -pg
	$CC build/stress.o build/o3/libSkribist.a -o stress/stress.o3 -lm
}

time build_all

