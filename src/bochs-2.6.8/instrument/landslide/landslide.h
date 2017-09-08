/**
 * @file landslide.h
 * @brief common landslide stuff
 * @author Ben Blum
 */

#ifndef __LS_LANDSLIDE_H
#define __LS_LANDSLIDE_H

#include <stdbool.h>
#include <stdint.h>

#include "arbiter.h"
#include "memory.h"
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
