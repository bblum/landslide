/**
 * @file save.c
 * @brief choice tree tracking, incl. save/restore
 * @author Ben Blum
 */

#include <inttypes.h>
#include <string.h> /* for memcmp, strlen */

#define MODULE_NAME "SAVE"
#define MODULE_COLOUR COLOUR_MAGENTA

#include "arbiter.h"
#include "common.h"
#include "compiler.h"
#include "estimate.h"
#include "found_a_bug.h"
#include "landslide.h"
#include "lockset.h"
#include "mem.h"
#include "save.h"
#include "schedule.h"
#include "stack.h"
#include "symtable.h"
#include "test.h"
#include "timetravel.h"
#include "tree.h"
#include "tsx.h"
#include "user_sync.h"
#include "vector_clock.h"
#include "x86.h"

/******************************************************************************
 * helpers
 ******************************************************************************/

static void copy_user_yield_state(struct user_yield_state *dest,
				  struct user_yield_state *src)
{
	dest->loop_count = src->loop_count;
	dest->blocked    = src->blocked;
}

static void copy_malloc_actions(struct malloc_actions *dest, const struct malloc_actions *src)
{
	dest->in_alloc            = src->in_alloc;
	dest->in_realloc          = src->in_realloc;
	dest->in_free             = src->in_free;
	dest->alloc_request_size  = src->alloc_request_size;
	dest->in_page_alloc       = src->in_page_alloc;
	dest->in_page_free        = src->in_page_free;
	dest->palloc_request_size = src->palloc_request_size;
}

#define COPY_FIELD(name) do { a_dest->name = a_src->name; } while (0)
static struct agent *copy_agent(struct agent *a_src)
{
	struct agent *a_dest = MM_XMALLOC(1, struct agent);
	assert(a_src != NULL && "cannot copy null agent");

	COPY_FIELD(tid);

	COPY_FIELD(action.handling_timer);
	COPY_FIELD(action.context_switch);
	COPY_FIELD(action.cs_free_pass);
	COPY_FIELD(action.forking);
	COPY_FIELD(action.sleeping);
	COPY_FIELD(action.spleeping);
	COPY_FIELD(action.vanishing);
	COPY_FIELD(action.readlining);
	COPY_FIELD(action.just_forked);
	COPY_FIELD(action.lmm_init);
	COPY_FIELD(action.vm_user_copy);
	COPY_FIELD(action.disk_io);
	COPY_FIELD(action.kern_mutex_locking);
	COPY_FIELD(action.kern_mutex_unlocking);
	COPY_FIELD(action.kern_mutex_trylocking);
	COPY_FIELD(action.user_mutex_initing);
	COPY_FIELD(action.user_mutex_locking);
	COPY_FIELD(action.user_mutex_unlocking);
	COPY_FIELD(action.user_mutex_yielding);
	COPY_FIELD(action.user_mutex_destroying);
	COPY_FIELD(action.user_cond_waiting);
	COPY_FIELD(action.user_cond_signalling);
	COPY_FIELD(action.user_cond_broadcasting);
	COPY_FIELD(action.user_sem_proberen);
	COPY_FIELD(action.user_sem_verhogen);
	COPY_FIELD(action.user_rwlock_locking);
	COPY_FIELD(action.user_rwlock_unlocking);
	COPY_FIELD(action.user_locked_mallocing);
	COPY_FIELD(action.user_locked_callocing);
	COPY_FIELD(action.user_locked_reallocing);
	COPY_FIELD(action.user_locked_freeing);
	COPY_FIELD(action.user_mem_sbrking);
	COPY_FIELD(action.user_wants_txn);
	COPY_FIELD(action.user_txn);
	COPY_FIELD(action.schedule_target);
	assert(memcmp(&a_dest->action, &a_src->action, sizeof(a_dest->action)) == 0 &&
	       "Did you update agent->action without updating save.c?");

	a_dest->kern_blocked_on = NULL; /* Will be recomputed later if needed */
	COPY_FIELD(kern_blocked_on_tid);
	COPY_FIELD(kern_blocked_on_addr);
	COPY_FIELD(kern_mutex_unlocking_addr);
	COPY_FIELD(user_blocked_on_addr);
	COPY_FIELD(user_mutex_recently_unblocked);
	COPY_FIELD(user_mutex_initing_addr);
	COPY_FIELD(user_mutex_locking_addr);
	COPY_FIELD(user_mutex_unlocking_addr);
	COPY_FIELD(user_rwlock_locking_addr);
	COPY_FIELD(user_joined_on_tid);
	COPY_FIELD(last_pf_eip);
	COPY_FIELD(last_pf_cr2);
	COPY_FIELD(just_delayed_for_data_race);
	COPY_FIELD(delayed_data_race_eip);
#ifdef PREEMPT_EVERYWHERE
	COPY_FIELD(preempt_for_shm_here);
#endif
	COPY_FIELD(just_delayed_for_vr_exit);
	COPY_FIELD(delayed_vr_exit_eip);
	COPY_FIELD(just_delayed_for_xbegin);
	COPY_FIELD(delayed_xbegin_eip);
	COPY_FIELD(most_recent_syscall);
	COPY_FIELD(last_call);
	lockset_clone(&a_dest->kern_locks_held, &a_src->kern_locks_held);
	lockset_clone(&a_dest->user_locks_held, &a_src->user_locks_held);
#ifdef PURE_HAPPENS_BEFORE
	vc_copy(&a_dest->clock, &a_src->clock);
#endif
	copy_user_yield_state(&a_dest->user_yield, &a_src->user_yield);
#ifdef ALLOW_REENTRANT_MALLOC_FREE
	copy_malloc_actions(&a_dest->kern_malloc_flags, &a_src->kern_malloc_flags);
	copy_malloc_actions(&a_dest->user_malloc_flags, &a_src->user_malloc_flags);
#endif
	a_dest->pre_vanish_trace = (a_src->pre_vanish_trace == NULL) ?
		NULL : copy_stack_trace(a_src->pre_vanish_trace);

