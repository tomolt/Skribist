#!/bin/sh

source ../environ.sh
$CC $CFLAGS -O3 stress.c -o stress.op -I../include ../source/Skribist.c
