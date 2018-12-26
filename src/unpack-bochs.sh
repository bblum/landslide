#!/bin/sh

function die() {
        echo -ne '\033[01;31m'
        echo "$1"
        echo -ne '\033[00m'
        exit 1
}

BOCHSDIR=bochs-2.6.8
BOCHSTARBALL=$BOCHSDIR.tar.gz
PATCH=patches/bochs-landslide-intrusiveness.patch

if [ ! -f "$BOCHSTARBALL" ]; then
	die "must be run from src/ directory with the bochs tarball in it"
fi
if [ ! -f "$PATCH" ]; then
	die "landslide intrusivenesss patch missing"
fi
if [ ! -d "landslide" ]; then
	die "where's landslide yo"
fi

if [ -d "$BOCHSDIR" ]; then
	echo "looks like bochs was already set up here"
	exit 0
fi

tar zvxf "$BOCHSTARBALL" || die "failed unpack bochs tarball"

patch -f -p1 -i "$PATCH" || die "failed patch landslide intrusiveness into bochs"

ln -s ../../landslide/ ./$BOCHSDIR/instrument/landslide || die "failed symlink in src"
ln -s ../patches/instrument.cc ./landslide/instrument.cc || die "failed link instrument"
ln -s ../patches/Makefile.in ./landslide/Makefile.in || die "failed link makefile"
ln -s ../rbtree/rbtree.h ./landslide/rbtree.h || die "failed link rbtree h"
ln -s ../rbtree/rbtree.c ./landslide/rbtree.c || die "failed link rbtree c"
echo "bochs unpacked successfully"