	a_dest->do_explore = false;

	return a_dest;
}
#undef COPY_FIELD

/* Updates the cur_agent and schedule_in_flight pointers upon finding the
 * corresponding agence in the s_src. */
static void copy_sched_q(struct agent_q *q_dest, const struct agent_q *q_src,
			 struct sched_state *dest,
			 const struct sched_state *src)
{
	struct agent *a_src;

	assert(Q_GET_SIZE(q_dest) == 0);

	Q_FOREACH(a_src, q_src, nobe) {
		struct agent *a_dest = copy_agent(a_src);

		// XXX: Q_INSERT_TAIL causes an assert to trip. ???
		Q_INSERT_HEAD(q_dest, a_dest, nobe);
		if (src->cur_agent == a_src)
			dest->cur_agent = a_dest;
		if (src->last_agent != NULL && src->last_agent == a_src)
			dest->last_agent = a_dest;
		if (src->schedule_in_flight == a_src)
			dest->schedule_in_flight = a_dest;
	}
}
static void copy_sched(struct sched_state *dest, const struct sched_state *src)
{
	dest->cur_agent           = NULL;
	dest->last_agent          = NULL;
	dest->last_vanished_agent = NULL;
	dest->schedule_in_flight  = NULL;
	Q_INIT_HEAD(&dest->rq);
	Q_INIT_HEAD(&dest->dq);
	Q_INIT_HEAD(&dest->sq);
	copy_sched_q(&dest->rq, &src->rq, dest, src);
	copy_sched_q(&dest->dq, &src->dq, dest, src);
	copy_sched_q(&dest->sq, &src->sq, dest, src);
	assert((src->cur_agent == NULL || dest->cur_agent != NULL) &&
	       "copy_sched couldn't set cur_agent!");
	assert((src->schedule_in_flight == NULL ||
	        dest->schedule_in_flight != NULL) &&
	       "copy_sched couldn't set schedule_in_flight!");

	/* The last_vanished agent is not on any queues. */
	if (src->last_vanished_agent != NULL) {
		dest->last_vanished_agent =
			copy_agent(src->last_vanished_agent);
		if (src->last_agent == src->last_vanished_agent) {
			assert(dest->last_agent == NULL &&
			       "but last_agent was already found!");
			dest->last_agent = dest->last_vanished_agent;
		}
	} else {
		dest->last_vanished_agent = NULL;
	}

	/* Must be after the last_vanished copy in case it was the last_agent */
	assert((src->last_agent == NULL || dest->last_agent != NULL) &&
	       "copy_sched couldn't set last_agent!");

	dest->inflight_tick_count    = src->inflight_tick_count;
	dest->delayed_in_flight      = src->delayed_in_flight;
	dest->just_finished_reschedule = src->just_finished_reschedule;
	dest->current_extra_runnable = src->current_extra_runnable;
	dest->num_agents             = src->num_agents;
	dest->most_agents_ever       = src->most_agents_ever;
	dest->guest_init_done        = src->guest_init_done;
	dest->entering_timer         = src->entering_timer;
	dest->voluntary_resched_tid  = src->voluntary_resched_tid;
	dest->voluntary_resched_stack =
		(src->voluntary_resched_stack == NULL) ? NULL :
			copy_stack_trace(src->voluntary_resched_stack);
	lockset_clone(&dest->known_semaphores, &src->known_semaphores);
#ifdef PURE_HAPPENS_BEFORE
	lock_clocks_copy(&dest->lock_clocks, &src->lock_clocks);
	vc_copy(&dest->scheduler_lock_clock, &src->scheduler_lock_clock);
	dest->scheduler_lock_held = src->scheduler_lock_held;
#endif
	ARRAY_LIST_CLONE(&dest->dpor_preferred_tids, &src->dpor_preferred_tids);
	dest->deadlock_fp_avoidance_count = src->deadlock_fp_avoidance_count;
	dest->icb_preemption_count = src->icb_preemption_count;
	dest->any_thread_txn = src->any_thread_txn;
	dest->delayed_txn_fail = src->delayed_txn_fail;
	dest->delayed_txn_fail_tid = src->delayed_txn_fail_tid;
	dest->upcoming_aborts = src->upcoming_aborts;
}

