/**
 * @file save.h
 * @brief save/restore facility amidst the choice tree
 * @author Ben Blum
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

#ifndef __LS_SAVE_H
#define __LS_SAVE_H

#include "estimate.h"

#include <sys/time.h>

struct ls_state;
struct nobe;
struct abort_set;

struct save_statistics {
	uint64_t total_choices;
	uint64_t total_jumps;
	uint64_t total_triggers;
	uint64_t depth_total;

	/* Records the timestamp last time we arrived at a node in the tree.
	 * This is updated only during save_setjmp -- it doesn't need to be during
	 * save_longjmp because each longjmp is immediately after a call to setjmp
	 * on the last nobe in the previous branch. */
	struct timeval last_save_time;
	uint64_t total_usecs;
};

struct save_state {
	/* The root of the decision tree, or NULL if save_setjmp() was never
	 * called. */
	const struct nobe *root;
	/* If root is set, this points to the "current" node in the tree */
	const struct nobe *current;
	int next_tid;
	bool next_xabort;
	unsigned int next_xabort_code;
	/* Statistics */
	struct save_statistics stats;
};

void abort_transaction(unsigned int tid, const struct nobe *h2, unsigned int code);

void save_init(struct save_state *);

void save_recover(struct save_state *, struct ls_state *, int new_tid, bool xabort, unsigned int xabort_code);

/* Current state, and the next_tid/our_choice is about the next in-flight
 * choice. */
void save_setjmp(struct save_state *, struct ls_state *,
		 int next_tid, bool our_choice, bool end_of_test,
		 bool is_preemption_point, unsigned int data_race_eip,
		 bool voluntary, unsigned int joined_tid, bool xbegin,
		 bool prune_aborts, bool check_retry);

/* If nobe is NULL, then longjmps to the root. Otherwise, nobe must be between
 * the current choice point and the root (inclusive). */
void save_longjmp(struct save_state *, struct ls_state *, const struct nobe *,
		  unsigned int tid, bool txn, unsigned int xabort_code,
		  struct abort_set *aborts);

void save_reset_tree(struct save_state *ss, struct ls_state *ls);

#endif
