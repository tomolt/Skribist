#!/bin/sh

do_build() {
	mkdir -p build
	for SRC in source/*.c; do
		clang -c -g -std=gnu99 -pedantic -Wall -Wextra $SRC -o build/$(basename $SRC .c).o
	done
	ar -rcs libSkribist.a build/*.o
}
time do_build
