/**
 * @file bug.c
 * @brief remembering which bugs have been found under which pp configs
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

#define _XOPEN_SOURCE 700

#include <pthread.h>

#include "array_list.h"
#include "common.h"
#include "job.h"
#include "pp.h"
#include "sync.h"
#include "xcalls.h"

struct bug_info {
	char *trace_filename;
	struct pp_set *config;
	char *log_filename;
};

static bool fab_inited = false;
static ARRAY_LIST(struct bug_info) fab_list;
static pthread_mutex_t fab_lock = PTHREAD_MUTEX_INITIALIZER;

static void check_init()
{
	if (!fab_inited) {
		LOCK(&fab_lock);
		if (!fab_inited) {
			ARRAY_LIST_INIT(&fab_list, 16);
			fab_inited = true;
		}
		UNLOCK(&fab_lock);
	}
}

void found_a_bug(char *trace_filename, struct job *j)
{
	struct bug_info b;
	b.trace_filename = XSTRDUP(trace_filename);
	b.config = clone_pp_set(j->config);
	b.log_filename = XSTRDUP(j->log_stderr.filename);

	check_init();

	LOCK(&fab_lock);
	ARRAY_LIST_APPEND(&fab_list, b);
	UNLOCK(&fab_lock);
}

/* Did a prior job with a subset of the given PPs already find a bug? */
bool bug_already_found(struct pp_set *config)
{
	unsigned int i;
	struct bug_info *b;
	bool result = false;

	check_init();

	LOCK(&fab_lock);
	ARRAY_LIST_FOREACH(&fab_list, i, b) {
		if (pp_subset(b->config, config)) {
			result = true;
			break;
		}
	}
	UNLOCK(&fab_lock);

	return result;
}

bool found_any_bugs()
{
	unsigned int i;
	struct bug_info *b;
	bool any = false;
	check_init();

	LOCK(&fab_lock);
	ARRAY_LIST_FOREACH(&fab_list, i, b) {
		printf(COLOUR_BOLD COLOUR_RED
		       "Found a bug - %s - with PPs: ", b->trace_filename);
		print_pp_set(b->config, true);
		// FIXME: do something better than hardcode print "id/"
		printf(" (log file: id/%s)\n" COLOUR_DEFAULT, b->log_filename);
		any = true;
	}
	UNLOCK(&fab_lock);

	if (!any) {
		printf(COLOUR_BOLD COLOUR_GREEN
		       "No bugs were found -- you survived!\n" COLOUR_DEFAULT);
	}

	return any;
}
