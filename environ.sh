# This shell script sets build environment variables.
# It should only be called from within a build script.

#!/bin/sh

if [ -z $CC ]; then
	CC=clang
fi

CFLAGS="-std=gnu99 -pedantic -Wall -Wextra -msse4.1"
