/**
 * @file explore.c
 * @brief DPOR
 * @author Ben Blum <bblum@andrew.cmu.edu>
 */

#define MODULE_NAME "EXPLORE"
#define MODULE_COLOUR COLOUR_BLUE

#include "common.h"
#include "estimate.h"
#include "landslide.h"
#include "save.h"
#include "schedule.h"
#include "tree.h"
#include "tsx.h"
#include "user_sync.h"
#include "variable_queue.h"
#include "x86.h"

/******************************************************************************
 * dpor
 ******************************************************************************/

static bool is_child_searched(const struct hax *h, unsigned int child_tid) {
	const struct hax_child *child;
	const struct abort_set *aborts;
	unsigned int i;

#ifdef HTM_ABORT_SETS
	ARRAY_LIST_FOREACH(&h->abort_sets_todo, i, aborts) {
		if (aborts->reordered_subtree_child.tid == child_tid) {
			return false;
		}
	}
#endif
	ARRAY_LIST_FOREACH(&h->children, i, child) {
		if (child->chosen_thread == child_tid && !child->xabort &&
		    child->all_explored)
			return true;
	}
	return false;
}

static void branch_sanity(const struct hax *root, const struct hax *current)
{
	const struct hax *our_branch = NULL;
	const struct hax *rabbit = current;

	assert(root != NULL);
	assert(current != NULL);

	while (1) {
		assert(current->oldsched != NULL);
		/* our_branch chases, indicating where we came from */
		our_branch = current;
		if ((current = current->parent) == NULL) {
			assert(our_branch == root && "two roots?!?");
			return;
		}
		/* cycle check */
		if (rabbit) rabbit = rabbit->parent;
		if (rabbit) rabbit = rabbit->parent;
		assert(rabbit != current && "tree has cycle??");
	}
}

static bool is_evil_ancestor(const struct hax *h0, const struct hax *h)
{
	return !h0->happens_before[h->depth] && h0->conflicts[h->depth];
}

/* identifies redundant interleavings that can arise as described in issue #4.
 * h0 is a descendant transition that we're fixing to reorder around its
 * ancestor h. however, if in a sequence of transitions preceding h that happens
 * to be independent with h0, h0's chosen thread is found to have been explored
 * in a sibling branch to any of those transitions, reordering h0 around h
 * itself would be equivalent to that sibling branch, and may be pruned. */
static bool equiv_already_explored(const struct hax *h0, const struct hax *h)
{
	for (const struct hax *h2 = h->parent; h2 != NULL && h2->parent != NULL;
	     h2 = h2->parent) {
		if (h2->chosen_thread == h0->chosen_thread) {
			/* transitions by same thread are not independent obv */
			return false;
		} else if (is_evil_ancestor(h0, h2)) {
			/* independent preceding transition chain ends here */
			return false;
		} else if (h0->happens_before[h2->depth]) {
			/* h0 can't be reordered past here so stop looking */
			return false;
		} else if (is_child_searched(h2->parent, h0->chosen_thread)) {
			/* found such an independent sibling branch!! */
			return true;
		}
	}
	/* no evidence of an equivalent interleaving being tested was found */
	return false;
}

/******************************************************************************
 * abort set reduction
 ******************************************************************************/

/* don't want to make ipc message-passing etc at all if feature not enabled */
#ifdef HTM_ABORT_SETS

static bool check_aborts(const struct hax *h, struct abort_noob *noob)
{
	if (h->parent == NULL) {
		return false;
	} else if (h->parent->xbegin) {
		assert(h->chosen_thread == h->parent->chosen_thread);
		assert(!h->parent->xaborted);
		/* note how it's surrounded by tid check at the callsite */
		assert(noob->tid == TID_NONE || noob->tid == h->chosen_thread);
		/* supporting/ignoring other codes is future work; see tsx.h */
		assert((!h->xaborted || h->xabort_code == _XABORT_RETRY) &&
		       "abort set reduction works only w/retry xabort code");

		noob->tid = h->chosen_thread;
		noob->check_success = !(noob->check_retry = h->xaborted);
		return true;
	} else {
		return false;
	}
}

