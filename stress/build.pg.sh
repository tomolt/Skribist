#!/bin/sh

source ../environ.sh
nasm -felf64 ../source/Platform.S -o ../Platform.o
$CC $CFLAGS -pg -O0 stress.c -o stress.op -I../include ../source/Skribist.c ../Platform.o
