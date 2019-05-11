#!/bin/sh

source ../environ.sh
nasm -felf64 ../source/Platform.S -o ../Platform.o
$CC $CFLAGS -g -O2 stress.c -o stress.dg -I../include ../source/Skribist.c ../Platform.o