static void finalize_aborts(struct abort_noob *noob, unsigned int tid)
{
	if (noob->tid == TID_NONE) {
		/* conflicting transition was not a transaction, so we want to
		 * make sure the scheduler does not try to restrict outcomes of
		 * any transactions that thread may execute in the future (since
		 * *this* transition conflicted, subsequent transactions'
		 * behaviour may change depending thereupon) */
		noob->tid = tid;
		noob->check_success = true;
		noob->check_retry   = true;
	}
	assert(noob->tid == tid);
}

/* compares by reordered subtree child only */
static struct abort_set *find_abort_set(mutable_abort_sets_t *list,
					struct abort_set *src,
					bool subtree_noob_must_match)
{
	struct abort_set *dest;
	unsigned int i;
	ARRAY_LIST_FOREACH(list, i, dest) {
		assert(dest->reordered_subtree_child.tid != TID_NONE);
		if (subtree_noob_must_match) {
			if (ABORT_NOOBS_EQ(&dest->reordered_subtree_child,
					   &src->reordered_subtree_child)) {
				return dest;
			}
		} else {
			if (dest->reordered_subtree_child.tid ==
			    src->reordered_subtree_child.tid) {
				return dest;
			}
		}
	}
	return NULL;
}

static void merge_abort_noob(struct abort_noob *dest, const struct abort_noob *src)
{
	dest->check_success |= src->check_success;
	dest->check_retry   |= src->check_retry;
}

static void update_hax_abort_set(struct hax *h, struct abort_set *src)
{
	assert(src->reordered_subtree_child.tid != TID_NONE);
	assert(src->preempted_evil_ancestor.tid != TID_NONE);
	assert(!h->xbegin); /* should not be tagging tids at xbegin pps */

	struct abort_set *todo = find_abort_set(mutable_abort_sets_todo(h), src, true);
	if (todo != NULL) {
		/* found matching planned aborts for this thread; merge in
		 * whatever our new plans are for the preempted thread */
		if (src->preempted_evil_ancestor.tid ==
		    todo->preempted_evil_ancestor.tid) {
			struct abort_set *ever =
				find_abort_set(mutable_abort_sets_ever(h), src, true);
			assert(ever != NULL && "missing todo in ever");
			assert(ABORT_SETS_EQ(ever, todo));
			/* merge existing set with more stuff to test */
			merge_abort_noob(&ever->preempted_evil_ancestor,
					 &src->preempted_evil_ancestor);
			merge_abort_noob(&todo->preempted_evil_ancestor,
					 &src->preempted_evil_ancestor);
			return;
		} else {
			/* uh oh, a third thread! i'm not sure how to "merge"
			 * the intention to restrict txnal outcomes for multiple
			 * conflicting evil ancestors at once -- future work;
			 * for now, just skip to saturation step, below */
		}
	} else if ((todo = find_abort_set(mutable_abort_sets_todo(h),
					  src, false)) != NULL) {
		/* found different planned aborts; can maybe merge anyway? */
		if (todo->preempted_evil_ancestor.tid !=
		    src->preempted_evil_ancestor.tid) {
			/* third thread case, as above */
		} else if(!ABORT_NOOB_ACTIVE(&todo->preempted_evil_ancestor) &&
			  !ABORT_NOOB_ACTIVE(&src->preempted_evil_ancestor)) {
			/* special case of {(1,0),(1,1)} + {(0,1),(1,1)} can be
			 * merged into "just do everything" set; it's possible
			 * this may have already happened; if so, just a noop */
			struct abort_set *ever =
				find_abort_set(mutable_abort_sets_ever(h), src, false);
			assert(ever != NULL && "missing todo in ever (2)");
			assert(ABORT_SETS_EQ(ever, todo));
			assert(!ABORT_SETS_EQ(ever, src));
			merge_abort_noob(&todo->reordered_subtree_child,
					 &src->reordered_subtree_child);
			merge_abort_noob(&ever->reordered_subtree_child,
					 &src->reordered_subtree_child);
			/* this should saturate it */
			assert(!ABORT_SET_ACTIVE(todo));
			assert(!ABORT_SET_ACTIVE(ever));
			return;
		} else {
			/* unmergeable disjoint sets, for example,
			 * {(1,0),(0,1)} + {(0,1),(1,0)}, as in fig63.c, or
			 * {(1,0),(1,0)} + {(0,1),(1,0)}, which is tempting to
			 * merge into {(1,1),(1,0)}, but then you will get owned
			 * and hafta duplicate work when {(1,0),(0,1)} comes
			 * along, so coalescing only to saturation is safe */
		}
	}

	/* no todo set found; check if a similar one exists already explored;
	 * if identical, just skip; if a subset, saturate with the difference */
	const struct abort_set *old;
	unsigned int i;
	unsigned int saturate_tid = TID_NONE;
	bool needs_success = true;
	bool needs_retry = true;
	ARRAY_LIST_FOREACH(&h->abort_sets_ever, i, old) {
		if (ABORT_NOOBS_EQ(&old->reordered_subtree_child,
				   &src->reordered_subtree_child)) {
			/* should happen only in case of a 3rd thread */
			assert(old->preempted_evil_ancestor.tid !=
			       src->preempted_evil_ancestor.tid);
			assert(old->preempted_evil_ancestor.tid != TID_NONE);
			/* should not be 2 existing sets for diff tids */
			assert(saturate_tid == TID_NONE ||
			       saturate_tid == old->preempted_evil_ancestor.tid);
			saturate_tid = old->preempted_evil_ancestor.tid;
			/* tend towards false if possible to not duplicate */
			needs_success &= !old->preempted_evil_ancestor.check_success;
			needs_retry   &= !old->preempted_evil_ancestor.check_retry;
			/* at least one arm shoulda been tested previously */
			assert(!(needs_success && needs_retry));
		}
	}
	if (saturate_tid != TID_NONE) {
		/* hafta explore the other half of an existing one */
		src->preempted_evil_ancestor =
			{ .tid = saturate_tid,
			  .check_success = needs_success,
			  .check_retry = needs_retry, };
		/* (otherwise, never seen it before, add it afresh) */
	}
	/* can skip if was duplicate *and* already saturated */
	if (saturate_tid == TID_NONE || needs_success || needs_retry) {
		ARRAY_LIST_APPEND(mutable_abort_sets_ever(h), *src);
		ARRAY_LIST_APPEND(mutable_abort_sets_todo(h), *src);
	}
}