static void copy_test(struct test_state *dest, const struct test_state *src)
{
	dest->test_is_running      = src->test_is_running;
	dest->test_ended           = src->test_ended;
	dest->start_population     = src->start_population;
	dest->start_kern_heap_size = src->start_kern_heap_size;
	dest->start_user_heap_size = src->start_user_heap_size;

	if (src->current_test == NULL) {
		dest->current_test = NULL;
	} else {
		dest->current_test = MM_XSTRDUP(src->current_test);
	}
}
static struct rb_node *dup_chunk(const struct rb_node *nobe,
				 const struct rb_node *parent)
{
	if (nobe == NULL)
		return NULL;

	struct chunk *src = rb_entry(nobe, struct chunk, nobe);
	struct chunk *dest = MM_XMALLOC(1, struct chunk);

	/* dup rb node contents */
	int colour_flag = src->nobe.rb_parent_color & 1;

	assert(((unsigned long)parent & 1) == 0);
	dest->nobe.rb_parent_color = (unsigned long)parent | colour_flag;
	dest->nobe.rb_right = dup_chunk(src->nobe.rb_right, &dest->nobe);
	dest->nobe.rb_left  = dup_chunk(src->nobe.rb_left, &dest->nobe);

	dest->base = src->base;
	dest->len  = src->len;
	dest->id   = src->id;

	if (src->malloc_trace == NULL) {
		dest->malloc_trace = NULL;
	} else {
		dest->malloc_trace = copy_stack_trace(src->malloc_trace);
	}
	if (src->free_trace == NULL) {
		dest->free_trace = NULL;
	} else {
		dest->free_trace = copy_stack_trace(src->free_trace);
	}

	dest->pages_reserved_for_malloc = src->pages_reserved_for_malloc;

	return &dest->nobe;
}
static void copy_mem(struct mem_state *dest, const struct mem_state *src, bool in_tree)
{
	dest->guest_init_done     = src->guest_init_done;
	dest->in_mm_init          = src->in_mm_init;
	dest->malloc_heap.rb_node = dup_chunk(src->malloc_heap.rb_node, NULL);
	dest->palloc_heap.rb_node = dup_chunk(src->palloc_heap.rb_node, NULL);
	dest->heap_size           = src->heap_size;
	dest->heap_next_id        = src->heap_next_id;
#ifndef ALLOW_REENTRANT_MALLOC_FREE
	copy_malloc_actions(&dest->flags, &src->flags);
#endif
	dest->cr3                 = src->cr3;
	dest->cr3_tid             = src->cr3_tid;
	dest->user_mutex_size     = src->user_mutex_size;
	dest->during_xchg         = src->during_xchg;
	dest->last_xchg_read      = src->last_xchg_read;
	ARRAY_LIST_CLONE(&dest->newpageses, &src->newpageses);
	/* NB: The shm and freed heaps are copied below, in shimsham_shm,
	 * because this function is also used to restore when time travelling,
	 * and we want it to reset the shm and freed heap to empty. But,
	 * depending whether we're testing user or kernel, we might skip
	 * the shimsham_shm call, so we at least must initialize them here. */
	dest->shm.rb_node         = NULL;
	dest->freed.rb_node       = NULL;
	/* do NOT copy data_races! */
	if (in_tree) {
		/* see corresponding assert in free_mem() */
		dest->data_races.rb_node = NULL;
		dest->data_races_suspected = 0;
		dest->data_races_confirmed = 0;
	}
}
static void copy_user_sync(struct user_sync_state *dest,
			   const struct user_sync_state *src,
			   int already_known_size)
{
	if (already_known_size == 0) {
		/* save work: don't clear an already learned size when jumping
		 * back in time, which would make us search the symtables all
		 * over again. it's not like user mutex size ever changes. */
		dest->mutex_size = src->mutex_size;
	}
	Q_INIT_HEAD(&dest->mutexes);

	struct mutex *mp_src;
	Q_FOREACH(mp_src, &src->mutexes, nobe) {
		struct mutex *mp_dest = MM_XMALLOC(1, struct mutex);
		mp_dest->addr = mp_src->addr;
		Q_INIT_HEAD(&mp_dest->chunks);

		struct mutex_chunk *c_src;
		Q_FOREACH(c_src, &mp_src->chunks, nobe) {
			struct mutex_chunk *c_dest = MM_XMALLOC(1, struct mutex_chunk);
			c_dest->base = c_src->base;
			c_dest->size = c_src->size;
			Q_INSERT_HEAD(&mp_dest->chunks, c_dest, nobe);
		}
		assert(Q_GET_SIZE(&mp_dest->chunks) == Q_GET_SIZE(&mp_src->chunks));

		Q_INSERT_HEAD(&dest->mutexes, mp_dest, nobe);
	}

	assert(Q_GET_SIZE(&dest->mutexes) == Q_GET_SIZE(&src->mutexes));

	assert(src->yield_progress == NOTHING_INTERESTING &&
	       "user yield progress flag wasn't reset before checkpoint");
	dest->yield_progress = NOTHING_INTERESTING;
	dest->xchg_count = src->xchg_count;
	dest->xchg_loop_has_pps = src->xchg_loop_has_pps;
}

/* To free copied state data structures. None of these free the arg pointer. */
static void free_sched_q(struct agent_q *q)
{
	while (Q_GET_SIZE(q) > 0) {
		struct agent *a = Q_GET_HEAD(q);
		assert(a != NULL);
		Q_REMOVE(q, a, nobe);
		lockset_free(&a->kern_locks_held);
		lockset_free(&a->user_locks_held);
#ifdef PURE_HAPPENS_BEFORE
		vc_destroy(&a->clock);
#endif
		if (a->pre_vanish_trace != NULL) {
			free_stack_trace(mutable_pre_vanish_trace(a));
		}
		MM_FREE(a);
	}
}
static void free_sched(struct sched_state *s)
{
	free_sched_q(&s->rq);
	free_sched_q(&s->dq);
	free_sched_q(&s->sq);
	lockset_free(&s->known_semaphores);
#ifdef PURE_HAPPENS_BEFORE
	lock_clocks_destroy(&s->lock_clocks);
	vc_destroy(&s->scheduler_lock_clock);
#endif
	ARRAY_LIST_FREE(&s->dpor_preferred_tids);
}
static void free_test(const struct test_state *t)
{
	MM_FREE(t->current_test);
}

static void free_heap(struct rb_node *nobe)
{
	if (nobe == NULL)
		return;
	free_heap(nobe->rb_left);
	free_heap(nobe->rb_right);

	struct chunk *c = rb_entry(nobe, struct chunk, nobe);
	if (c->malloc_trace != NULL) free_stack_trace(mutable_malloc_trace(c));
	if (c->free_trace   != NULL) free_stack_trace(mutable_free_trace(c));
	MM_FREE(c);
}

