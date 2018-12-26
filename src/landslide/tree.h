/**
 * @file tree.h
 * @brief exploration tree tracking
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

#ifndef __LS_TREE_H
#define __LS_TREE_H

#include <stdbool.h>

#include "array_list.h"
#include "simulator.h"
#include "timetravel.h"
#include "variable_queue.h"

struct ls_state;
struct mem_state;
struct sched_state;
struct stack_trace;
struct test_state;
struct abort_set;

struct nobe_child {
	int chosen_thread;
	bool all_explored;
	/* if xabort, then (h->)child->chosen_thread == h->chosen_thread */
	bool xabort;
	unsigned int xabort_code;
};

/* Represents a single preemption point in the decision tree.
 * The data here stored actually reflects the state upon the *completion* of
 * that transition; i.e., when the next preemption has to be made. */
struct nobe {
	/**** Basic info ****/

	unsigned int eip; /* The eip for the *next* preemption point. */
	unsigned long trigger_count; /* from ls_state */
	int chosen_thread; /* TID that was chosen to get here. -1 if root. */
	bool xaborted;
	unsigned int xabort_code;
	const struct stack_trace *stack_trace;

	/***** Saved state from the past. The state struct pointers are
	 * non-NULL if it's a point directly backwards from where we are. *****/

	const struct sched_state *oldsched;
	const struct test_state *oldtest;
	const struct mem_state *old_kern_mem;
	const struct mem_state *old_user_mem;
	const struct user_sync_state *old_user_sync;
	const symtable_t *old_symtable;
	/* List of things that are *not* saved/restored (i.e., glowing green):
	 *  - arbiter_state (just a preemption history queue, maintained internally)
	 *  - ls_state's absolute_trigger_count (obv.)
	 *  - save_state (duh)
	 *  - data_races
	 */

	struct timetravel_pp time_machine;

	/**** Tree link data. ****/

	const struct nobe *parent;
	unsigned int depth; /* starts at 0 */
	ARRAY_LIST(const struct nobe_child) children;

	/**** DPOR state ****/

	/* Other transitions (ancestors) that conflict with or happen-before
	 * this one. The length of each array is given by 'depth'. */
	const bool *conflicts;      /* if true, then they aren't independent. */
	const bool *happens_before; /* "happens_after", really. */

	/* All branches of the subtree rooted here executed already? */
	bool all_explored;
	/* Despite setting a bookmark here, we may intend this not to be a real
	 * preemption point. It may be speculative, looking for a data race. */
	bool is_preemption_point;
	/* Optional value that may be set if this is a speculative DR PP.
	 * Indicates the suspected DR eip value in the upcoming transition. */
	unsigned int data_race_eip;
	/* Was the arbiter interested in this instruction because it was the end
	 * of a voluntary reschedule? (Used for ICB preemption counting.) */
	bool voluntary;
	/* Does this preemption point denote the start of a transaction?
	 * (If so, then a failure will need to be injected, not just timers.) */
	bool xbegin;
	ARRAY_LIST(const unsigned int) xabort_codes_ever; /* append-only */
	ARRAY_LIST(const unsigned int) xabort_codes_todo; /* serves as workqueue */
	/* Is this is a pre-xbegin preemption point which we wanna reschedule
	 * for later and constrain what codes are injected? And/or are we trying
	 * to reschedule an xbegin transition at this preemption point and
	 * constrain that likewise? (Note this should never be set at xbegin
	 * PPs themselves, only at pre-xbegin PPs, or even normal ones.)
	 * If the arbiter ever tries to have sched inject an xbegin result
	 * not on this list, it will prune the entire resulting subtree. */
	ARRAY_LIST(const struct abort_set) abort_sets_ever;
	ARRAY_LIST(const struct abort_set) abort_sets_todo;

	/* Note: a list of available tids to run next is implicit in the copied
	 * sched! Also, the "tags" that POR uses to denote to-be-explored
	 * siblings are in the agent structs on the scheduler queues. */

	/**** Estimation state ****/

	/* how many children of this are marked (already explored or tagged for
	 * later exploration). this represents the value as it was at the
	 * completion of the last branch, and is updated at estimation time. */
	unsigned long marked_children;
	/* the estimated proportion of the tree that the branches in this node's
	 * subtree represent (NOT counting tagged but unexplored children) */
	long double proportion;
	/* how much time this transition took to execute (from its parent) */
	uint64_t usecs;
	/* cumulative sum of the usecs/branches in this nobe's subtree,
	 * including estimated time of unexplored children subtrees. because
	 * we use reverse execution rather than record/replay, this estimate
	 * does not double-count time for transitions with multiple children.
	 * this value is computed as: subtree_usecs = (Sum{children} child
	 * usecs + child subtree_usecs) * marked children / explored children,
	 * where child subtree_usecs is the same value computed recursively. */
	long double subtree_usecs;
	/* was an estimate already computed with this nobe as the leaf? used to
	 * avoid recomputing (causing double-counting) and to ensure non-leaf
	 * nodes cannot be used as the source of an estimate. */
	bool estimate_computed;
};

/* c's type system sucks too much to propagate const through data structures,
 * so this lets you modify them in modify-nobe callback's. (see timetravel.h) */
#define MUTABLE_FN(type, fieldname) \
	static inline type *mutable_##fieldname(struct nobe *h)	\
		{ return (type *)h->fieldname; }
MUTABLE_FN(struct sched_state, oldsched)
MUTABLE_FN(struct test_state, oldtest)
MUTABLE_FN(struct stack_trace, stack_trace)
MUTABLE_FN(struct mem_state, old_kern_mem)
MUTABLE_FN(struct mem_state, old_user_mem)
MUTABLE_FN(struct user_sync_state, old_user_sync)
MUTABLE_FN(bool, conflicts)
MUTABLE_FN(bool, happens_before)
typedef ARRAY_LIST(struct nobe_child) mutable_children_t;
static inline mutable_children_t *mutable_children(struct nobe *h)
	{ return (mutable_children_t *)&h->children; }
typedef ARRAY_LIST(unsigned int) mutable_xabort_codes_t;
static inline mutable_xabort_codes_t *mutable_xabort_codes_todo(struct nobe *h)
	{ return (mutable_xabort_codes_t *)&h->xabort_codes_todo; }
static inline mutable_xabort_codes_t *mutable_xabort_codes_ever(struct nobe *h)
	{ return (mutable_xabort_codes_t *)&h->xabort_codes_ever; }
typedef ARRAY_LIST(struct abort_set) mutable_abort_sets_t;
static inline mutable_abort_sets_t *mutable_abort_sets_ever(struct nobe *h)
	{ return (mutable_abort_sets_t *)&h->abort_sets_ever; }
static inline mutable_abort_sets_t *mutable_abort_sets_todo(struct nobe *h)
	{ return (mutable_abort_sets_t *)&h->abort_sets_todo; }
typedef Q_NEW_HEAD(struct, struct nobe) mutable_pp_children_t;
static inline void set_happens_before(struct nobe *h, unsigned int i, bool val)
	{ *(bool *)&(h->happens_before[i]) = val; }
static inline void set_conflicts(struct nobe *h, unsigned int i, bool val)
	{ *(bool *)&(h->conflicts[i]) = val; }

#endif
