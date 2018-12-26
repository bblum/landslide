/**
 * @file explore.h
 * @brief choice tree exploration
 * @author Ben Blum <bblum@andrew.cmu.edu>
 */

#ifndef __LS_EXPLORE_H
#define __LS_EXPLORE_H

struct arbiter_state;
struct nobe;
struct ls_state;
struct abort_set;

struct nobe *explore(struct ls_state *ls, unsigned int *new_tid, bool *txn, unsigned int *xabort_code, struct abort_set *aborts);

#endif