static void free_shm(struct rb_node *nobe)
{
	if (nobe == NULL)
		return;
	free_shm(nobe->rb_left);
	free_shm(nobe->rb_right);
	struct mem_access *ma = rb_entry(nobe, struct mem_access, nobe);
	while (Q_GET_SIZE(&ma->locksets) > 0) {
		struct mem_lockset *l = Q_GET_HEAD(&ma->locksets);
		assert(l != NULL);
		Q_REMOVE(&ma->locksets, l, nobe);
		lockset_free(&l->locks_held);
#ifdef PURE_HAPPENS_BEFORE
		vc_destroy(&l->clock);
#endif
		MM_FREE(l);
	}
	MM_FREE(ma);
}

static void free_mem(struct mem_state *m, bool in_tree)
{
	free_heap(m->malloc_heap.rb_node);
	m->malloc_heap.rb_node = NULL;
	free_heap(m->palloc_heap.rb_node);
	m->palloc_heap.rb_node = NULL;
	free_shm(m->shm.rb_node);
	m->shm.rb_node = NULL;
	free_heap(m->freed.rb_node);
	m->freed.rb_node = NULL;
	if (in_tree) {
		/* data races are "glowing green", and should only appear in the
		 * copy of mem_state owned by landslide itself; never copied */
		assert(m->data_races.rb_node == NULL);
		assert(m->data_races_suspected == 0);
		assert(m->data_races_confirmed == 0);
	}
}

static unsigned int free_user_sync(struct user_sync_state *u)
{
	while (Q_GET_SIZE(&u->mutexes) > 0) {
		struct mutex *mp = Q_GET_HEAD(&u->mutexes);
		assert(mp != NULL);
		Q_REMOVE(&u->mutexes, mp, nobe);
		while (Q_GET_SIZE(&mp->chunks) > 0) {
			struct mutex_chunk *c = Q_GET_HEAD(&mp->chunks);
			assert(c != NULL);
			Q_REMOVE(&mp->chunks, c, nobe);
			MM_FREE(c);
		}
		MM_FREE(mp);
	}

	return u->mutex_size;
}

static void free_arbiter_choices(struct arbiter_state *a)
{
	struct choice *c;
	Q_FOREACH(c, &a->choices, nobe) {
		lsprintf(INFO, "Preserving buffered arbiter choice %d\n", c->tid);
	}
}

static void free_hax(struct hax *h)
{
	free_sched(mutable_oldsched(h));
	MM_FREE(mutable_oldsched(h));
	free_test(mutable_oldtest(h));
	MM_FREE(mutable_oldtest(h));
	free_mem(mutable_old_kern_mem(h), true);
	MM_FREE(mutable_old_kern_mem(h));
	free_mem(mutable_old_user_mem(h), true);
	MM_FREE(mutable_old_user_mem(h));
	free_user_sync(mutable_old_user_sync(h));
	MM_FREE(mutable_old_user_sync(h));
	h->oldsched = NULL;
	h->oldtest = NULL;
	h->old_kern_mem = NULL;
	h->old_user_mem = NULL;
	MM_FREE(mutable_conflicts(h));
	MM_FREE(mutable_happens_before(h));
	h->conflicts = NULL;
	h->happens_before = NULL;
	free_stack_trace(mutable_stack_trace(h));
	ARRAY_LIST_FREE(mutable_children(h));
	if (h->xbegin) {
		ARRAY_LIST_FREE(mutable_xabort_codes_todo(h));
		ARRAY_LIST_FREE(mutable_xabort_codes_ever(h));
	}
	ARRAY_LIST_FREE(mutable_abort_sets_ever(h));
	ARRAY_LIST_FREE(mutable_abort_sets_todo(h));
}

/* Reverse that which is not glowing green. */
static void restore_ls(struct ls_state *ls, const struct hax *h)
{
	lsprintf(DEV, "88 MPH: eip 0x%x -> 0x%x; "
		 "triggers %lu -> %lu (absolute %" PRIu64 "); last choice %d\n",
		 ls->eip, h->eip, ls->trigger_count, h->trigger_count,
		 ls->absolute_trigger_count, h->chosen_thread);

	ls->eip           = h->eip;
	ls->trigger_count = h->trigger_count;

	// TODO: can have "move" instead of "copy" for these
	free_sched(&ls->sched);
	copy_sched(&ls->sched, h->oldsched);
	free_test(&ls->test);
	copy_test(&ls->test, h->oldtest);
	free_mem(&ls->kern_mem, false);
	copy_mem(&ls->kern_mem, h->old_kern_mem, false); /* note: leaves shm empty, as we want */
	free_mem(&ls->user_mem, false);
	copy_mem(&ls->user_mem, h->old_user_mem, false); /* as above */
	int already_known_size = free_user_sync(&ls->user_sync);
	copy_user_sync(&ls->user_sync, h->old_user_sync, already_known_size);
	free_arbiter_choices(&ls->arbiter);

	set_symtable(h->old_symtable);

	ls->just_jumped = true;
}

/******************************************************************************
 * Independence and happens-before computation
 ******************************************************************************/

static void inherit_happens_before(struct hax *h, const struct hax *old)
{
	for (int i = 0; i < old->depth; i++) {
		set_happens_before(h, i, h->happens_before[i] || old->happens_before[i]);
	}
}

static bool enabled_by(struct hax *h, const struct hax *old)
{
	if (old->parent == NULL) {
		lsprintf(INFO, "#%d/tid%d has no parent\n",
			 old->depth, old->chosen_thread);
		return true;
	} else {
		const struct agent *a;
		lsprintf(INFO, "Searching for #%d/tid%d among siblings of "
			 "#%d/tid%d: ", h->depth, h->chosen_thread, old->depth,
			 old->chosen_thread);
		print_qs(INFO, old->parent->oldsched);
		CONST_FOR_EACH_RUNNABLE_AGENT(a, old->parent->oldsched,
			if (a->tid == h->chosen_thread && !BLOCKED(a)) {
				printf(INFO, "not enabled_by\n");
				return false;
			}
		);
		/* Transition A enables transition B if B was not a sibling of
		 * A; i.e., if before A was run B could not have been chosen. */
		printf(INFO, "yes enabled_by\n");
		return true;
	}
}

