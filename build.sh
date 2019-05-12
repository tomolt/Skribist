#!/bin/sh

source ./environ.sh

build_variant() {
	mkdir -p build/$1 &&
	nasm -felf64 source/Platform.S -o build/$1/Platform.o &&
	$CC $CFLAGS $SKR_CFLAGS $2 source/Skribist.c -c -o build/$1/Skribist.o -Iinclude &&
	ar rcs build/$1/libSkribist.a build/$1/*.o
}

build_all() {
	build_variant "d0" "-g -O0"
	build_variant "d2" "-g -O2"
	build_variant "p2" "-pg -O2"
	build_variant "o3" "-O3"
	$CC $CFLAGS -g -O0 examples/bitmap.c build/d0/libSkribist.a -o examples/bitmap -Iinclude -lm
	$CC $CFLAGS -g -O0 stress/stress.c build/d2/libSkribist.a -o stress/stress.d2 -Iinclude -lm
	$CC $CFLAGS -g -O0 stress/stress.c build/p2/libSkribist.a -o stress/stress.p2 -Iinclude -lm
	$CC $CFLAGS -g -O0 stress/stress.c build/o3/libSkribist.a -o stress/stress.o3 -Iinclude -lm
}

time build_all

