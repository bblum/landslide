#!/bin/bash

DIR=$1

function die() {
	echo -ne '\033[01;31m'
	echo "$1"
	echo -ne '\033[00m'
	exit 1
}

function check_subdir() {
	if [ ! -d "$DIR/$1" ]; then
		die "$DIR doesn't look like a p2 directory -- $1 subdir missing (try creating it and try again)"
	fi
}

ARCH="`cat ../current-architecture.txt`"
if [ "$ARCH" = "psu" ]; then
	PSU=1
elif [ "$ARCH" = "p2" ]; then
	PSU=0
else
	die "unknown architecture $ARCH in current-architecture?"
fi

if [ -z "$DIR" ]; then
	die "usage: $0 <absolute-path-to-p2-directory>"
fi
if [ ! -d "$DIR" ]; then
	die "$DIR not a directory"
fi
check_subdir 410kern
check_subdir 410user
check_subdir spec
check_subdir user
check_subdir user/libthread
check_subdir user/libsyscall
if [ "$PSU" = 0 ]; then
	check_subdir user/libautostack
fi
check_subdir user/inc
# check_subdir vq_challenge

DESTDIR=p2-basecode
if [ "`basename $PWD`" != "$DESTDIR" ]; then
	die "$0 run from wrong directory -- need to cd into $DESTDIR, wherever that is"
fi

function sync_subdir() {
	mkdir -p "./$1"
	rsync -rlpvgoD --delete "$DIR/$1/" "./$1/" || die "rsync failed."
}
function sync_optional_subdir() {
	if [ -d "$DIR/$1" ]; then
		sync_subdir "$1"
	fi
}
function sync_optional_file() {
	if [ -f "$DIR/$1" ]; then
		mkdir -p "`dirname $1`"
		cp "$DIR/$1" "./$1" || die "couldn't copy $1"
	fi
}

# note: if you update this you need to update check-for...sh too
sync_optional_subdir vq_challenge
sync_subdir user/inc
if [ "$PSU" = 0 ]; then
	# allow only cmu implementations
	sync_subdir user/libautostack
else
	# allow either implementation
	sync_optional_subdir user/libautostack
	sync_optional_subdir user/libatomic
	# i'm not sure why timmy needed these
	sync_optional_file 410user/crt0.c
	sync_optional_file 410user/user.mk
	sync_optional_file 410user/inc/atomic.h
	sync_optional_file 410user/inc/rwlock.h
	sync_optional_file 410user/libstdlib/panic.c
	sync_optional_file 410user/libstdlib/user.mk
fi
sync_subdir user/libsyscall
sync_subdir user/libthread
sync_optional_file user/config.mk # e.g. refp2 has this

# e.g. refp2 has this
if [ -f "$DIR/user/config.mk" ]; then
	cp "$DIR/user/config.mk" "./user/"
fi

# Merge student config.mk targets into ours

CONFIG_MK_PATTERN="THREAD_OBJS\|SYSCALL_OBJS\|AUTOSTACK_OBJS"
if [ "$PSU" = 1 ]; then
	CONFIG_MK_PATTERN="$CONFIG_MK_PATTERN\|ATOMIC_OBJS"
fi

rm -f config.mk
grep -v "$CONFIG_MK_PATTERN" config-incomplete.mk >> config.mk
# replace backslash-newlines so grep finds entire list
cat "$DIR/config.mk" | tr '\n' '@' | sed 's/\\@/ /g' | sed 's/@/\n/g' | grep "$CONFIG_MK_PATTERN" >> config.mk

echo "import p2 success; you may now make, hopefully"