static void compute_happens_before(struct hax *h, unsigned int joined_tid)
{
	unsigned int i;

	/* Between two transitions of a thread X, X_0 before X_1, while there
	 * may be many transitions Y that for which enabled_by(X_1, Y), only the
	 * earliest such Y (the one soonest after X_0) is the actual enabler. */
	const struct hax *enabler = NULL;

	for (i = 0; i < h->depth; i++) {
		set_happens_before(h, i, false);
	}

	for (const struct hax *old = h->parent; old != NULL; old = old->parent) {
		assert(--i == old->depth); /* sanity check */
		assert(old->depth >= 0 && old->depth < h->depth);
		if (h->chosen_thread == old->chosen_thread) {
			set_happens_before(h, old->depth, true);
			inherit_happens_before(h, old);
			/* Computing any further would be redundant, and would
			 * break the true-enabler finding alg. */
			break;
		} else if (enabled_by(h, old)) {
			enabler = old;
			set_happens_before(h, enabler->depth, true);
		}
	}

	/* Take the happens-before set of the oldest enabler. */
	if (enabler != NULL) {
		set_happens_before(h, enabler->depth, true);
		inherit_happens_before(h, enabler);
	}

#ifdef TRUSTED_THR_JOIN
	if (joined_tid != TID_NONE) {
		/* because this pp is at the *end* of join; it doesn't actually
		 * matter if the joinee is still alive or not -- if it is, they
		 * will be in vanish or close nearby, past the exit signal step
		 * at least. just find the last thing it did before us. */
		for (const struct hax *old = h->parent; old != NULL;
		     old = old->parent) {
			if (joined_tid == old->chosen_thread) {
				set_happens_before(h, old->depth, true);
				inherit_happens_before(h, old);
				break;
			}
			assert(old->parent != NULL &&
			       "trusted thr join not so trusted...");
		}
	}
#endif

	lsprintf(DEV, "Transitions { ");
	for (i = 0; i < h->depth; i++) {
		if (h->happens_before[i]) {
			printf(DEV, "#%d ", i);
		}
	}
	printf(DEV, "} happen-before #%d/tid%d.\n", h->depth, h->chosen_thread);
}

static void add_xabort_code(struct hax *h, unsigned int *code)
{
	assert(h->xbegin);
	unsigned int i;
	unsigned int *existing_code;
	// this can be done for mario's htm data structures;
	// or any program you trust that no txn branches on the failure code.
	// // merge conflict and explicit together for marios
	// *code = _XABORT_CONFLICT;
	ARRAY_LIST_FOREACH(mutable_xabort_codes_ever(h), i, existing_code) {
		if (*existing_code == *code) {
			return;
		}
	}
	ARRAY_LIST_APPEND(mutable_xabort_codes_ever(h), *code);
	ARRAY_LIST_APPEND(mutable_xabort_codes_todo(h), *code);
}


/* h2 should be supplied as the parent PP of the transition which should abort */
void abort_transaction(unsigned int tid, const struct hax *h2, unsigned int code)
{
#ifdef HTM_ABORT_CODES
	// FIXME: this should have stronger assertions regarding being called
	// from within a transaction to ensure the code doesn't just get lost
	for (; h2 != NULL; h2 = h2->parent) {
		if (h2->chosen_thread == tid) {
			if (h2->xbegin) {
				modify_hax(add_xabort_code, h2, code);
			} else {
				assert(code != _XABORT_EXPLICIT);
			}
			/* Done either way. If wasn't xbegin, wasn't a txn. */
			return;
		}
	}
#endif
}

/* Resets the current set of shared-memory accesses by moving what we've got so
 * far into the save point we're creating. Then, intersects the memory accesses
 * with those of each ancestor to compute independences and find data races.
 * NB: Looking for data races depends on having computed happens_before first. */
static void shimsham_shm(struct ls_state *ls, struct hax *h, bool in_kernel)
{
	/* note: NOT "&h->foo"! gcc does NOT warn for this mistake! ARGH! */
	struct mem_state *oldmem = in_kernel ? mutable_old_kern_mem(h)
	                                     : mutable_old_user_mem(h);
	struct mem_state *newmem = in_kernel ? &ls->kern_mem   : &ls->user_mem;

	/* store shared memory accesses from this transition; reset to empty */
	oldmem->shm.rb_node = newmem->shm.rb_node;
	newmem->shm.rb_node = NULL;

	/* do the same for the list of freed chunks in this transition */
	oldmem->freed.rb_node = newmem->freed.rb_node;
	newmem->freed.rb_node = NULL;

	/* ensure that memory tracking kept the shm heap totally empty for the
	 * space (kernel or user) that we're NOT testing. */
	if (in_kernel && testing_userspace()) {
		assert(oldmem->shm.rb_node == NULL &&
		       "kernel shm nonempty when testing userspace");
		return;
	} else if (!in_kernel && !testing_userspace()) {
		assert(oldmem->shm.rb_node == NULL &&
		       "user shm nonempty when testing kernelspace");
		return;
	}

	/* compute newly-completed transition's conflicts with previous ones */
	for (const struct hax *old = h->parent; old != NULL; old = old->parent) {
		assert(old->depth >= 0 && old->depth < h->depth);
		if (h->chosen_thread == old->chosen_thread) {
			lsprintf(INFO, "Same TID %d for #%d and #%d\n",
				 h->chosen_thread, h->depth, old->depth);
		} else if (TID_IS_IDLE(h->chosen_thread) ||
			   TID_IS_IDLE(old->chosen_thread)) {
			/* Idle shouldn't have siblings, but just in case. */
			set_conflicts(h, old->depth, true);
		} else if (old->depth == 0) {
			/* Basically guaranteed, and irrelevant. Suppress printing. */
			set_conflicts(h, 0, true);
		} else if (h->happens_before[old->depth]) {
			/* No conflict if reordering is impossible */
			set_conflicts(h, old->depth, false);
		} else {
			/* The haxes are independent if there was no intersection. */
			set_conflicts(h, old->depth,
				      mem_shm_intersect(ls, h, old, in_kernel));
			if (h->conflicts[old->depth]) {
				abort_transaction(h->chosen_thread, h->parent,
						  _XABORT_CONFLICT);
				/* no!!!!!!! (see htm_causality.c) */
				//abort_transaction(old->chosen_thread, old->parent,
				//		  _XABORT_CONFLICT);
			}
		}
	}
}

