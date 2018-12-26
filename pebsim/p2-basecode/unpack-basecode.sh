#!/bin/bash

function die() {
        echo -ne '\033[01;31m'
        echo "$1"
        echo -ne '\033[00m'
        exit 1
}

if [ -d "410user" ]; then
	echo "p2 basecode appears to be already unpacked..."
	exit 0
fi

LSTESTS=landslide-friendly-tests
BASECODE=p2-basecode-copyright-cmu-15-410-course-staff-see-license-inside.tar.bz2

if [ ! -d "$LSTESTS" ]; then
	die "where my test cases be at?"
fi
if [ ! -f "$BASECODE" ]; then
	die "where my basecode be at?"
fi

tar jxfv "$BASECODE" || die "failed untar"

cp $LSTESTS/*.c 410user/progs || die "failed to copy tests to 410user progs"
cp $LSTESTS/inc/* 410user/inc || die "failed to copy headers"
cp $LSTESTS/inc/* 410user/inc || die "failed to copy headers"
cp $LSTESTS/libhtm/* 410user/libhtm || die "failed to copy libhtm"

echo "successfully unpacked p2 basecode."
