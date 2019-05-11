#!/bin/sh

source ../environ.sh
nasm -felf64 ../source/Platform.S -o ../Platform.o
$CC $CFLAGS -g -O0 bitmap.c -o bitmap -I../include ../source/Skribist.c ../Platform.o
