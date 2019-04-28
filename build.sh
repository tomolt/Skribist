#!/bin/sh

build_dir() {
	pushd . > /dev/null
	cd $1
	./build.sh
	popd > /dev/null
}

build_all() {
	build_dir examples
	build_dir stress
}

source ./environ.sh
time build_all
