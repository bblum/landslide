/**
 * @file tree.h
 * @brief exploration tree tracking
 * @author Ben Blum
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

struct hax_child {
	int chosen_thread;
	bool all_explored;
};

/* Represents a single preemption point in the decision tree.
 * The data here stored actually reflects the state upon the *completion* of
 * that transition; i.e., when the next preemption has to be made. */
struct hax {
	/**** Basic info ****/

	unsigned int eip; /* The eip for the *next* preemption point. */
	unsigned long trigger_count; /* from ls_state */
	int chosen_thread; /* TID that was chosen to get here. -1 if root. */
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

	struct timetravel_hax time_machine;

	/**** Tree link data. ****/

	const struct hax *parent;
	unsigned int depth; /* starts at 0 */
	ARRAY_LIST(const struct hax_child) children;

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
 * so this lets you modify them in modify-hax callback's. (see timetravel.h) */
#define MUTABLE_FN(type, fieldname) \
	static inline type *mutable_##fieldname(struct hax *h)	\
		{ return (type *)h->fieldname; }
MUTABLE_FN(struct sched_state, oldsched)
MUTABLE_FN(struct test_state, oldtest)
MUTABLE_FN(struct stack_trace, stack_trace)
MUTABLE_FN(struct mem_state, old_kern_mem)
MUTABLE_FN(struct mem_state, old_user_mem)
MUTABLE_FN(struct user_sync_state, old_user_sync)
MUTABLE_FN(bool, conflicts)
MUTABLE_FN(bool, happens_before)
typedef ARRAY_LIST(struct hax_child) mutable_children_t;
static inline mutable_children_t *mutable_children(struct hax *h)
	{ return (mutable_children_t *)&h->children; }
typedef ARRAY_LIST(unsigned int) mutable_xabort_codes_t;
static inline mutable_xabort_codes_t *mutable_xabort_codes_todo(struct hax *h)
	{ return (mutable_xabort_codes_t *)&h->xabort_codes_todo; }
static inline mutable_xabort_codes_t *mutable_xabort_codes_ever(struct hax *h)
	{ return (mutable_xabort_codes_t *)&h->xabort_codes_ever; }
typedef Q_NEW_HEAD(struct, struct hax) mutable_hax_children_t;
static inline void set_happens_before(struct hax *h, unsigned int i, bool val)
	{ *(bool *)&(h->happens_before[i]) = val; }
static inline void set_conflicts(struct hax *h, unsigned int i, bool val)
	{ *(bool *)&(h->conflicts[i]) = val; }

#endif
