#!/bin/bash

function msg() {
	echo -ne '\033[01;36m'
	echo "$1"
	echo -ne '\033[00m'
}

function die() {
	echo -ne '\033[01;31m'
	echo "$1"
	echo -ne '\033[00m'
	exit 1
}

# Some quick checks before we get started with side effects
if [ -z "$1" ]; then
	die "usage: ./setup.sh ABSOLUTE_PATH_TO_P2_DIRECTORY"
fi
if [ ! -d "$1" ]; then
	die "argument '$1' is not a directory"
fi

# Get started with side effects.

# skip this if patch already applied
if ! grep -- "--std=gnu++0x" src/bochs-2.6.8/instrument/landslide/Makefile.in >/dev/null; then
	# PSU cluster machines don't have a gcc new enough for gnu++11
	# -N: forwards (don't ask if seems reversed); -f: force (don't ask questions)
	patch -N -f -p1 < gross-patches/psu-gnu0xx.patch || die "couldn't patch landslide for gnu0xx"
fi

# set paths appropriately and configure bochs
sed -i 's/--with-x/--without-x/' ./prepare-workspace.sh || die "couldn't sed prepare"
sed -i 's/--with-x11/--without-x11/' ./prepare-workspace.sh || die "couldn't sed prepare"
./prepare-workspace.sh || die "couldn't prepare workspace"

VERSION_FILE=current-git-commit.txt
rm -f "$VERSION_FILE"
git show | head -n 1 > "$VERSION_FILE"

# Build iterative deepening wrapper.

cd id || die "couldn't cd into id"
sed -i 's@system_cpus / 2@system_cpus@' option.c || msg "couldn't adjust default cpu number"
make || die "couldn't build id program"

# Put config.landslide into place.

cd ../pebsim || die "couldn't cd into pebsim"

rm -f current-psu-group.txt
echo "$1" > current-psu-group.txt
rm -f current-architecture.txt
echo "psu" > current-architecture.txt

CONFIG=config.landslide.pathos-p2

[ -f $CONFIG ] || die "couldn't find appropriate config: $CONFIG"

rm -f config.landslide
ln -s $CONFIG config.landslide || die "couldn't create config symlink"
rm -f bochsrc.txt || die "couldn't clear symlink bochsrc.txt"
ln -s bochsrc-pebbles.txt bochsrc.txt || die "couldn't create bochsrc symlink"

# Import and build student p2.

cd p2-basecode || die "couldn't cd into p2 basecode directory"

# update makefile for different userspace library requiremence, if necessary
# but also supports cmu projecce
if [ ! -d "$1/user/libautostack" ]; then
	msg "setting up PSU-compatible (no libautostack) build"
	if grep "^STUDENT_LIBS_EARLY *=.*libautostack.a" Makefile >/dev/null; then
		sed -i 's/\(^STUDENT_LIBS_EARLY *=.*\) libautostack.a/\1/' Makefile || die "couldn't remove libautostack from libs-early"
	fi
else
	msg "setting up CMU-compatible (yes libautostack) build"
fi
if [ -d "$1/user/libatomic" ]; then
	msg "setting up PSU-compatible (libatomic) build"
	if ! grep "^STUDENT_LIBS_LATE *=.*libatomic.a" Makefile >/dev/null; then
		sed -i 's/\(^STUDENT_LIBS_LATE *=.*\)/\1 libatomic.a/' Makefile || die "couldn't add libatomic to libs-late"
	fi
else
	msg "setting up CMU-compatible (no libatomic) build"
fi

# PSU's cluster machines suck
if grep -- "-fno-aggressive-loop-optimizations" Makefile >/dev/null; then
	if ! gcc -c -x c -fno-aggressive-loop-optimizations /dev/null -o /dev/null 2>/dev/null; then
		msg "gcc version is too old; disabling -fno-aggressive-loop-optimizations"
		sed -i 's/-fno-aggressive-loop-optimizations//' Makefile || die "couldn't disable it"
	fi
fi

P2DIR="$PWD"
msg "Importing your project into '$P2DIR' - look there if something goes wrong..."

./import-p2.sh "$1" || die "could not import your p2"

make || die "source code import was successful, but build failed (from '$PWD')"

cp bootfd.img ../../pebsim/ || die "couldn't move floppy disk image (from '$PWD')"
cp kernel ../../pebsim/ || die "couldn't move kernel binary (from '$PWD')"
# TODO: make line numbers file and symbol tabble
rm -f ../../src/bochs-2.6.8/instrument/landslide/line_numbers.h || die "couldn't clear linenrs"
touch ../../src/bochs-2.6.8/instrument/landslide/line_numbers.h || die "couldn't make linenrs"

cd ../../pebsim/ || die "couldn't cd into pebsim"

msg "Setting up Landslide..."

rm -f ../work/modules/landslide/student_specifics.h
export LANDSLIDE_CONFIG=config.landslide
./build.sh || die "Failed to compile landslide. Please send a tarball of this directory to Ben for assistance."

echo -ne '\033[01;33m'
echo "Note: Your P2 was imported into '$P2DIR'. If you wish to make changes to it, recommend editing it in '$1' then running this script again."
echo -ne '\033[01;32m'
echo "Setup successful. Can now run ./landslide."
echo -ne '\033[00m'