static void update_hax_pop_abort_set(struct hax *h, unsigned int *index)
{
	ARRAY_LIST_REMOVE_SWAP(mutable_abort_sets_todo(h), *index);
}

#endif

static void get_abort_set(const struct hax *h, unsigned int tid,
			  struct abort_set *dest)
{
#ifdef HTM_ABORT_SETS
	const struct abort_set *src;
	unsigned int i;
	ARRAY_LIST_FOREACH(&h->abort_sets_todo, i, src) {
		if (src->reordered_subtree_child.tid == tid) {
			*dest = *src;
			modify_hax(update_hax_pop_abort_set, h, i);
			return;
		}
	}
#endif
	/* no abort set for this tid found */
	ABORT_SET_INIT_INACTIVE(dest);
}

/******************************************************************************
 * tagging
 ******************************************************************************/

/* Finds the nearest parent save point that's actually a preemption point.
 * This is how we skip over speculative data race save points when identifying
 * which "good sibling" transitions to tag (when to preempt to get to them?) */
static const struct hax *pp_parent(const struct hax *h)
{
	do {
		h = h->parent;
		assert(h != NULL);
		/* If the root PP is a non-enabled speculative DR, we can get
		 * a bit stuck in a corner if we need to tag a child. FIXME:
		 * Really what we want is the oldest non-speculative ancestor,
		 * and to change the condition at the fixme in explore() to see
		 * if ancestor has no non-speculative parent. */
		assert(h->parent != NULL || h->is_preemption_point);
	} while (!h->is_preemption_point);
	/* see comment regarding just_delayed_for_xbegin in schedule.c -- xbegin
	 * PPs always come with a twin preceding them, which should be used
	 * instead for thread-related preemptions; the xbegin PP itself being
	 * reserved for failure injections only. */
	if (h->xbegin) {
		assert(h->parent->is_preemption_point);
		h = h->parent;
	}
	return h;
}

