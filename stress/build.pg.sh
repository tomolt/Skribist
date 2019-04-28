#!/bin/sh

source ../environ.sh
$CC $CFLAGS -pg -O0 stress.c -o stress.op -I../include ../source/Skribist.c
