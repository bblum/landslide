/**
 * @file lockset.h
 * @brief Data structures for data race detection
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

#ifndef __LS_LOCKSET_H
#define __LS_LOCKSET_H

#include "array_list.h"
#include "common.h"

struct sched_state;

enum lock_type {
	LOCK_MUTEX,
	LOCK_SEM,
	LOCK_RWLOCK,
	LOCK_RWLOCK_READ,
};

#define SAME_LOCK_TYPE(t1, t2) \
	(((t1) == (t2)) || \
	 ((t1) == LOCK_RWLOCK && (t2) == LOCK_RWLOCK_READ) || \
	 ((t1) == LOCK_RWLOCK_READ && (t2) == LOCK_RWLOCK))

struct lock {
	unsigned int addr;
	enum lock_type type;
};

/* Tracks the locks held by a given thread, for data race detection. */
struct lockset {
	ARRAY_LIST(struct lock) list;
};

/* For efficient storage of locksets on memory accesses. */
enum lockset_cmp_result {
	LOCKSETS_EQ,     /* locksets contain all same elements */
	LOCKSETS_SUBSET, /* l0 subset l1 */
	LOCKSETS_SUPSET, /* l1 subset l0 */
	LOCKSETS_DIFF    /* locksets contain some disjoint locks */
};

void lockset_init(struct lockset *l);
void lockset_free(struct lockset *l);
void lockset_print(verbosity v, const struct lockset *l);
void lockset_clone(struct lockset *dest, const struct lockset *src);
void lockset_record_semaphore(struct lockset *semaphores, unsigned int lock_addr,
			      bool is_semaphore);
void lockset_add(struct sched_state *s, struct lockset *l,
		 unsigned int lock_addr, enum lock_type type);
void lockset_remove(struct sched_state *s, unsigned int lock_addr,
		    enum lock_type type, bool in_kernel);
bool lockset_intersect(struct lockset *l0, struct lockset *l1);
enum lockset_cmp_result lockset_compare(struct lockset *l0, struct lockset *l1);

#endif