static void update_hax_tag_tid(struct hax *h, unsigned int *tid)
{
	struct agent *a;
	FOR_EACH_RUNNABLE_AGENT(a, mutable_oldsched(h),
		if (a->tid == *tid) {
			a->do_explore = true;
			return;
		}
	);
	assert(0 && "couldn't find tid in oldsched to tag");
}

static bool tag_good_sibling(const struct hax *h0, const struct hax *ancestor,
			     unsigned int icb_bound, bool *need_bpor,
			     struct abort_set *aborts)
{
	unsigned int tid = h0->chosen_thread;
	const struct hax *grandparent = pp_parent(ancestor);
	assert(need_bpor == NULL || !*need_bpor);

	const struct agent *a;
	CONST_FOR_EACH_RUNNABLE_AGENT(a, grandparent->oldsched,
		if (a->tid == tid) {
			if (BLOCKED(a) || HTM_BLOCKED(grandparent->oldsched, a) ||
			    ABORT_SET_BLOCKED(&grandparent->oldsched->upcoming_aborts, a->tid) ||
			    is_child_searched(grandparent, a->tid)) {
				return false;
			} else if (ICB_BLOCKED(grandparent->oldsched, icb_bound,
					       grandparent->voluntary, a)) {
				if (need_bpor != NULL) {
					*need_bpor = true;
					lsprintf(DEV, "from #%d/tid%d, want TID "
						 "%d, sibling of #%d/tid%d, but "
						 "ICB says no :(\n", h0->depth,
						 h0->chosen_thread, a->tid,
						 ancestor->depth,
						 ancestor->chosen_thread);
				}
				return false;
			} else {
				/* normal case; thread can be tagged */
				modify_hax(update_hax_tag_tid, grandparent, a->tid);
#ifdef HTM_ABORT_SETS
				/* probably safe to make this be ACTIVE()? */
				if (aborts != NULL) {
					lsprintf(DEV, "with the abort set ");
					print_abort_set(DEV, aborts);
					printf(DEV, ", the following tag...\n");
					/* dont send dat pointer!! */
					modify_hax(update_hax_abort_set,
						   grandparent, *aborts);
				}
#endif
				lsprintf(DEV, "from #%d/tid%d, tagged TID %d%s, "
					 "sibling of #%d/tid%d\n", h0->depth,
					 h0->chosen_thread, a->tid,
					 need_bpor == NULL ? " (during BPOR)" : "",
					 ancestor->depth, ancestor->chosen_thread);
				return true;
			}
		}
	);

	return false;
}

static void tag_all_siblings(const struct hax *h0, const struct hax *ancestor,
			     unsigned int icb_bound, bool *need_bpor)
{
	const struct hax *grandparent = pp_parent(ancestor);
	unsigned int num_tagged = 0;

	lsprintf(DEV, "from #%d/tid%d%s, tagged all siblings of #%d/tid%d: ",
		 h0->depth, h0->chosen_thread,
		 need_bpor == NULL ? " (during BPOR)" : "",
		 ancestor->depth, ancestor->chosen_thread);

	const struct agent *a;
	CONST_FOR_EACH_RUNNABLE_AGENT(a, grandparent->oldsched,
		if (BLOCKED(a) || HTM_BLOCKED(grandparent->oldsched, a) ||
		    ABORT_SET_BLOCKED(&grandparent->oldsched->upcoming_aborts, a->tid) ||
		    is_child_searched(grandparent, a->tid)) {
			// continue;
		} else if (ICB_BLOCKED(grandparent->oldsched, icb_bound,
				       grandparent->voluntary, a)) {
			/* also continue, but note that this sibling will
			 * require a BPOR backtrack (though not all ones might).
			 * this is a site for potential reduction -- if another
			 * not-icb-blocked sibling is tagged, and successfully
			 * leads to the desired thread reordering, that's enough
			 * to know to untag something from this case. */
			if (need_bpor != NULL) {
				*need_bpor = true;
				printf(DEV, "(tid%d needs BPOR) ", a->tid);
			}
		} else {
			/* normal case; sibling can be tagged */
			modify_hax(update_hax_tag_tid, grandparent, a->tid);
			print_agent(DEV, a);
			printf(DEV, " ");
			num_tagged++;
		}
	);

	printf(DEV, "\n");

	assert(need_bpor == NULL || !*need_bpor || num_tagged <= 2);
}