/******************************************************************************
 * interface
 ******************************************************************************/

#define HAX_CHILDREN_INIT_SIZE 4

void save_init(struct save_state *ss)
{
	ss->root = NULL;
	ss->current = NULL;
	ss->next_tid = TID_NONE;
	ss->next_xabort = false;
	ss->next_xabort_code = _XBEGIN_STARTED;
	ss->stats.total_choices = 0;
	ss->stats.total_jumps = 0;
	ss->stats.total_triggers = 0;
	ss->stats.depth_total = 0;
	ss->stats.total_usecs = 0;

	update_time(&ss->stats.last_save_time);
}

void save_recover(struct save_state *ss, struct ls_state *ls, int new_tid, bool xabort, unsigned int xabort_code)
{
	/* After a longjmp, we will be on exactly the node we jumped to, but
	 * there must be a special call to let us know what our new course is
	 * (see sched_recover). */
	assert(ls->just_jumped);
	ss->next_tid = new_tid;
	ss->next_xabort = xabort;
	ss->next_xabort_code = xabort_code;
	lsprintf(INFO, "explorer chose tid %d; ready for action\n", new_tid);
}

static void add_hax_child(struct hax *h, int tid, bool xabort, unsigned int xabort_code)
{
	const struct hax_child *other;
	unsigned int i;
	/* with abort sets, duplicate children differentiated only by those sets
	 * can get added here; tracking them (e.g. to ensure no duplicates with
	 * even identical abort sets get added) would be too much of a pain */
#ifndef HTM_ABORT_SETS
	ARRAY_LIST_FOREACH(&h->children, i, other) {
		assert(other->chosen_thread != tid || other->xabort != xabort ||
		       (other->xabort && xabort && other->xabort_code != xabort_code));
	}
#endif
	struct hax_child child;
	child.chosen_thread = tid;
	child.all_explored  = false;
	child.xabort        = xabort;
	child.xabort_code   = xabort_code;
	ARRAY_LIST_APPEND(mutable_children(h), child);
}

static void add_hax_child_preempt(struct hax *h, int *chosen_thread)
	{ add_hax_child(h, *chosen_thread, false, _XBEGIN_STARTED); }

/* chosen thread will be the same as haxs chosen thread */
static void add_hax_child_xabort(struct hax *h, unsigned int *xabort_code)
	{ add_hax_child(h, h->chosen_thread, true, *xabort_code); }

/* In the typical case, this signifies that we have reached a new decision
 * point. We:
 *  - Add a new choice node to signify this
 *  - The TID that was chosen to get here is stored in ss->new_tid; copy that
 *  - Create a list of possible TIDs to explore from this choice point
 *  - Store the new_tid for some subsequent choice point
 */
