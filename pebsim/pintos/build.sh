#!/bin/bash

# arguments: 3x either 0 or 1, each corresponding to userprog, vm, or filesys
# projects. if vm or filesys, userprog should also be 1. not both vm and filesys
# should be 1.

# Converts pintos source (with landslide annotations already applied) into a
# bootfd.img and kernel binary and puts them into pebsim/.

function msg {
        echo -e "\033[01;33m$1\033[00m"
}
function err {
        echo -e "\033[01;31m$1\033[00m" >&2
}
function die() {
	err "$1"
	exit 1
}


USERPROG=$1
VM=$2
FILESYS=$3
if [ -z "$USERPROG" ]; then
	die "usage: $0 <userprog>"
elif [ "$USERPROG" = "0" ]; then
	[ "$VM" = "0" -a "$FILESYS" = "0" ] || die "need specify userprog if you also specify vm or filesys"
	PROJECT=threads
elif [ "$VM" = "1" ]; then
	[ "$FILESYS" = "0" ] || die "can't be both vm and filesys, pick one"
	PROJECT=vm
elif [ "$FILESYS" = "1" ]; then
	PROJECT=filesys
else
	PROJECT=userprog
fi

msg "20th century pintos, $PROJECT edition."

cd pintos/src/$PROJECT
make || die "failed make"
cd ../../../
if [ -f kernel.o ]; then
	mv kernel.o kernel.o.old
fi
if [ -f kernel.o.strip ]; then
	mv kernel.o.strip kernel.o.strip.old
fi
if [ -f kernel.sym ]; then
	mv kernel.sym kernel.sym.old
fi
cp pintos/src/$PROJECT/build/kernel.o kernel.o || die "failed cp kernel.o"
./fix-symbols.sh kernel.o || die "failed fix symbols"

# Put stuff in the right place for make-bootfd script
cp pintos/src/$PROJECT/build/kernel.bin kernel.bin || die "failed cp kernel.bin"
cp pintos/src/$PROJECT/build/loader.bin loader.bin || die "failed cp loader.bin"

./make-bootfd.sh || die "couldn't make boot disk image"

# make symbol table file for function names
nm kernel.o | sed 's/ . / /' > kernel.sym || die "failed nm symbol table"

# make header file for filenames and line numbers
ADDRS_FILE=`mktemp kernel-code-addrs.XXXXXXXX.txt`
LINES_FILE=line_numbers.h
[ -f "$ADDRS_FILE" ] || die "failed make temp file for code addrs"
rm -f "$LINES_FILE" || die "failed rm old line numbers header"
# generates a file of lines each with e.g. "c002abcd"
objdump -d kernel.o | grep '^\S*:' | sed 's/:.*//' | grep -v kernel.o | sort > "$ADDRS_FILE" || die "failed make code addrs list"
# generates a file of e.g. "{ .eip = 0xc002abcd, .filename = "c.c", .line = 42 },"
# see symtable.c for the expected format / usage context
cat "$ADDRS_FILE" | addr2line -e kernel.o | sed 's@.*../../@@' | sed 's/ (discriminator.*//' | sed 's/^??:/unknown:/' | sed 's/:?$/:0/' | sed 's/^/.filename = "/' | sed 's/:/", .line = /' | sed 's/$/ },/' | paste "$ADDRS_FILE" - | sed 's/^/{ .eip = 0x/' | sed 's/\t/, /' > "$LINES_FILE" || die "failed generate line numbers header"
rm "$ADDRS_FILE" || msg "warning: couldnt remove temp file of code noobs"

msg "Pintos images built successfully."

# Put final product in parent pebsim directory
mv bootfd.img ../bootfd.img || die "failed mv bootfd.img"
mv kernel.o.strip ../kernel-pintos || die "failed mv kernel.o.strip"
mv kernel.sym ../kernel.sym || die "failed mv kernel.sym"
mv "$LINES_FILE" ../../src/bochs-2.6.8/instrument/landslide/ || die "failed mv linenrs"
rm -f ../kernel
ln -s kernel-pintos ../kernel || die "failed create kernel symlink"
