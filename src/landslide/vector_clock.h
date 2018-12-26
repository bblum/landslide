/**
 * @file vector_clock.h
 * @brief Data structures for more different data race detection
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

#ifndef __LS_VECTOR_CLOCK_H
#define __LS_VECTOR_CLOCK_H

#include "array_list.h"
#include "common.h"
#include "rbtree.h"

struct epoch {
	unsigned int tid;
	unsigned int timestamp;
};

struct vector_clock {
	ARRAY_LIST(struct epoch) v;
};

/* The global set of all vector clocks associated with each mutex/xchg.
 * Corresponds to "L" in the fasttrack paper. Stored in sched_state.
 * C, W, and R are stored individually in the agent and mem_access strux. */
struct lock_clocks {
	struct rb_root map;
	unsigned int num_lox;
};

void vc_init(struct vector_clock *vc);
void vc_copy(struct vector_clock *vc_new, const struct vector_clock *vc_existing);
#define vc_destroy(vc) do { ARRAY_LIST_FREE(&(vc)->v); } while (0)
void vc_inc(struct vector_clock *vc, unsigned int tid);
unsigned int vc_get(struct vector_clock *vc, unsigned int tid);
void vc_merge(struct vector_clock *vc_dest, struct vector_clock *vc_src);
bool vc_eq(struct vector_clock *vc1, struct vector_clock *vc2);
bool vc_happens_before(struct vector_clock *vc_before, struct vector_clock *vc_after);
void vc_print(verbosity v, struct vector_clock *vc);

void lock_clocks_init(struct lock_clocks *lm);
void lock_clocks_copy(struct lock_clocks *lm_new, const struct lock_clocks *lm_existing);
void lock_clocks_destroy(struct lock_clocks *lm);
bool lock_clock_find(struct lock_clocks *lc, unsigned int lock_addr, struct vector_clock **result);
struct vector_clock *lock_clock_get(struct lock_clocks *lc, unsigned int lock_addr);
void lock_clock_set(struct lock_clocks *lc, unsigned int lock_addr, struct vector_clock *vc);

/* abstract global lock for HTM */
#define VC_TXN ((unsigned int)0xfffffffe)

/* FT-acquire, see fasttrack paper */
#define VC_ACQUIRE(lc, current_clock, lock_addr) do {			\
		struct vector_clock *__clock;				\
		if (lock_clock_find((lc), (lock_addr), &__clock)) {	\
			vc_merge((current_clock), __clock);		\
		}							\
	} while (0)

/* FT-release, see fasttrack paper */
#define VC_RELEASE(lc, current_clock, tid, lock_addr) do {		\
		struct vector_clock *__clock = (current_clock);		\
		lock_clock_set((lc), (lock_addr), __clock);		\
		vc_inc(__clock, (tid));					\
	} while (0)

#endif
