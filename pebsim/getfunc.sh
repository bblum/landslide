#!/bin/bash

# @file getfunc.sh
# @brief utility functions for extracting the start and end addresses of kernel or user functions
# @author Ben Blum

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

function success {
	echo -e "\033[01;32m$1\033[00m"
}
function msg {
	echo -e "\033[01;33m$1\033[00m" >&2
}
function err {
	echo -e "\033[01;31m$1\033[00m" >&2
}
OUTPUT_PIPE=
function die {
	err "$1"
	# If some part of the setup process fails before landslide can send the
	# thunderbirds message, we need to not leave the master hanging (literally).
	if [ ! -z "$OUTPUT_PIPE" ]; then
		echo -n > $OUTPUT_PIPE
	fi
	kill $$ # may be called in backticks; exit won't work
}

function _get_sym {
	objdump -t $2 | grep " $1$" | cut -d" " -f1
}

function _get_func {
	objdump -d $2 | grep "<$1>:" | cut -d" " -f1
}

# Gets the last instruction, spatially. Might not be ret or iret.
function _get_func_end {
	objdump -d $2 | grep -A10000 "<$1>:" | tail -n+2 | grep -m 1 -B10000 ^$ | grep -v ":.*90.*nop$" | tail -n 2 | head -n 1 | sed 's/ //g' | cut -d":" -f1
}

# Gets the last instruction, temporally. Must be ret or iret.
function _get_func_ret {
	RET_INSTR=`objdump -d $2 | grep -A10000 "<$1>:" | tail -n+2 | grep -m 1 -B10000 ^$ | grep '[^0-9abcdef]c3.*ret\|[^0-9abcdef]cf.*iret'`
	# Test for there being only one ret or iret - normal case.
	if [ "`echo "$RET_INSTR" | wc -l`" = "1" ]; then
		echo "$RET_INSTR" | sed 's/ //g' | cut -d":" -f1
	else
		err "!!!"
		err "!!! Function $1 has multiple end-points."
		err "!!! You will need to hand-write multiple copies of the above macro by hand for"
		err "!!! each one, and edit the corresponding function in kernel_specifics.c to test"
		err "!!! for all of them."
		err "!!! PLEASE ASK BEN FOR HELP."
		die "!!!"
	fi
}

function get_sym {
	RESULT=`_get_sym $1 $KERNEL_IMG`
	if [ -z "$RESULT" ]; then
		die "Couldn't find symbol $1."
	fi
	echo $RESULT
}
function get_func {
	RESULT=`_get_func $1 $KERNEL_IMG`
	if [ -z "$RESULT" ]; then
		die "Couldn't find function $1."
	fi
	echo $RESULT
}
function get_func_end {
	RESULT=`_get_func_end $1 $KERNEL_IMG`
	if [ -z "$RESULT" ]; then
		die "Couldn't find end-of-function $1."
	fi
	echo $RESULT
}
function get_func_ret {
	RESULT=`_get_func_ret $1 $KERNEL_IMG`
	if [ -z "$RESULT" ]; then
		die "Couldn't find ret-of-function $1."
	fi
	echo $RESULT
}

function get_test_file {
	if [ ! -z "$PINTOS_KERNEL" ]; then
		if [ -z "$PINTOS_USERPROG" ]; then
			die "PINTOS_KERNEL is set but PINTOS_USERPROG was not"
		elif [ "$PINTOS_USERPROG" = "0" ]; then
			echo "/dev/null"
		else
			echo "$KERNEL_SOURCE_DIR/userprog/build/tests/userprog/$TEST_CASE"
		fi
	elif [ -f $KERNEL_SOURCE_DIR/user/progs/$TEST_CASE ]; then
		echo "$KERNEL_SOURCE_DIR/user/progs/$TEST_CASE"
	elif [ -f $KERNEL_SOURCE_DIR/410user/progs/$TEST_CASE ]; then
		echo "$KERNEL_SOURCE_DIR/410user/progs/$TEST_CASE"
	else
		die "couldn't find $TEST_CASE in $KERNEL_SOURCE_DIR/{410,}user/progs!"
	fi
}

# As above functions but objdumps the userspace program binary instead.
# However, might emit the empty string if not present.
function get_user_sym {
	TF=`get_test_file`
	if [ -f "$TF" -a "$TF" != "/dev/null" ]; then
		_get_sym $1 $TF
	fi
}
function get_user_func {
	TF=`get_test_file`
	if [ -f "$TF" -a "$TF" != "/dev/null" ]; then
		_get_func $1 $TF
	fi
}
function get_user_func_end {
	TF=`get_test_file`
	if [ -f "$TF" -a "$TF" != "/dev/null" ]; then
		_get_func_end $1 $TF
	fi
}
function get_user_func_ret {
	TF=`get_test_file`
	if [ -f "$TF" -a "$TF" != "/dev/null" ]; then
		_get_func_ret $1 $TF
	fi
}