static bool tag_sibling(const struct hax *h0, const struct hax *ancestor,
			unsigned int icb_bound, struct abort_set *aborts)
{
	bool need_bpor = false;
	if (!tag_good_sibling(h0, ancestor, icb_bound, &need_bpor, aborts)) {
		/* FIXME: can't use abort sets when a 3rd thread is involved
		 * (future work?  should can at least do the ancestor, maybe?)
		 * is this sound, anyway? (should it make future dpors never
		 * prune, instead of leave undecided?) */
		tag_all_siblings(h0, ancestor, icb_bound, &need_bpor);
	}
	return need_bpor;
}

#ifdef ICB
static bool stop_bpor_backtracking(const struct hax *h0, const struct hax *ancestor2)
{
	/* Don't BPOR-tag past the previous transition of same thread... */
	if (h0->chosen_thread == ancestor2->chosen_thread) {
		return true;
	/* ...or past the point where the thread was created. */
	} else if (const_find_agent(ancestor2->oldsched, h0->chosen_thread) == NULL) {
		return true;
	} else {
		return false;
	}

}

/* BPOR [Coons et al, OOPSLA 2013]. */
static void tag_reachable_aunts(const struct hax *h0, const struct hax *ancestor,
				unsigned int icb_bound)
{
	bool good_sibling_tagged = false;
	assert(ancestor != NULL);

	lsprintf(DEV, "BPOR trying to reorder #%d/tid%d around E.A. #%d/tid%d\n",
		 h0->depth, h0->chosen_thread,
		 ancestor->depth, ancestor->chosen_thread);

	/* Search among ancestors for a place where we could switch to running
	 * h0's thread without exceeding the preemption bound. */
	for (const struct hax *ancestor2 = ancestor->parent;
	     ancestor2 != NULL && ancestor2->parent != NULL
	     && !stop_bpor_backtracking(h0, ancestor2);
	     ancestor2 = ancestor2->parent) {
		/* May need to tag multiple times for same reason as not
		 * using "break" in the main dpor loop below. */
		if (tag_good_sibling(h0, ancestor2, icb_bound, NULL, NULL)) {
			lsprintf(DEV, "BPOR can run #%d/tid%d after reachable "
				 "aunt #%d/tid%d\n", h0->depth, h0->chosen_thread,
				 ancestor2->depth, ancestor2->chosen_thread);
			good_sibling_tagged = true;
		}
	}

	if (good_sibling_tagged) {
		return;
	} else {
		lsprintf(DEV, "BPOR, ouch! lots of conservative tags incoming\n");
	}

	/* If got here, couldn't find a way to run h0's thread directly. But
	 * there may be another way to make it runnable using all_siblings
	 * which is reachable within the bound (either because the other sibling
	 * is runnable during a voluntary resched, or because the preemption
	 * count changes among these ancestors). */
	for (const struct hax *ancestor2 = ancestor->parent;
	     ancestor2 != NULL && ancestor2->parent != NULL
	     && !stop_bpor_backtracking(h0, ancestor2);
	     ancestor2 = ancestor2->parent) {
		tag_all_siblings(h0, ancestor2, icb_bound, NULL);
	}
}
#endif

static void update_hax_pop_xabort_code(struct hax *h, int *index)
{
	ARRAY_LIST_REMOVE_SWAP(mutable_xabort_codes_todo(h), *index);
}

static bool any_tagged_child(const struct hax *h, unsigned int *new_tid, bool *txn,
			     unsigned int *xabort_code, struct abort_set *aborts)
{
	const struct agent *a;

	if ((*txn = (h->xbegin && ARRAY_LIST_SIZE(&h->xabort_codes_todo) > 0))) {
		assert(ARRAY_LIST_SIZE(&h->abort_sets_ever) == 0);
		/* pop the code from the list so it won't be doubly explored */
		*xabort_code = *ARRAY_LIST_GET(&h->xabort_codes_todo, 0);
		modify_hax(update_hax_pop_xabort_code, h, 0);
		/* injecting a failure in the xbeginning thread, obvs. */
		*new_tid = h->chosen_thread;
		/* abort sets are used on thread pps, not xbegin pps */
		ABORT_SET_INIT_INACTIVE(aborts);
		return true;
	}

	/* do_explore doesn't get set on blocked threads, but might get set
	 * on threads we've already looked at. */
	CONST_FOR_EACH_RUNNABLE_AGENT(a, h->oldsched,
		if (a->do_explore && !is_child_searched(h, a->tid)) {
			*new_tid = a->tid;
			get_abort_set(h, a->tid, aborts);
			return true;
		}
	);

	return false;
}