void save_setjmp(struct save_state *ss, struct ls_state *ls,
		 int new_tid, bool our_choice, bool end_of_test,
		 bool is_preemption_point, unsigned int data_race_eip,
		 bool voluntary, unsigned int joined_tid, bool xbegin,
		 bool prune_aborts, bool check_retry)
{
	struct hax *h;

	lsprintf(INFO, "tid %d to eip 0x%x, where we %s tid %d\n", ss->next_tid,
		 ls->eip, our_choice ? "choose" : "follow", new_tid);

	/* Whether there should be a choice node in the tree is dependent on
	 * whether the current pending choice was our decision or not. The
	 * explorer's choice (!ours) will be in anticipation of a new node, but
	 * at that point we won't have the info to create the node until we go
	 * one step further. */
	if (our_choice) {
		h = MM_XMALLOC(1, struct hax);

		h->eip           = ls->eip;
		h->trigger_count = ls->trigger_count;
		h->chosen_thread = ss->next_tid;
		h->xaborted      = ss->next_xabort;
		h->xabort_code   = ss->next_xabort_code;

		/* compute elapsed and cumulative time */
		h->usecs = update_time(&ss->stats.last_save_time);
		ss->stats.total_usecs += h->usecs;

		/* Put the choice into the tree. */
		if (ss->root == NULL) {
			/* First/root choice. */
			assert(ss->current == NULL);
			assert(end_of_test || ss->next_tid == TID_NONE);

			/* No, not necessarily... (only true for pathos) */
			//assert(is_preemption_point);

			h->parent = NULL;
			h->depth  = 0;
			ss->root  = h;
		} else {
			/* Subsequent choice. */
			assert(ss->current != NULL);
			assert(end_of_test || ss->next_tid != TID_NONE);
			assert(!ss->current->estimate_computed &&
			       "last nobe was estimate()d; cannot give it a child");

			if (h->xaborted) {
				assert(ss->current->chosen_thread == h->chosen_thread);
				modify_hax(add_hax_child_xabort, ss->current,
					   ss->next_xabort_code);
			} else {
				modify_hax(add_hax_child_preempt, ss->current,
					   h->chosen_thread);
			}

			h->parent = ss->current;
			h->depth = 1 + h->parent->depth;

			lsprintf(DEV, "elapsed usecs %" PRIu64 "\n", h->usecs);
		}

		ARRAY_LIST_INIT(&h->children, HAX_CHILDREN_INIT_SIZE);
		h->all_explored = end_of_test;

		h->data_race_eip = data_race_eip;
#ifdef PREEMPT_EVERYWHERE
		h->is_preemption_point = true;
#else
		/* root nobe can't be speculative; see explore.c pp_parent() */
		h->is_preemption_point = is_preemption_point || h->parent == NULL;
		if (is_preemption_point) { assert(data_race_eip == ADDR_NONE); }
#endif

		h->marked_children = 0;
		h->proportion = 0.0L;
		h->subtree_usecs = 0.0L;
		h->estimate_computed = false;
		h->voluntary = voluntary;
		if ((h->xbegin = xbegin)) {
			ARRAY_LIST_INIT(&h->xabort_codes_ever, 8);
			ARRAY_LIST_INIT(&h->xabort_codes_todo, 8);
#ifndef HTM_DONT_RETRY
			/* e.g. mario's htm data structures may supply -A -S to
			 * disable this code, since they wrap these in a retry
			 * loop, leaving DPOR conflicts as the only aborts */
			if (prune_aborts) {
				/* suppress the other xbegin outcome */
				if (check_retry) {
					ARRAY_LIST_APPEND(
						mutable_xabort_codes_ever(h),
						_XABORT_RETRY);
				}
			} else {
				/* test success first, abort later, as normal */
				unsigned int retry_code = _XABORT_RETRY;
				add_xabort_code(h, &retry_code);
			}
#endif
		}
		ARRAY_LIST_INIT(&h->abort_sets_ever, 8);
		ARRAY_LIST_INIT(&h->abort_sets_todo, 8);

		timetravel_hax_init(&h->time_machine);

		if (voluntary) {
#ifndef PINTOS_KERNEL
			assert(h->chosen_thread == TID_NONE || h->chosen_thread ==
			       ls->sched.voluntary_resched_tid);
#endif
			assert(ls->sched.voluntary_resched_stack != NULL);
			assert(data_race_eip == ADDR_NONE);
			h->stack_trace = ls->sched.voluntary_resched_stack;
			ls->sched.voluntary_resched_stack = NULL;
		} else {
			struct stack_trace *st = stack_trace(ls);
			h->stack_trace = st;

			// FIXME: make this happen for xbegin delayed-eip pps
			// as well (prob requires another argument to setjmp)
			// i mean, it's not like any studence will need it tho?
			if (data_race_eip != ADDR_NONE) {
				/* first frame of stack will be bogus, due to
				 * the technique for delaying the access (in
				 * x86.c). fix it up with the proper eip. */
				struct stack_frame *first_frame =
					ARRAY_LIST_GET(mutable_stack_frames(st), 0);
				assert(first_frame != NULL);
				destroy_frame(first_frame);
				eip_to_frame(data_race_eip, first_frame);
			}
		}
	} else {
		assert(0 && "Not our_choice deprecated.");
	}

	h->oldsched = MM_XMALLOC(1, struct sched_state);
	copy_sched(mutable_oldsched(h), &ls->sched);

	h->oldtest = MM_XMALLOC(1, struct test_state);
	copy_test(mutable_oldtest(h), &ls->test);

	h->old_kern_mem = MM_XMALLOC(1, struct mem_state);
	copy_mem(mutable_old_kern_mem(h), &ls->kern_mem, true);

	h->old_user_mem = MM_XMALLOC(1, struct mem_state);
	copy_mem(mutable_old_user_mem(h), &ls->user_mem, true);

	h->old_user_sync = MM_XMALLOC(1, struct user_sync_state);
	copy_user_sync(mutable_old_user_sync(h), &ls->user_sync, 0);

	h->old_symtable = get_symtable();

	if (h->depth > 0) {
		h->conflicts      = MM_XMALLOC(h->depth, bool);
		h->happens_before = MM_XMALLOC(h->depth, bool);
		/* For progress sense. */
		ss->stats.total_triggers +=
			ls->trigger_count - h->parent->trigger_count;
	} else {
		h->conflicts      = NULL;
		h->happens_before = NULL;
	}
	compute_happens_before(h, joined_tid);
	/* Compute independence relation for both kernel and user mems. Whether
	 * to actually compute for either is determined by the user/kernel config
	 * option, which decides whether we will have stored the user/kernel shms
	 * at all (e.g., running in user mode, the kernel shm will be empty). */
	shimsham_shm(ls, h, true);
	shimsham_shm(ls, h, false);

	ss->current  = h;
	ss->next_tid = new_tid;
	if (check_retry) {
		/* using abort set reduction, we skipped the success path */
		assert(prune_aborts && "you can't just prune for no reason!");
		ss->next_xabort = true;
		ss->next_xabort_code = _XABORT_RETRY;
	} else {
		/* normal xbegin success path (regardless of abort sets) */
		ss->next_xabort = false;
		ss->next_xabort_code = _XBEGIN_STARTED;
	}
	if (h->chosen_thread == TID_NONE) {
		lsprintf(CHOICE, MODULE_COLOUR "#%d: Starting test with TID %d.\n"
			 COLOUR_DEFAULT, h->depth, ss->next_tid);
	} else if (ss->next_tid == TID_NONE) {
		lsprintf(CHOICE, MODULE_COLOUR "#%d: Test ends with TID %d.\n"
			 COLOUR_DEFAULT, h->depth, h->chosen_thread);
	} else if (h->chosen_thread != ss->next_tid) {
		if (voluntary) {
			lsprintf(CHOICE, COLOUR_BOLD MODULE_COLOUR "#%d: Voluntary "
				 "reschedule from TID %d to TID %d\n" COLOUR_DEFAULT,
				 h->depth, h->chosen_thread, ss->next_tid);
		} else {
			lsprintf(CHOICE, COLOUR_BOLD MODULE_COLOUR "#%d: Preempting "
				 "TID %d to switch to TID %d\n" COLOUR_DEFAULT,
				 h->depth, h->chosen_thread, ss->next_tid);
		}
		lsprintf(CHOICE, "Current stack: ");
		print_stack_trace(CHOICE, h->stack_trace);
		printf(CHOICE, "\n");
	} else {
		lsprintf(CHOICE, MODULE_COLOUR "#%d: Allowing TID %d to continue.\n"
			 COLOUR_DEFAULT, h->depth, h->chosen_thread);
	}

	ss->stats.total_choices++;
	unsigned int tid;
	bool txn;
	unsigned int xabort_code;
	struct abort_set aborts;
	bool was_jumped_to = false;
	/* In bochs, if timetravel-set indicates we're a jumped-to child and
	 * should switch our choice, we also need to refresh the time machine
	 * so it can be jumped to again. Do this "until" we're the parent. */
	while (timetravel_set(ls, h, &tid, &txn, &xabort_code, &aborts)) {
		lsprintf(DEV, "#%d/tid%d: child process, suggested to run "
			 "tid%d/txn%d/code%d; refreshing save point\n",
			 h->depth, h->chosen_thread, tid, txn, xabort_code);
		was_jumped_to = true;
	}
	if (was_jumped_to) {
#ifdef BOCHS
		if (tid != TID_NONE) {
			arbiter_append_choice(&ls->arbiter, tid, txn,
					      xabort_code, &aborts);
		}
		restore_ls(ls, h);
#else
		assert(0 && "in simics timetravel-set should only return once");
#endif
	}
}

