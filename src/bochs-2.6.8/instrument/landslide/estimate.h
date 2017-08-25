/**
 * @file estimate.h
 * @brief online state space size estimation
 * @author Ben Blum <bblum@andrew.cmu.edu>
 */

#ifndef __LS_ESTIMATE_H
#define __LS_ESTIMATE_H

#include <sys/time.h>

struct hax;
struct ls_state;
struct agent;

/* Returns number of elapsed useconds since last call to this. If there was no
 * last call, return value is undefined. */
uint64_t update_time(struct timeval *tv);

/* internal logic used by user_sync. when a thread is identified to be
 * yield-blocked, we may need to undo estimates from tagging it in the past. */
void update_hax_yield_block_tid(struct hax *h, unsigned int *tid);
void untag_blocked_branch(const struct hax *ancestor, const struct hax *leaf,
			  const struct agent *a, bool was_ancestor);

/* main interface. */
long double estimate_time(const struct hax *root, const struct hax *current);
long double estimate_proportion(const struct hax *root, const struct hax *current);
void print_estimates(struct ls_state *ls);

#endif
