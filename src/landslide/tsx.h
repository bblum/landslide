/**
 * @file tsx.h
 * @brief abort set state space reduction optimization
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

#ifndef __LS_TSX_H
#define __LS_TSX_H

#include "array_list.h"
#include "common.h"
#include "student_specifics.h" /* for HTM_ABORT_SETS */

/* abort sets - experimental; not guaranteed to be a sound reduction...
 * not supported simultaneously with multiple xabort codes, as only RETRY is
 * outside the programmer's control, the other abort causes can depend on the
 * logic executed by the success path, which in turn can change if reordered
 * with other threads, so reordering always has to execute the success case
 * first. however, if retry aborts are issued and abort codes are not
 * distinguished, we can prune independent success and/or abort paths.
 * theoretically, it should be possible even with multiple abort codes, but
 * that sounds like a lot of implementation trouble for what i'd expect to be
 * very little reduction benefit (most abort paths that aren't just retry loops
 * tend to conflict with most others). */

#ifdef HTM_ABORT_SETS

struct abort_noob {
	/* may be TID_NONE to indicate this half of the set is not being used
	 * (i.e., this thread's conflicting transition was not a transaction);
	 * the codes list will be uninitialized (optional type) if so */
	unsigned int tid;
	/* determines which of the possible transactional outcomes we need to
	 * test or can prune; if the target tid's conflicting transition was not
	 * transactional to begin with, both of these will be set (to indicate
	 * no pruning); on the other hand, if dpor is not sure whether to prune
	 * (can arise from non-first-conflict tagging; see explore.c), there
	 * should not be an abort set to begin with, so at least one of these
	 * should be set if it exists. */
	bool check_success;
	bool check_retry;
};

/* equivalent of DPOR's sleep sets but for pruning independent xabort codes.
 * i am not sure how to deal with multiple simultaneously active abort sets
 * (involving disjoint threads, or otherwise), so i'm just gonna say if the
 * explorer tries to activate a second abort set in a sched that's already got
 * one, they all just cancel, and fall back to the non-reduced behaviour of
 * "all xbegin outcomes are fair game". */
struct abort_set {
	/* for example, say you've run T1 xabort-explicit -> T2 xabort-conflict;
	 * the abort set thereby generated will be T2 as subtree root and T1 as
	 * preempted ancestor (with respective abort codes in the list). if T2's
	 * success path is also found to conflict with T1's abort, also-succeed
	 * will also be set in the subtree child noob. */
	struct abort_noob reordered_subtree_child;
	struct abort_noob preempted_evil_ancestor;
};

#define ABORT_NOOB_ACTIVE(n)					\
	((n)->tid != TID_NONE && (n)->check_success != (n)->check_retry)
#define ABORT_NOOBS_EQ(n1,n2) ((n1)->tid == (n2)->tid &&	\
			       (n1)->check_success == (n2)->check_success && \
			       (n1)->check_retry == (n2)->check_retry)
#define ABORT_SETS_EQ(a1,a2)					\
	(ABORT_NOOBS_EQ(&(a1)->reordered_subtree_child,		\
			&(a2)->reordered_subtree_child) &&	\
	 ABORT_NOOBS_EQ(&(a1)->preempted_evil_ancestor,		\
			&(a2)->preempted_evil_ancestor))
#define ABORT_SET_ACTIVE(a) ((a) != NULL &&			\
	(ABORT_NOOB_ACTIVE(&(a)->reordered_subtree_child) ||	\
	 ABORT_NOOB_ACTIVE(&(a)->preempted_evil_ancestor)))
#define ABORT_SET_INIT_INACTIVE(a) do {				\
		(a)->reordered_subtree_child.tid = TID_NONE;	\
		(a)->preempted_evil_ancestor.tid = TID_NONE;	\
	} while (0)
#define ABORT_SET_BLOCKED(a, t)					\
	(ABORT_NOOB_ACTIVE(&(a)->reordered_subtree_child) &&	\
	 (a)->preempted_evil_ancestor.tid == (t))

static inline bool abort_noob_pop(struct abort_noob *n, bool *check_retry)
{
	assert(n->check_success || n->check_retry);
	if (n->check_success && n->check_retry) {
		/* abort set saturated; don't prune */
		return false;
	} else {
		*check_retry = n->check_retry;
		/* deactivate it to let the next thread go */
		n->tid = TID_NONE;
		return true;
	}
}

/* returns true if the abort set is active, the tid is in it, AND you're
 * supposed to prune one of the arms of xbegin outcomes (returns which one to
 * prune through the check-retry parameter) */
static inline bool abort_set_pop(struct abort_set *a, unsigned int tid,
				 bool *check_retry)
{
	if (a->reordered_subtree_child.tid == tid) {
		return abort_noob_pop(&a->reordered_subtree_child, check_retry);
	} else if (a->preempted_evil_ancestor.tid == tid) {
		assert(!ABORT_NOOB_ACTIVE(&a->reordered_subtree_child) &&
		       "oops tried to run an abort-blocked tid!");
		return abort_noob_pop(&a->preempted_evil_ancestor, check_retry);
	} else {
		return false;
	}
}

/* returns the one active abort set if the other is inactive;
 * if both are active returns null */
static struct abort_noob *only_active_noob(struct abort_set *a)
{
	assert(ABORT_SET_ACTIVE(a));
	if (!ABORT_NOOB_ACTIVE(&a->preempted_evil_ancestor)) {
		return &a->reordered_subtree_child;
	} else if (!ABORT_NOOB_ACTIVE(&a->reordered_subtree_child)) {
		return &a->preempted_evil_ancestor;
	} else {
		return NULL;
	}
}

static inline void print_abort_noob(verbosity v, const struct abort_noob *n)
{
	if (n->tid == TID_NONE) {
		printf(v, "none");
	} else {
		printf(v, "TID%d (%d,%d)", n->tid,
		       n->check_success, n->check_retry);
	}
}

static inline void print_abort_set(verbosity v, const struct abort_set *a)
{
	printf(v, "{");
	print_abort_noob(v, &a->reordered_subtree_child);
	printf(v, ", ");
	print_abort_noob(v, &a->preempted_evil_ancestor);
	printf(v, "}");
}

#else /* !HTM_ABORT_SETS */

struct abort_set { };

#define ABORT_SET_ACTIVE(a) false
#define ABORT_SET_INIT_INACTIVE(a) do { } while (0)
#define ABORT_SET_BLOCKED(a, t) false

static inline bool abort_set_pop(struct abort_set *a, unsigned int tid,
				 bool *check_retry) { return false; }

static inline void print_abort_set(verbosity v, const struct abort_set *a) { }

#endif

#endif
