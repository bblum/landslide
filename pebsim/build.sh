#!/bin/bash

# Copyright (c) 2018, Ben Blum
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright notice,
#       this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the copyright holder nor the names of its
#       contributors may be used to endorse or promote products derived from
#       this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# @file build.sh
# @brief Outermost wrapper for the landslide build process.
# @author Ben Blum

source ./getfunc.sh

function sched_func {
	echo -n
}
function ignore_sym {
	echo -n
}
# pp-related functions will be redefined later, but for now we ignore any pps
# defined in the static config file, which definegen will take care of
function within_function {
	echo -n
}
function without_function {
	echo -n
}
function within_user_function {
	echo -n
}
function without_user_function {
	echo -n
}
function ignore_dr_function {
	echo -n
}
function thrlib_function {
	echo -n
}
function data_race {
	echo -n
}
function disk_io_func {
	echo -n
}
function extra_sym {
	echo -n
}
function starting_threads {
	echo -n
}
function id_magic {
	echo -n
}
INPUT_PIPE=
function input_pipe {
	if [ ! -z "$INPUT_PIPE" ]; then
		die "input_pipe called more than once; oldval $INPUT_PIPE, newval $1"
	fi
	INPUT_PIPE=$1
}

OUTPUT_PIPE=
function output_pipe {
	if [ ! -z "$OUTPUT_PIPE" ]; then
		die "output_pipe called more than once; oldval $OUTPUT_PIPE, newval $1"
	fi
	OUTPUT_PIPE=$1
}

# Doesn't work without the "./". Everything is awful forever.
if [ ! -f "./$LANDSLIDE_CONFIG" ]; then
	die "Where's $LANDSLIDE_CONFIG?"
fi
TIMER_WRAPPER_DISPATCH=
IDLE_TID=
TESTING_USERSPACE=0
PREEMPT_ENABLE_FLAG=
PRINT_DATA_RACES=0
VERBOSE=0
EXTRA_VERBOSE=0
TABULAR_TRACE=0
OBFUSCATED_KERNEL=0
PINTOS_KERNEL=
source ./$LANDSLIDE_CONFIG

if [ ! -z "$QUICKSAND_CONFIG_STATIC" ]; then
	if [ ! -f "./$QUICKSAND_CONFIG_STATIC" ]; then
		die "Where's ID config $QUICKSAND_CONFIG_STATIC?"
	fi
	source "$QUICKSAND_CONFIG_STATIC"
fi

#### Check environment ####

if [ ! -d ../src/bochs-2.6.8/instrument/landslide ]; then
	die "Where's ../src/bochs-2.6.8/instrument/landslide?"
fi

#### Verify config options ####
function verify_nonempty {
	if [ -z "`eval echo \\$\$1`" ]; then
		die "Missing value for config option $1!"
	fi
}
function verify_numeric {
	verify_nonempty $1
	expr `eval echo \\$\$1` + 1 2>/dev/null >/dev/null
	if [ "$?" != 0 ]; then
		die "Value for $1 needs to be numeric; got \"`eval echo \\$\$1`\" instead."
	fi
}
function verify_file {
	verify_nonempty $1
	if [ ! -f "`eval echo \\$\$1`" ]; then
		die "$1 (\"`eval echo \\$\$1`\") doesn't seem to be a file."
	fi
}
function verify_dir {
	verify_nonempty $1
	if [ ! -d "`eval echo \\$\$1`" ]; then
		die "$1 (\"`eval echo \\$\$1`\") doesn't seem to be a directory."
	fi
}

verify_file KERNEL_IMG
verify_dir KERNEL_SOURCE_DIR
verify_nonempty TEST_CASE
verify_nonempty TIMER_WRAPPER
verify_nonempty CONTEXT_SWITCH
if [ -z "$PINTOS_KERNEL" ]; then
	verify_nonempty READLINE
	verify_numeric SHELL_TID
fi
verify_numeric INIT_TID
verify_numeric FIRST_TID
verify_numeric BUG_ON_THREADS_WEDGED
verify_numeric EXPLORE_BACKWARDS
verify_numeric DONT_EXPLORE
verify_numeric BREAK_ON_BUG
verify_numeric TESTING_USERSPACE
verify_numeric PRINT_DATA_RACES
verify_numeric VERBOSE
verify_numeric EXTRA_VERBOSE
verify_numeric TABULAR_TRACE
if [ "$TESTING_USERSPACE" = 1 ]; then
	verify_nonempty EXEC
fi

if [ ! -z "$PREEMPT_ENABLE_FLAG" ]; then
	verify_numeric PREEMPT_ENABLE_VALUE
fi

if [ ! "$CURRENT_THREAD_LIVES_ON_RQ" = "0" ]; then
	if [ ! "$CURRENT_THREAD_LIVES_ON_RQ" = "1" ]; then
		die "CURRENT_THREAD_LIVES_ON_RQ config must be either 0 or 1."
	fi
fi

#### Check kernel image ####

