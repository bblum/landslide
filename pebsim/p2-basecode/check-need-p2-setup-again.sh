#!/bin/bash

ARCH="`cat ../current-architecture.txt`"
if [ "$ARCH" = "psu" ]; then
	FILE="../current-psu-group.txt"
	SETUP_SCRIPT="./psu-setup.sh"
	PSU=1
elif [ "$ARCH" = "p2" ]; then
	FILE="../current-p2-group.txt"
	SETUP_SCRIPT="./p2-setup.sh"
        PSU=0
else
        die "unknown architecture $ARCH in current-architecture?"
fi

DISABLE_FILE="dont_check_for_p2_updates"
RERUN_MSG="Please re-run the $SETUP_SCRIPT script to make sure landslide sees your updates!"
DISABLE_MSG="If you want to disable this check, run 'touch $DISABLE_FILE'."

if [ -f "$DISABLE_FILE" -o -f "../$DISABLE_FILE" -o -f "../../$DISABLE_FILE" ]; then
	echo -e "\033[01;33mWarning: Not checking if you updated your p2 code since $SETUP_SCRIPT.\033[00m"
	echo -e "\033[01;33mTo re-enable this check, run 'rm $DISABLE_FILE'.\033[00m"
	exit 0
fi

if [ "`basename $PWD`" != "p2-basecode" ]; then
	echo "somehow, check-...sh not run from within p2-basecode directory? this is a bug."
	exit 0 # dont wanna lock studence in to this message
fi

function die() {
	echo -e "\033[01;31m$1\033[00m"
	echo -e "\033[01;31m$RERUN_MSG\033[00m"
	exit 1
}

function die_no_p2_path() {
	RERUN_MSG="Please run the $SETUP_SCRIPT script so landslide knows where to find your code!"
	die "Couldn't remember where your p2 directory is."
}

if [ ! -f "$FILE" ]; then
	die_no_p2_path
fi

# Check sanity of studence personal p2 directory.

ORIG_SRC_DIR=`cat "$FILE"`
if [ -z "$ORIG_SRC_DIR" ]; then
	die_no_p2_path
elif [ ! -d "$ORIG_SRC_DIR" ]; then
	die "Last $SETUP_SCRIPT argument was '$ORIG_SRC_DIR' but I can't see that directory anymore."
fi

# Check for updates in said personal directory.

function check_updates_in_subdir() {
	diff -qru --exclude="*.dep" --exclude="*.o" --exclude=".*" "$ORIG_SRC_DIR/$1/" "$1/" >/dev/null
	RV=$?
	if [ "$RV" != "0" ]; then
		die "Files in $ORIG_SRC_DIR have been updated since last $SETUP_SCRIPT!"
	fi
}
function check_updates_in_optional_subdir() {
	if [ -d "$DIR/$1" ]; then
		check_updates_in_subdir "$1"
	fi
}
function check_updates_in_optional_file() {
	if [ -f "$DIR/$1" ]; then
		diff -qu "$ORIG_SRC_DIR/$1" "$1" >/dev/null
		RV=$?
		if [ "$RV" != "0" ]; then
			die "File $ORIG_SRC_DIR/$1 has been updated since last $SETUP_SCRIPT!"
		fi
	fi
}

# compare to structure in import-p2.sh; this must match
check_updates_in_optional_subdir vq_challenge
check_updates_in_subdir user/inc
check_updates_in_subdir user/libsyscall
check_updates_in_subdir user/libthread
check_updates_in_optional_file user/config.mk
if [ "$PSU" = 0 ]; then
	# allow only cmu implementations
	check_updates_in_subdir user/libautostack
else
	# allow either type of implementation
	check_updates_in_optional_subdir user/libautostack
	check_updates_in_optional_subdir user/libatomic
fi

# Check for studence updating code in this import-destination directory.

MAKE_OUTPUPT=`PATH=/afs/cs.cmu.edu/academic/class/15410-s16/bin/:$PATH make --dry-run`
if [ "`echo "$MAKE_OUTPUPT" | wc -l`" != 1 ]; then
	die "Some files in pebsim/p2-basecode/ have been updated since $SETUP_SCRIPT was last run."
fi
