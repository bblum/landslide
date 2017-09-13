#!/bin/bash

function die() {
	echo -ne '\033[01;31m'
	echo "$1"
	echo -ne '\033[00m'
	exit 1
}

if [ ! -d "pebsim" -o ! -d "src/bochs-2.6.8/instrument/landslide" ]; then
	die "$0 must be run from the root of the landslide repository."
fi

INSTALLDIR="$PWD/install"
CONFIGURE_CMD="./configure --enable-all-optimizations --enable-idle-hack --enable-cpu-level=6 --enable-smp --enable-x86-64 --enable-gameport --enable-disasm --enable-plugins --with-nogui --without-wx --with-x --with-x11 --with-term --enable-debugger --enable-readline --disable-logging --disable-docbook --enable-instrumentation=instrument/landslide --prefix=$INSTALLDIR --exec-prefix=$INSTALLDIR"

mkdir -p install || die "couldnt make install dir"
cd src/bochs-2.6.8 || die "??"

if [ -f "config.status" ]; then
	if grep -- "--prefix=$INSTALLDIR" config.status >/dev/null; then
		if grep -- "--exec-prefix=$INSTALLDIR" config.status >/dev/null; then
			exit 0
		fi
	fi
fi

$CONFIGURE_CMD
RV=$?
if [ "$RV" != 0 ]; then
	rm -f config.status
	die "failed ./configure"
fi
cd ../../
