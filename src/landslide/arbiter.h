/**
 * @file arbiter.h
 * @author Ben Blum
 * @brief decision-making routines for landslide
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

#ifndef __LS_ARBITER_H
#define __LS_ARBITER_H

#include "tsx.h"
#include "variable_queue.h"

struct ls_state;
struct agent;

struct choice {
	unsigned int tid;
	bool txn;
	unsigned int xabort_code;
	struct abort_set aborts;
	Q_NEW_LINK(struct choice) nobe;
};

Q_NEW_HEAD(struct choice_q, struct choice);

struct arbiter_state {
	struct choice_q choices;
};

/* maintenance interface */
void arbiter_init(struct arbiter_state *);
void arbiter_append_choice(struct arbiter_state *, unsigned int tid, bool txn,
			   unsigned int xabort_code, struct abort_set *aborts);
bool arbiter_pop_choice(struct arbiter_state *, unsigned int *tid, bool *txn,
			unsigned int *xabort_code, struct abort_set *aborts);

/* scheduling interface */
bool arbiter_interested(struct ls_state *, bool just_finished_reschedule,
			bool *voluntary, bool *need_handle_sleep, bool *data_race,
			bool *joined, bool *xbegin);
bool arbiter_choose(struct ls_state *, struct agent *current, bool voluntary,
		    struct agent **result, bool *our_choice);

#endif
