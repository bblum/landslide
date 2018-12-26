/**
 * @file messaging.h
 * @brief routines for communicating with the iterative deepening wrapper
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

#ifndef __LS_MESSAGING_H
#define __LS_MESSAGING_H

struct messaging_state {
	bool pipes_opened;
	int input_fd;
	int output_fd;
};

void messaging_init(struct messaging_state *m);
void messaging_open_pipes(struct messaging_state *m, const char *i, const char *o);

#define DR_TID_WILDCARD 0x15410de0u /* 0 could be a valid tid */
void message_data_race(struct messaging_state *m, unsigned int eip,
		       unsigned int last_call, unsigned int tid,
		       unsigned int most_recent_syscall, bool confirmed,
		       bool deterministic, bool free_re_malloc);

/* returns the # of useconds that landslide was put to sleep for */
uint64_t message_estimate(struct messaging_state *m, long double proportion,
			  unsigned int elapsed_branches, long double total_usecs,
			  unsigned long elapsed_usecs,
			  unsigned int icb_preemptions, unsigned int icb_bound);

void message_found_a_bug(struct messaging_state *m, const char *trace_filename,
			 unsigned int icb_preemptions, unsigned int icb_bound);

bool should_abort(struct messaging_state *m);

void message_assert_fail(struct messaging_state *state, const char *message,
			 const char *file, unsigned int line, const char *function);

#endif
