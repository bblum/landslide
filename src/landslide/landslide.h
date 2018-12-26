/**
 * @file landslide.h
 * @brief common landslide stuff
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

#ifndef __LS_LANDSLIDE_H
#define __LS_LANDSLIDE_H

#include <stdbool.h>
#include <stdint.h>

#include "arbiter.h"
#include "mem.h"
#include "messaging.h"
#include "pp.h"
#include "rand.h"
#include "save.h"
#include "schedule.h"
#include "simulator.h"
#include "test.h"
#include "timetravel.h"
#include "user_sync.h"

#define TRACE_OPCODES_LEN 16

struct ls_state {
	/* simulator_state_t must be the first thing in the device struct */
	simulator_state_t log;

	uint64_t trigger_count; /* in this branch of the tree */
	uint64_t absolute_trigger_count; /* in the whole execution */

	/* Pointers to relevant objects. Currently only supports one CPU. */
	cpu_t      *cpu0;
	keyboard_t *kbd0;
	apic_t     *apic0;
	pic_t      *pic0;
	unsigned int eip;
	unsigned int last_instr_eip;
	uint8_t instruction_text[TRACE_OPCODES_LEN];

	struct sched_state sched;
	struct arbiter_state arbiter;
	struct save_state save;
	struct test_state test;
	struct mem_state kern_mem;
	struct mem_state user_mem;
	struct user_sync_state user_sync;
	struct rand_state rand;
	struct messaging_state mess;
	struct pp_config pps;
	struct timetravel_state timetravel;

	/* used iff ICB is set */
	unsigned int icb_bound;
	bool icb_need_increment_bound;

	char *html_file;

	bool just_jumped;
	bool end_branch_early;
};

enum trace_type { TRACE_MEMORY, TRACE_EXCEPTION, TRACE_INSTRUCTION, TRACE_OTHER };

/* simulator-agnostic definition comprising only what we need */
struct trace_entry {
	enum trace_type type;
	/* if TRACE_MEMORY */
	unsigned int pa, va; /* phys/virt memory access address */
	bool write;
	/* if TRACE_EXCEPTION */
	int exn_number;
	/* if TRACE_INSTRUCTION */
	unsigned char *instruction_text; /* [TRACE_OPCODES_LEN] */
};

/* process exit codes */
#define LS_NO_KNOWN_BUG 0
#define LS_BUG_FOUND 1
#define LS_ASSERTION_FAILED 2

struct ls_state *new_landslide();
void landslide_entrypoint(struct ls_state *ls, struct trace_entry *entry);

#ifdef BOCHS
extern struct ls_state landslide;
#define GET_LANDSLIDE() (&landslide)
#else
#include "landslide-simics.h"
#endif

#endif
