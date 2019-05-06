#!/bin/sh

source ../environ.sh
$CC $CFLAGS -g -O2 stress.c -o stress.dg -I../include ../source/Skribist.c