/******************************************************************************
 * misc utilities
 ******************************************************************************/

static void print_pruned_children(struct save_state *ss, const struct hax *h)
{
	bool any_pruned = false;
	const struct agent *a;

	CONST_FOR_EACH_RUNNABLE_AGENT(a, h->oldsched,
		if (!is_child_searched(h, a->tid)) {
			if (!any_pruned) {
				lsprintf(DEV, "at #%d/tid%d pruned tids ",
					 h->depth, h->chosen_thread);
			}
			printf(DEV, "%d ", a->tid);
			any_pruned = true;
		}
	);
	if (any_pruned)
		printf(DEV, "\n");
}

static void update_hax_set_all_explored(struct hax *h, int *unused)
{
	h->all_explored = true;
}

static void update_hax_set_child_all_explored(struct hax *h, int chosen_thread,
					      bool xabort, unsigned int xabort_code)
{
	struct hax_child *child;
	unsigned int i;
	ARRAY_LIST_FOREACH(mutable_children(h), i, child) {
		if (child->chosen_thread == chosen_thread &&
		    child->xabort == xabort &&
		    (!xabort || child->xabort_code == xabort_code)) {
			child->all_explored = true;
			return;
		}
	}
	assert(0 && "child hax not found to mark all-explored");
}

static void update_hax_xabort_all_explored(struct hax *h, unsigned int *xabort_code)
	{ update_hax_set_child_all_explored(h, h->chosen_thread, true, *xabort_code); }
static void update_hax_preempt_all_explored(struct hax *h, int *chosen_thread)
	{ update_hax_set_child_all_explored(h, *chosen_thread, false, 0); }

static void set_all_explored(const struct hax *h)
{
	modify_hax(update_hax_set_all_explored, h, 0);
	if (h->parent != NULL) {
		if (h->xaborted) {
			assert(h->chosen_thread == h->parent->chosen_thread);
			modify_hax(update_hax_xabort_all_explored,
				   h->parent, h->xabort_code);
		} else {
			modify_hax(update_hax_preempt_all_explored,
				   h->parent, h->chosen_thread);
		}
	}
}

/******************************************************************************
 * main
 ******************************************************************************/