if [ ! "$OBFUSCATED_KERNEL" = 0 ]; then
	if [ ! "$OBFUSCATED_KERNEL" = 1 ]; then
		die "Invalid value for OBFUSCATED_KERNEL; have '$OBFUSCATED_KERNEL'; need 0/1"
	elif [ ! -z "$PINTOS_KERNEL" ]; then
		die "PINTOS_KERNEL and OBFUSCATED_KERNEL are incompatible."
	fi
fi

if [ -z "$PINTOS_KERNEL" ]; then
	# Pebbles
	if ! grep "${TEST_CASE}_exec2obj_userapp_code_ptr" $KERNEL_IMG 2>&1 >/dev/null; then
		die "Missing test program: $KERNEL_IMG isn't built with '$TEST_CASE'!"
	fi
else
	# Pintos. Verify
	cd pintos || die "couldn't cd pintos"
	./make-bootfd.sh "$TEST_CASE" || die "couldn't remake bootfd"
	if ! cmp bootfd.img ../bootfd.img >/dev/null 2>/dev/null; then
		msg "bootfd image needs updated to run $TEST_CASE"
		cp bootfd.img .. || die "made bootfd but failed cp"
	fi
	cd .. || die "?????.... ?"
fi

MISSING_ANNOTATIONS=
function verify_tell {
	if ! (objdump -d $KERNEL_IMG | grep "call.*$1" 2>&1 >/dev/null); then
		err "Missing annotation: $KERNEL_IMG never calls $1()"
		MISSING_ANNOTATIONS=very_yes
	fi
}

source ./symbols.sh

verify_tell "$TL_FORKING"
verify_tell "$TL_OFF_RQ"
verify_tell "$TL_ON_RQ"
verify_tell "$TL_SWITCH"
verify_tell "$TL_INIT_DONE"
verify_tell "$TL_VANISH"

if [ ! -z "$MISSING_ANNOTATIONS" ]; then
	die "Please fix the missing annotations."
fi

#### Check file sanity ####

HEADER=../src/bochs-2.6.8/instrument/landslide/student_specifics.h
STUDENT=../src/bochs-2.6.8/instrument/landslide/student.c
SKIP_HEADER=
if [ -L $HEADER ]; then
	rm $HEADER
elif [ -f $HEADER ]; then
	if grep 'automatically generated' $HEADER 2>&1 >/dev/null; then
		MD5SUM=`grep 'image md5sum' $HEADER | sed 's/.*md5sum //' | cut -d' ' -f1`
		MD5SUM_C=`grep 'config md5sum' $HEADER | sed 's/.*md5sum //' | cut -d' ' -f1`
		MD5SUM_ID=`grep 'deepening md5sum' $HEADER | sed 's/.*md5sum //' | cut -d' ' -f1`
		MY_MD5SUM=`md5sum $KERNEL_IMG | cut -d' ' -f1`
		MY_MD5SUM_C=`md5sum ./$LANDSLIDE_CONFIG | cut -d' ' -f1`
		if [ ! -z "$QUICKSAND_CONFIG_STATIC" ]; then
			MY_MD5SUM_ID=`md5sum ./$QUICKSAND_CONFIG_STATIC | cut -d' ' -f1`
		else
			MY_MD5SUM_ID="NONE"
		fi
		if [ "$MD5SUM" == "$MY_MD5SUM" -a "$MD5SUM_C" == "$MY_MD5SUM_C" -a "$MD5SUM_ID" == "$MY_MD5SUM_ID" ]; then
			SKIP_HEADER=yes
		else
			rm -f $HEADER || die "Couldn't overwrite existing header $HEADER"
		fi
	elif [ ! -z "$QUICKSAND_CONFIG_STATIC" ]; then
		# Run from QS. Attempt to silently clobber the existing header.
		rm -f "$HEADER" || die "Attempted to silently clobber $HEADER but failed!"
	else
		# Run in manual mode. Let user know about header problem.
		die "$HEADER exists, would be clobbered; please remove/relocate it."
	fi
fi

# generate dynamic pp config file independently of definegen

