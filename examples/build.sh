#!/bin/sh

source ../environ.sh
$CC $CFLAGS -g -O0 bitmap.c -o bitmap -I../include ../source/Skribist.c
