#!/bin/bash

DIR=$1
USERPROG=$2

function msg {
	echo -e "\033[01;33m$1\033[00m"
}

function die() {
	echo -ne '\033[01;31m'
	echo "$1"
	echo -ne '\033[00m'
	exit 1
}

function check_file() {
	if [ ! -f "$DIR/$1" ]; then
		die "$DIR doesn't look like a pintos directory -- $1 file missing (try creating it and try again)"
	fi
}
function check_subdir() {
	if [ ! -d "$DIR/$1" ]; then
		die "$DIR doesn't look like a pintos directory -- $1 subdir missing (try creating it and try again)"
	fi
}

if [ -z "$DIR" -o -z "$USERPROG" ]; then
	die "usage: $0 <absolute-path-to-pintos-directory> <userprog>"
fi
if [ ! -d "$DIR" ]; then
	die "$DIR not a directory"
fi
# look for src/.
if [ "$USERPROG" = "0" ]; then
	PROJ="p1"
else
	PROJ="p2"
fi
if [ ! -d "$DIR/src/threads" ]; then
	# support either "foo/" or "foo/pintos-pX/" or "foo/pX/",
	# when "foo" is supplied as DIR.
	if [ -d "$DIR/pintos-$PROJ/src/threads" ]; then
		DIR="$DIR/pintos-$PROJ"
	elif [ -d "$DIR/$PROJ/src/threads" ]; then
		DIR="$DIR/$PROJ"
	else
		die "Couldn't find src/ subdirectory in '$DIR'"
	fi
fi
check_subdir src/threads
check_subdir src/userprog
check_subdir src/vm
check_subdir src/devices
check_subdir src/filesys
check_subdir src/lib
check_subdir src/misc
check_subdir src/utils
check_file src/Makefile.build

# we should be inside pebsim/pintos/, and furthermore,
# we should copy files into pebsim/pintos/pintos/.
DESTDIR=pintos
SUBDIR=pintos

if [ "`basename $PWD`" != "$DESTDIR" ]; then
	die "$0 run from wrong directory -- need to cd into $DESTDIR, wherever that is"
fi

# Set up basecode if not already done.
if [ ! -d "$SUBDIR" ]; then
	REPO=group0
	if [ ! -d "$REPO" ]; then
		git clone "https://github.com/Berkeley-CS162/$REPO.git" || die "Couldn't clone basecode repository."
	fi
	mv "$REPO/$SUBDIR" "$SUBDIR" || die "couldn't bring $SUBDIR out of repository"
	rm -rf "$REPO" || die "couldn't remove rest of repository $REPO"
fi

function sync_file() {
	cp "$DIR/$1" "./$SUBDIR/$1" || die "cp of $1 failed."
}
function sync_subdir() {
	mkdir -p "./$SUBDIR/$1"
	rsync -rlpvgoD --delete "$DIR/$1/" "./$SUBDIR/$1/" || die "rsync failed."
}
function sync_optional_subdir() {
	if [ -d "$DIR/$1" ]; then
		sync_subdir "$1"
	fi
}

sync_subdir src/threads
sync_subdir src/userprog
sync_subdir src/vm
sync_subdir src/filesys # some studence may add fs code helper functions
# our basecode is missing some files / patches from the uchicago version.
sync_subdir src/devices
sync_subdir src/lib
sync_subdir src/misc
sync_subdir src/utils
sync_subdir src/tests
# And the makefile. Just clobber, applying patch will take care of rest.
sync_file src/Makefile.build
sync_file src/Make.config

# Patch source codez.

function check_file() {
	if [ ! -f "$1" ]; then
		die "Where's $1?"
	fi
}

# Fix bug in Pintos.pm in berkeley versions of basecode.
# This can't go in the patch because it's conditional on version...
PINTOS_PM="./$SUBDIR/src/utils/Pintos.pm"
check_file "$PINTOS_PM"
if grep "source = 'FILE'" "$PINTOS_PM" >/dev/null; then
	sed -i "s/source = 'FILE'/source = 'file'/" "$PINTOS_PM" || die "couldn't fix $PINTOS_PM"
	msg "Successfully fixed $PINTOS_PM."