if [ ! -z "$QUICKSAND_CONFIG_DYNAMIC" ]; then
	if [ ! -f "$QUICKSAND_CONFIG_DYNAMIC" ]; then
		die "Where's $QUICKSAND_CONFIG_DYNAMIC?"
	fi
	# ./landslide defines QUICKSAND_CONFIG_TEMP as a temp file to use here
	[ ! -z "$QUICKSAND_CONFIG_TEMP" ] || die "failed make temp file for PP config"

	# commands are K, U, DR, I, and O.
	function within_function {
		echo "K 0x`get_func $1` 0x`get_func_end $1` 1" >> "$QUICKSAND_CONFIG_TEMP" || die "couldn't write to $QUICKSAND_CONFIG_TEMP"
	}
	function without_function {
		echo "K 0x`get_func $1` 0x`get_func_end $1` 0" >> "$QUICKSAND_CONFIG_TEMP" || die "couldn't write to $QUICKSAND_CONFIG_TEMP"
	}
	function within_user_function {
		echo "U 0x`get_user_func $1` 0x`get_user_func_end $1` 1" >> "$QUICKSAND_CONFIG_TEMP" || die "couldn't write to $QUICKSAND_CONFIG_TEMP"
	}
	function without_user_function {
		echo "U 0x`get_user_func $1` 0x`get_user_func_end $1` 0" >> "$QUICKSAND_CONFIG_TEMP" || die "couldn't write to $QUICKSAND_CONFIG_TEMP"
	}
	function data_race {
		if [ -z "$1" -o -z "$2" -o -z "$3" -o -z "$4" ]; then
			die "data_race needs four args: got \"$1\" and \"$2\" and \"$3\" and \"$4\""
		fi
		echo "DR $1 $2 $3 $4" >> "$QUICKSAND_CONFIG_TEMP" || die "couldn't write to $QUICKSAND_CONFIG_TEMP"
	}
	function input_pipe {
		echo "I $1" >> "$QUICKSAND_CONFIG_TEMP" || die "couldn't write to $QUICKSAND_CONFIG_TEMP"
	}
	function output_pipe {
		echo "O $1" >> "$QUICKSAND_CONFIG_TEMP" || die "couldn't write to $QUICKSAND_CONFIG_TEMP"
	}
	msg "Processing dynamic quicksand PPs..."
	source "$QUICKSAND_CONFIG_DYNAMIC"
fi

#### Do the needful ####

if [ -z "$SKIP_HEADER" ]; then
	msg "Generating header file..."
	./definegen.sh > $HEADER || (rm -f $HEADER; die "definegen.sh failed.")
	if [ -z "$PINTOS_KERNEL" ]; then
		# Make the symbol table here
		# (for pintos it's made during setup.sh, but userspace tests change)
		msg "Generating symbol table..."
		TEST_FILE=`get_test_file`
		[ -f "$TEST_FILE" ] || die "test case file misisng (exp'd $TEST_FILE)"
		CPPFILT=`which "c++filt" || echo "cat"`
		# XXX: following code duplicated with pintos/build.sh
		nm "$KERNEL_IMG" | $CPPFILT | sed 's/ . / /' > kernel.sym || die "failed nm kernel symbols"
		nm "$TEST_FILE" | $CPPFILT | sed 's/ . / /' >> kernel.sym || die "failed nm user symbols"
		# make header file for filenames and line numbers
		# TODO: kernel space line numbers for P3 testing... haha never(?)
		ADDRS_FILE=`mktemp user-code-addrs.XXXXXXXX.txt`
		LINES_FILE=line_numbers.h
		[ -f "$ADDRS_FILE" ] || die "failed make temp file for code addrs"
		rm -f "$LINES_FILE" || die "failed rm old line numbers header"
		# generates a file of lines each with e.g. "c002abcd"
		objdump -d "$TEST_FILE" | sed 's/^ *//' | grep '^\S*:' | sed 's/:.*//' | grep -v "$TEST_FILE" | sort > "$ADDRS_FILE" || die "failed make code addrs list"
		# generates a file of e.g. "{ .eip = 0xc002abcd, .filename = "c.c", .line = 42 },"
		# see symtable.c for the expected format / usage context
		cat "$ADDRS_FILE" | addr2line -e "$TEST_FILE" | sed 's@.*p2-basecode/@@' | sed 's/ (discriminator.*//' | sed 's/^??:/unknown:/' | sed 's/:?$/:0/' | sed 's/^/"/' | sed 's/:/", /' | sed 's/$/ },/' | paste "$ADDRS_FILE" - | sed 's/^/{ 0x/' | sed 's/\t/, /' > "$LINES_FILE" || die "failed generate line numbers header"
		# ".field =" initializer syntax not supported by PSU's stupid cluster machines which have no gcc younger than 4.4 but below is what it should be
		# cat "$ADDRS_FILE" | addr2line -e "$TEST_FILE" | sed 's@.*p2-basecode/@@' | sed 's/ (discriminator.*//' | sed 's/^??:/unknown:/' | sed 's/:?$/:0/' | sed 's/^/.filename = "/' | sed 's/:/", .line = /' | sed 's/$/ },/' | paste "$ADDRS_FILE" - | sed 's/^/{ .eip = 0x/' | sed 's/\t/, /' > "$LINES_FILE" || die "failed generate line numbers header"
		rm "$ADDRS_FILE" || msg "warning: couldnt remove temp file of code noobs"
		mv "$LINES_FILE" ../src/bochs-2.6.8/instrument/landslide/ || die "failed mv linenrs"
	fi
else
	msg "Header already generated; skipping."
fi

if [ ! -f $STUDENT ]; then
	die "$STUDENT doesn't seem to exist yet. Please implement it."
fi

cd ../src/bochs-2.6.8/ || die "couldnt cd bochs src dir"
msg "Compiling landslide..."
MAKE_OUTPUT=`make`
MAKE_RV=$?
[ "$MAKE_RV" = 0 ] || die "building landslide failed"
if echo "$MAKE_OUTPUT" | grep "g++" >/dev/null; then
	msg "landslide needed recompile; doing a make install"
	make install >/dev/null || die "installing landslide failed."
fi
success "Build succeeded."