void save_longjmp(struct save_state *ss, struct ls_state *ls, const struct hax *h,
		  unsigned int tid, bool txn, unsigned int xabort_code,
		  struct abort_set *aborts)
{
	const struct hax *rabbit = ss->current;

	assert(ss->root != NULL && "Can't longjmp with no decision tree!");
	assert(ss->current != NULL);
	assert(ss->current->estimate_computed);

	ss->stats.depth_total += ss->current->depth;

	/* The caller is allowed to say NULL, which means jump to the root. */
	if (h == NULL)
		h = ss->root;

	/* Find the target choice point from among our ancestors. */
	while (ss->current != h) {
		timetravel_delete(ls, &ss->current->time_machine);
#ifndef BOCHS
		/* This nobe will soon be in the future. Reclaim memory.
		 * (Bochs, ofc, will reclaim the memory upon process exit.) */
		struct hax *old_current = (struct hax *)ss->current;
		free_hax(old_current);
#endif
		ss->current = ss->current->parent;
		/* allow for empty children iff ICB just reset the tree */
		assert(ARRAY_LIST_SIZE(&ss->current->children) > 0 ||
		       (ss->current == ss->root && ss->stats.total_jumps == -1));
#ifndef BOCHS
		MM_FREE(old_current);
#endif

		/* We won't have simics bookmarks, or indeed some saved state,
		 * for non-ancestor choice points. */
		assert(ss->current != NULL && "Can't jump to a non-ancestor!");

		/* the tree must not have properties of Philip J. Fry */
		if (rabbit) rabbit = rabbit->parent;
		if (rabbit) rabbit = rabbit->parent;
		assert(rabbit != ss->current && "somehow, a cycle?!?");
	}

#ifndef BOCHS
	/* In simics, timetravel-jump will return and this process will have
	 * properties of my wayward son; so prepare state for the new branch.
	 * In bochs, it will not return; rather timetravel_set returns twice.
	 * These ifndefs just skip whatever the next process will never see. */
	arbiter_append_choice(&ls->arbiter, tid, txn, xabort_code, aborts);
	restore_ls(ls, h);
#endif

	ss->stats.total_jumps++;
	timetravel_jump(ls, &h->time_machine, tid, txn, xabort_code, aborts);
#ifdef BOCHS
	assert(0 && "returned from time leap somehow");
#endif
}

#ifdef ICB
static void reset_root(struct hax *root, int *unused)
{
	assert(root->parent == NULL);
	struct agent *a;
	FOR_EACH_RUNNABLE_AGENT(a, mutable_oldsched(root),
		a->do_explore = false;
	);

	/* Need to reset tree state as if this is the 1st time we came here. */
	ARRAY_LIST_FREE(mutable_children(root));
	ARRAY_LIST_INIT(mutable_children(root), HAX_CHILDREN_INIT_SIZE);
	root->all_explored = false;
	root->marked_children = 0;
	root->proportion = 0.0L;
	root->subtree_usecs = 0.0L;
	root->estimate_computed = false;
}

void save_reset_tree(struct save_state *ss, struct ls_state *ls)
{
	/* Do this before longjmp so the change gets copied into ls->sched. */
	modify_hax(reset_root, ss->root, 0);

	ss->stats.total_choices = 1;
	ss->stats.total_triggers = 0;
	ss->stats.total_usecs = ss->root->usecs;
	ss->stats.total_jumps = (uint64_t)-1; /* to become 0; see longjmp */
	ss->stats.depth_total = (uint64_t)-ss->current->depth; /* see above */

	/* As before but with some additional changes */
	struct abort_set aborts;
	ABORT_SET_INIT_INACTIVE(&aborts);
	save_longjmp(ss, ls, ss->root, TID_NONE, false, -1, &aborts);
}
#else
void save_reset_tree(struct save_state *ss, struct ls_state *ls)
{
	assert(0 && "how did this get here i am not good with computer");
}
#endif