const struct hax *explore(struct ls_state *ls, unsigned int *new_tid, bool *txn,
			  unsigned int *xabort_code, struct abort_set *aborts)
{
	struct save_state *ss = &ls->save;
	const struct hax *current = ss->current;

	set_all_explored(current);
	branch_sanity(ss->root, ss->current);

	/* this cannot happen in-line with walking the branch, below, since it
	 * needs to be computed for all ancestors and be ready for checking
	 * against descendants in advance. */
	update_user_yield_blocked_transitions(current);

	/* Compare each transition along this branch against each of its
	 * ancestors. */
	for (const struct hax *h = current; h != NULL; h = h->parent) {
		/* remember whether there were any intervening transitions that
		 * also conflicted with this one; abort sets may only be used
		 * for reduction if no such transition exists */
		bool closest_conflict = true;
		/* then, find the earliest xbegin, if any, so we can make sure
		 * it returns the same value in the other future subtree */
		struct abort_set as;
		ABORT_SET_INIT_INACTIVE(&as);
#ifdef HTM_ABORT_SETS
		check_aborts(h, &as.reordered_subtree_child);
#endif
		/* In outer loop, we include user threads blocked in a yield
		 * loop as the "descendant" for comparison, because we want
		 * to reorder them before conflicting ancestors if needed... */
		for (const struct hax *ancestor = h->parent; ancestor != NULL;
		     ancestor = ancestor->parent) {
			if (ancestor->parent == NULL) {
				continue;
			/* ...however, in this inner loop, we never consider
			 * user yield-blocked threads as potential transitions
			 * to interleave around. */
			} else if (is_user_yield_blocked(ancestor)) {
				if (h->chosen_thread != ancestor->chosen_thread) {
					lsprintf(INFO, "not comparing #%d/tid%d "
						 "to #%d/tid%d; the latter is "
						 "yield-blocked\n", h->depth,
						 h->chosen_thread, ancestor->depth,
						 ancestor->chosen_thread);
				}
				continue;
			} else if (!is_evil_ancestor(h, ancestor)) {
#ifdef HTM_ABORT_SETS
				/* look for independent, intervening transaction
				 * preemption points by the same child tid */
				if (h->chosen_thread == ancestor->chosen_thread) {
					check_aborts(ancestor,
						     &as.reordered_subtree_child);
				}
#endif
				continue;
			} else if (equiv_already_explored(h, ancestor)) {
				/* not 100% sure on this but safer this way */
				closest_conflict = false;
				continue;
			}

#ifdef HTM_ABORT_SETS
			/* also is the evil ancestor itself transactional
			 * (note that if its abort path is split into multiple
			 * pps, we need to know to reproduce its failure code
			 * only when we're interleaving around the beginning
			 * thereof, so we don't need to check back before the
			 * ancestor for more xbegin pps by its same thread.) */
			if (closest_conflict) {
				/* account for non-enabled speculative pps
				 * (see pp-parent, etc, in tag-sibling */
				const struct hax *h2 = ancestor;
				do {
					assert(h2->chosen_thread ==
					       ancestor->chosen_thread);
					check_aborts(h2, &as.preempted_evil_ancestor);
					h2 = h2->parent;
				} while (h2 != NULL && !h2->is_preemption_point);
				/* if either child or ancestor weren't txnal,
				 * saturate their xabort set to prevent any
				 * pruning by future dpors at this pp */
				finalize_aborts(&as.reordered_subtree_child,
						h->chosen_thread);
				finalize_aborts(&as.preempted_evil_ancestor,
						ancestor->chosen_thread);
			}
#endif

			/* The ancestor is "evil". Find which siblings need to
			 * be explored. */
			/* An initialized yet empty abort set is 'strongest'
			 * (i.e., the top element, 'explore everything'); a null
			 * abort set is weakest (so that far-conflict tagging
			 * does not oops-override any existing constrained abort
			 * set to explore too much).. i THINK this is sound? */
			bool need_bpor = tag_sibling(h, ancestor, ls->icb_bound,
				closest_conflict ? &as : NULL);
#ifdef ICB
			if (need_bpor) {
				tag_reachable_aunts(h, ancestor, ls->icb_bound);
				ls->icb_need_increment_bound = true;
			}
#else
			assert(!need_bpor);
#endif

			/* In theory, stopping after the first baddie
			 * is fine; the others would be handled "by
			 * induction". But that relies on choice points
			 * being comprehensive enough, which we almost
			 * always do not satisfy. So continue. (See MS
			 * thesis section 5.4.3 / figure 5.4.) */
			/* break; */
			/* instead, just remember (for abort sets) */
			closest_conflict = false;
		}
	}

	/* We will choose a tagged sibling that's deepest, to maintain a
	 * depth-first ordering. This allows us to avoid having tagged siblings
	 * outside of the current branch of the tree. A trail of "all_explored"
	 * flags gets left behind. */
	for (const struct hax *h = current->parent; h != NULL; h = h->parent) {
		if (any_tagged_child(h, new_tid, txn, xabort_code, aborts)) {
			assert(h->is_preemption_point);
			lsprintf(BRANCH, "from #%d/tid%d, chose tid %d%s, "
				 "child of #%d/tid%d\n", current->depth,
				 current->chosen_thread, *new_tid,
				 *txn ? " (xbegin failure injection)" : "",
				 h->depth, h->chosen_thread);
			return h;
		} else {
			if (h->is_preemption_point) {
				lsprintf(DEV, "#%d/tid%d all_explored\n",
					 h->depth, h->chosen_thread);
				print_pruned_children(ss, h);
			} else {
				lsprintf(INFO, "#%d/tid%d not a PP\n",
					 h->depth, h->chosen_thread);
			}
			set_all_explored(h);
		}
	}

	lsprintf(ALWAYS, "found no tagged siblings on current branch!\n");
	return NULL;
}
