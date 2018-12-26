/**
 * @file pp.h
 * @brief preemption point config
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

#ifndef __LS_PP_H
#define __LS_PP_H

#include "array_list.h"
#include "student_specifics.h"

struct ls_state;

/* ofc, these don't correspond 1-to-1 to PPs; they indicate whitelist/blacklist
 * directives that the arbiter should use to enable or disable mutex/etc PPs. */
struct pp_within {
	unsigned int func_start;
	unsigned int func_end;
	bool within;
};

/* these are more 1-to-1 */
struct pp_data_race {
	unsigned int addr;
	unsigned int tid;
	unsigned int last_call;
	unsigned int most_recent_syscall;
};

typedef ARRAY_LIST(struct pp_within) pp_within_list_t;

struct pp_config {
	bool dynamic_pps_loaded;
	pp_within_list_t kern_withins;
	pp_within_list_t user_withins;
	ARRAY_LIST(struct pp_data_race) data_races;
	char *output_pipe_filename;
	char *input_pipe_filename;
};

void pps_init(struct pp_config *p);
bool load_dynamic_pps(struct ls_state *ls, const char *filename);

bool kern_within_functions(struct ls_state *ls);
bool user_within_functions(struct ls_state *ls);
bool suspected_data_race(struct ls_state *ls);
#ifdef PREEMPT_EVERYWHERE
void maybe_preempt_here(struct ls_state *ls, unsigned int addr);
#endif

#endif