fi

# Fix CFLAGS and apply part of patch manually. Needs to be sed instead of patch
# to compensate for variance among studence implementations.
MAKE_CONFIG="./$SUBDIR/src/Make.config"
check_file "$MAKE_CONFIG"
if grep "^CFLAGS =" "$MAKE_CONFIG" >/dev/null; then
	sed -i "s/^CFLAGS =/CFLAGS = -std=gnu99 -fno-omit-frame-pointer/" "$MAKE_CONFIG" || die "couldn't fix CFLAGS in $MAKE_CONFIG"
else
	die "$MAKE_CONFIG doesn't contain CFLAGS?"
fi

THREAD_H="./$SUBDIR/src/threads/thread.h"
THREAD_C="./$SUBDIR/src/threads/thread.c"
check_file "$THREAD_H"
check_file "$THREAD_C"

# Figure out whether the student changed ready_list to an array with whatever name
READY_LIST=`grep "static struct list.*ready" "$THREAD_C" | head -n 1`
[ ! -z "$READY_LIST" ] || die "couldn't find ready-list declaration in $THREAD_C"
READY_LIST_NAME=`echo "$READY_LIST" | sed 's/.* \([a-zA-Z0-9_]*ready[a-zA-Z0-9_]*\).*/\1/'`
if echo "$READY_LIST" | grep "ready.*\[.*\]" >/dev/null; then
	# it's an array; find out how many there are
	READY_LIST_LENGTH=`echo "$READY_LIST" | sed 's/.*\[\([0-9]*\)\].*/\1/'`
	[ ! -z "$READY_LIST_LENGTH" ] || die "couldn't get length of ready-list array"
	# add function to provide the length to the part of the patch in list.c
	echo "#define READY_LIST_LENGTH $READY_LIST_LENGTH" >> "$THREAD_H" || die "ready list length h"
	# make get rq addr (see below) return the address of the first element
	READY_LIST_NAME=`echo "$READY_LIST_NAME[0]"`
else
	# normal single runqueue
	echo "#define READY_LIST_LENGTH 1" >> "$THREAD_H" || die "ready list length h"
fi

# Fix TIME_SLICE issue causing landslide's tick mechanism letting instructions leak.
sed -i "s/define TIME_SLICE 4/define TIME_SLICE 1/" "$THREAD_C" || die "couldn't fix TIME_SLICE"

echo "struct list *get_rq_addr() { return &$READY_LIST_NAME; }" >> "$THREAD_C" || die "get rq addr c"
# It's ok for a function decl to go outside the ifdef.
echo "struct list *get_rq_addr(void);" >> "$THREAD_H" || die "get rq addr h"

# Remove whitespace in e.g. "thread_unblock (t)" so the patch applies
sed -i "s/ (/(/g" "$THREAD_C" || die "couldn't fix whitespace in $THREAD_C"

# Insert forking() annotation in a more robust way than context-dependent patch
# This is terrible :( really the right solution is to make landslide so forking()
# is not needed at all and it will just automatically add the new thread
sed -i "s@\(.*/\* Add to run queue\. \*/.*\)@  tell_landslide_forking(); \1@" "$THREAD_C"
# make sure the above hack doesn't get owned in the most obvious way
NUM_FORKINGS=`grep "tell_landslide_forking" "$THREAD_C" | wc -l`
[ "$NUM_FORKINGS" == 1 ] || die "ben, your forking annotation hack is broken on this pintos, try something else"

# Apply tell_landslide annotations.

PATCH=annotate-pintos.patch
if [ ! -f "$PATCH" ]; then
	die "can't see annotations patch file, $PATCH; where is it?"
fi
# -l = ignore whitespace
# -f = force, don't ask questions
patch -l -f -p1 -i "$PATCH" || die "Failed to patch in annotations. Please email Ben Blum <bblum@cs.cmu.edu> or your local OS course staff for help."

# success
echo "import pintos success; you may now make, hopefully"
