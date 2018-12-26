/**
 * @file common.h
 * @brief things common to all parts of landslide
 * @author Ben Blum <bblum@andrew.cmu.edu>
 * Copyright (c) 2018, Ben Blum
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __LS_COMMON_H
#define __LS_COMMON_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef BOCHS
#include "bochs.h" // XXX: need to include this before undefining "assert" below.
#endif

#include "student_specifics.h" // for verbosity

#ifndef MODULE_NAME
#error "Please define MODULE_NAME before including common.h!"
#endif

#ifndef MODULE_COLOUR
#define MODULE_COLOUR COLOUR_WHITE
#endif

#define COLOUR_BOLD "\033[01m"
#define COLOUR_DARK "\033[00m"
#define COLOUR_RED "\033[31m"
#define COLOUR_GREEN "\033[32m"
#define COLOUR_YELLOW "\033[33m"
#define COLOUR_BLUE "\033[34m"
#define COLOUR_MAGENTA "\033[35m"
#define COLOUR_CYAN "\033[36m"
#define COLOUR_GREY "\033[37m"
#define COLOUR_WHITE "\033[38m"
#define COLOUR_DEFAULT "\033[00m"

#define BUF_SIZE 256 /* default length for internal print buffers */

/* "optional" types for integers */
#define TID_NONE ((unsigned int)-1)
#define ADDR_NONE ((unsigned int)-1)

/* Verbosity levels */
#define ALWAYS 0 /* yeah, uh */
#define BUG    ALWAYS /* stuff that must get printed to be ever useful */
#define BRANCH 1 /* once per branch - try not to have too many of these */
#define CHOICE 2 /* once per transition - really try to cut back on these */
#define DEV    3 /* messages only useful for development */
#define INFO   4 /* if you wouldn't put it anywhere but twitter */

#if VERBOSE == 0 && EXTRA_VERBOSE == 0
#define MAX_VERBOSITY CHOICE // default
#elif EXTRA_VERBOSE != 0
#define MAX_VERBOSITY INFO // EXTRA_VERBOSE overrides VERBOSE
#else
#ifdef ID_WRAPPER_MAGIC
#define MAX_VERBOSITY BRANCH
#else
#define MAX_VERBOSITY DEV
#endif
#endif

typedef int verbosity;

// Can be called directly if you want to override the module name and colour.
#define _lsprintf(v, mn, mc, ...) do { if ((v) <= MAX_VERBOSITY) {	\
	fprintf(stderr, "\033[80D" COLOUR_BOLD mc "[" mn "]              " \
		COLOUR_DEFAULT "\033[80D\033[16C" __VA_ARGS__);		\
	} } while (0)

#define lsprintf(v, ...) _lsprintf((v), MODULE_NAME, MODULE_COLOUR, __VA_ARGS__)

#ifdef printf
#undef printf
#endif
#define printf(v, ...) do { if ((v) <= MAX_VERBOSITY) {	\
		fprintf(stderr, __VA_ARGS__);					\
	} } while (0)

/* Specialized prints that are only emitted when in kernel- or user-space */
bool testing_userspace();
#define lsuprintf(...) \
	do { if ( testing_userspace()) { lsprintf(__VA_ARGS__); } } while (0)
#define lskprintf(...) \
	do { if (!testing_userspace()) { lsprintf(__VA_ARGS__); } } while (0)
#define uprintf(...) \
	do { if ( testing_userspace()) {   printf(__VA_ARGS__); } } while (0)
#define kprintf(...) \
	do { if (!testing_userspace()) {   printf(__VA_ARGS__); } } while (0)

/* Custom assert */
#ifdef assert
#undef assert
#endif

extern void landslide_assert_fail(const char *message, const char *file,
				  unsigned int line, const char *function);
#define assert(expr) do { if (!(expr)) {				\
		landslide_assert_fail(__STRING(expr), __FILE__,		\
				      __LINE__, __ASSERT_FUNCTION);	\
	} } while (0)

/* you couldn't make this shit up if you tried */
/* note to self: snprintf will at least *ALWAYS* terminate with \0. */
#define scnprintf(buf, maxlen, ...) ({								\
	int __snprintf_ret = snprintf((buf), (maxlen), __VA_ARGS__);	\
	if (__snprintf_ret > (maxlen)) { __snprintf_ret = (maxlen); }	\
	__snprintf_ret; })

#define HURDLE_VIOLATION(msg) do { hurdle_violation(msg); assert(0); } while (0)
static inline void hurdle_violation(const char *msg) {
	lsprintf(ALWAYS, COLOUR_BOLD COLOUR_RED
		 "/!\\ /!\\ /!\\ HURDLE VIOLATION /!\\ /!\\ /!\\\n");
	lsprintf(ALWAYS, COLOUR_BOLD COLOUR_RED "%s\n", msg);
	lsprintf(ALWAYS, COLOUR_BOLD COLOUR_RED "Landslide can probably cope "
		 " with this, but instead of continuing to use Landslide, please"
		 " go fix your kernel.\n" COLOUR_DEFAULT);
}

#endif
