#!/bin/sh

source ../environ.sh
$CC $CFLAGS -g -O0 stress.c -o stress.dg -I../include ../source/Skribist.c
