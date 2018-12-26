/**
 * @file found_a_bug.h
 * @brief function for dumping debug info and quitting simics when finding a bug
 * @author Ben Blum <bblum@andrew.cmu.edu>
 *
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

#ifndef __LS_FOUND_A_BUG_H
#define __LS_FOUND_A_BUG_H

#ifndef __LS_COMMON_H
#error "must include common.h before found_a_bug.h"
#endif

#include <inttypes.h> /* for PRIu64 */
#include <unistd.h> /* for write */

#include "landslide.h"
#include "stack.h"

struct ls_state;

#define _PRINT_TREE_INFO(v, mn, mc, ls) do {				\
	_lsprintf(v, mn, mc, "Current instruction count %" PRIu64 ", "	\
		  "total instructions %" PRIu64 "\n", 			\
		  ls->trigger_count, ls->absolute_trigger_count);	\
	_lsprintf(v, mn, mc, "Total preemption-points %" PRIu64 ", "	\
		  "total backtracks %" PRIu64 "\n", 			\
		  ls->save.stats.total_choices,				\
		  ls->save.stats.total_jumps);				\
	_lsprintf(v, mn, mc,						\
		  "Average instrs/preemption-point %lu, "		\
		  "average branch depth %lu\n",				\
		  ls->save.stats.total_triggers /			\
		  (1+ls->save.stats.total_choices),			\
		  ls->save.stats.depth_total / 				\
		  (1+ls->save.stats.total_jumps));			\
	} while (0)

#define PRINT_TREE_INFO(v, ls) \
	_PRINT_TREE_INFO(v, MODULE_NAME, MODULE_COLOUR, ls)

/* gross C glue for enabling the third-order-function-style macro below. */
struct fab_html_env {
	int html_fd;
};
typedef void (*fab_cb_t)(void *env, struct fab_html_env *html_env);

void _found_a_bug(struct ls_state *, bool bug_found, bool verbose,
		  const char *reason, unsigned int reason_len,
		  void *callback_env, fab_cb_t callback);

#define DUMP_DECISION_INFO(ls) \
	_found_a_bug(ls, false, true,  NULL, 0, NULL, NULL) // Verbose
#define DUMP_DECISION_INFO_QUIET(ls) \
	_found_a_bug(ls, false, false, NULL, 0, NULL, NULL) // Not

/* Simple common interface. */
#define FOUND_A_BUG(ls, ...) do { 						\
		char __fab_buf[1024];						\
		int __fab_len = scnprintf(__fab_buf, 1024, __VA_ARGS__);	\
		_found_a_bug(ls, true, false, __fab_buf, __fab_len, NULL, NULL);\
	} while (0)

// FIXME: Find a clean way to move this stuff to html.h
#define HTML_BUF_LEN 4096
#define HTML_PRINTF(env, ...) do {						\
		char __html_buf[HTML_BUF_LEN];					\
		unsigned int __len =						\
			scnprintf(__html_buf, HTML_BUF_LEN, __VA_ARGS__);	\
		assert(__len > 0 && "failed scnprintf");			\
		unsigned int __ret = write((env)->html_fd, __html_buf, __len);	\
		assert(__ret > 0 && "failed write");				\
	} while(0)
#define HTML_PRINT_STACK_TRACE(env, st) do {					\
		char __html_buf[HTML_BUF_LEN];					\
		unsigned int __len =						\
			html_stack_trace(__html_buf, HTML_BUF_LEN, st);		\
		unsigned int __ret = write((env)->html_fd, __html_buf, __len);	\
		assert(__ret > 0 && "failed write");				\
	} while (0)

/* 3rd-order-style function for getting extra info into html trace output. */
#define FOUND_A_BUG_HTML_INFO(ls, reason, reason_len, env, cb) \
	_found_a_bug(ls, true, false, reason, reason_len, env, cb)

#endif
