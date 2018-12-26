/**
 * @file landslide.c
 * @brief a module for friendly open-source simulators which provide yon hax and sploits
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "LANDSLIDE"
#define MODULE_COLOUR COLOUR_DARK COLOUR_MAGENTA

#include "common.h"
#include "explore.h"
#include "estimate.h"
#include "found_a_bug.h"
#include "html.h"
#include "kernel_specifics.h"
#include "kspec.h"
#include "landslide.h"
#include "mem.h"
#include "messaging.h"
#include "rand.h"
#include "save.h"
#include "simulator.h"
#include "test.h"
#include "tree.h"
#include "tsx.h"
#include "user_specifics.h"
#include "x86.h"

#ifdef BOCHS
struct ls_state landslide;
static bool landslide_inited = false;
#endif

struct ls_state *new_landslide()
{
#ifdef BOCHS
	struct ls_state *ls = &landslide;
	assert(!landslide_inited && "cant do dis twice");
	landslide_inited = true;
	ls->cpu0  = BX_CPU(0);
	ls->kbd0  = NULL;
	ls->apic0 = NULL;
	ls->pic0  = NULL;
#else
	struct ls_state *ls = ls_alloc_init();
#endif
	ls->trigger_count = 0;
	ls->absolute_trigger_count = 0;
	ls->last_instr_eip = 0;

	sched_init(&ls->sched);
	arbiter_init(&ls->arbiter);
	save_init(&ls->save);
	test_init(&ls->test);
	mem_init(ls);
	user_sync_init(&ls->user_sync);
	rand_init(&ls->rand);
	messaging_init(&ls->mess);
	pps_init(&ls->pps);
	timetravel_init(&ls->timetravel);

#ifdef ICB
	ls->icb_bound = ICB_START_BOUND;
#else
	/* garbage value; may be sent to QS in a message (WTB option types :\),
	 * but should not get printed to the user */
	ls->icb_bound = 31337;
#endif
	ls->icb_need_increment_bound = false;

	ls->html_file = NULL;
	ls->just_jumped = false;
	ls->end_branch_early = false;

	lsprintf(ALWAYS, "welcome to landslide.\n");

	return ls;
}

/******************************************************************************
 * pebbles system calls
 ******************************************************************************/

static void check_test_case_magics(struct ls_state *ls)
{
	const char *hint = ls->save.stats.total_jumps == 0 ?
		"Your " TEST_CASE "() has a deterministic bug!" :
		"Your " TEST_CASE "() is not threadsafe!";
#ifdef USER_MAGIC_GLOBAL_RESULT
	unsigned int magic_value  = READ_MEMORY(ls->cpu0, (unsigned int)USER_MAGIC_GLOBAL_VALUE);
	unsigned int magic_result = READ_MEMORY(ls->cpu0, (unsigned int)USER_MAGIC_GLOBAL_RESULT);
	if (magic_value != magic_result) {
		FOUND_A_BUG(ls, "I expected magic_global_value (%u) == "
			    "magic_global_expected_result (%u), but they were "
			    "different. %s", magic_result, magic_value, hint);
	}
#endif
#ifdef USER_MAGIC_LOCAL_RESULT
	unsigned int magic_parent = READ_MEMORY(ls->cpu0, (unsigned int)USER_MAGIC_LOCAL_PARENT);
	unsigned int magic_child  = READ_MEMORY(ls->cpu0, (unsigned int)USER_MAGIC_LOCAL_CHILD);
	unsigned int magic_sum    = READ_MEMORY(ls->cpu0, (unsigned int)USER_MAGIC_LOCAL_RESULT);
	if (magic_parent + magic_child != magic_sum) {
		FOUND_A_BUG(ls, "I expected magic_thread_local_value_parent (%u) + "
			    "magic_thread_local_value_child (%u) == "
			    "magic_expected_sum (%u), but the sum is wrong! %s",
			    magic_parent, magic_child, magic_sum, hint);
	}
#endif
#ifdef USER_MAGIC_SUM_MINUS_RESULT
	// this mode is used for cmpxchg test, where the resulting values in
	// global and the resulting sum of parent+child have two legal outcomes,
	// but the difference between the two must always equal the start value.
	unsigned int magic_value  = READ_MEMORY(ls->cpu0, (unsigned int)USER_MAGIC_GLOBAL_VALUE);
	unsigned int magic_parent = READ_MEMORY(ls->cpu0, (unsigned int)USER_MAGIC_LOCAL_PARENT);
	unsigned int magic_child  = READ_MEMORY(ls->cpu0, (unsigned int)USER_MAGIC_LOCAL_CHILD);
	unsigned int magic_minus  = READ_MEMORY(ls->cpu0, (unsigned int)USER_MAGIC_SUM_MINUS_RESULT);
	if (magic_parent + magic_child - magic_value != magic_minus) {
		FOUND_A_BUG(ls, "I expected magic_thread_local_value_parent (%u) + "
			    "magic_thread_local_value_child (%u) - "
			    "magic_global_value (%u) == "
			    "magic_expected_sum_minus_result (%u), but the "
			    "difference is wrong! %s", magic_parent, magic_child,
			    magic_value, magic_minus, hint);
	}
#endif
}

#define CASE_SYSCALL(num, name) \
	case (num): printf(CHOICE, "%s()\n", (name)); break

static void check_user_syscall(struct ls_state *ls)
{
	if (ls->instruction_text[0] != OPCODE_INT) {
		if (ls->sched.cur_agent->most_recent_syscall != 0) {
			mem_check_syscall_return(ls,
				ls->sched.cur_agent->most_recent_syscall);
		}
		ls->sched.cur_agent->most_recent_syscall = 0;
		return;
	}

	lsprintf(CHOICE, "TID %d makes syscall ", ls->sched.cur_agent->tid);

	unsigned int number = OPCODE_INT_ARG(ls->cpu0, ls->eip);
	switch(number) {
		CASE_SYSCALL(FORK_INT, "fork");
		CASE_SYSCALL(EXEC_INT, "exec");
		CASE_SYSCALL(WAIT_INT, "wait");
		CASE_SYSCALL(YIELD_INT, "yield");
		CASE_SYSCALL(DESCHEDULE_INT, "deschedule");
		CASE_SYSCALL(MAKE_RUNNABLE_INT, "make_runnable");
		CASE_SYSCALL(GETTID_INT, "gettid");
		CASE_SYSCALL(NEW_PAGES_INT, "new_pages");
		CASE_SYSCALL(REMOVE_PAGES_INT, "remove_pages");
		CASE_SYSCALL(SLEEP_INT, "sleep");
		CASE_SYSCALL(GETCHAR_INT, "getchar");
		CASE_SYSCALL(READLINE_INT, "readline");
		CASE_SYSCALL(PRINT_INT, "print");
		CASE_SYSCALL(SET_TERM_COLOR_INT, "set_term_color");
		CASE_SYSCALL(SET_CURSOR_POS_INT, "set_cursor_pos");
		CASE_SYSCALL(GET_CURSOR_POS_INT, "get_cursor_pos");
		CASE_SYSCALL(THREAD_FORK_INT, "thread_fork");
		CASE_SYSCALL(GET_TICKS_INT, "get_ticks");
		CASE_SYSCALL(MISBEHAVE_INT, "misbehave");
		CASE_SYSCALL(HALT_INT, "halt");
		CASE_SYSCALL(LS_INT, "ls");
		CASE_SYSCALL(TASK_VANISH_INT, "task_vanish");
		CASE_SYSCALL(SET_STATUS_INT, "set_status");
		CASE_SYSCALL(VANISH_INT, "vanish");
		CASE_SYSCALL(CAS2I_RUNFLAG_INT, "cas2i_runflag");
		CASE_SYSCALL(SWEXN_INT, "swexn");
		default:
			printf(CHOICE, "((unknown 0x%x))\n", number);
			break;
	}

	// XXX: gross hack for PSU to check the __landslide_magics at the right
	// time -- if we check them at test end, the kernel may zero them out
	if (number == VANISH_INT &&
	    ls->sched.num_agents == ls->test.start_population + 1) {
		check_test_case_magics(ls);
	}

	ls->sched.cur_agent->most_recent_syscall = number;
}
#undef CASE_SYSCALL

#define TRIPLE_FAULT_EXCEPTION 1024 /* special simics value */

static const char *exception_names[] = {
	"divide error",
	"single-step exception",
	"nmi",
	"breakpoint",
	"overflow",
	"bounds check",
	"invalid opcode",
	"coprocessor not available",
	"double fault",
	"coprocessor segment overrun",
	"invalid tss",
	"segment not present",
	"stack segment exception",
	"general protection fault",
	"page fault",
	"<reserved>",
	"coprocessor error",
};

static void check_exception(struct ls_state *ls, int number)
{
	if (number < ARRAY_SIZE(exception_names)) {
		ls->sched.cur_agent->most_recent_syscall = number;
		lsprintf(CHOICE, "Exception #%d (%s) taken at ",
			 number, exception_names[number]);
		print_eip(CHOICE, ls->eip);
		printf(CHOICE, "\n");
	} else if (number == TRIPLE_FAULT_EXCEPTION) {
		FOUND_A_BUG(ls, "Triple fault!");
	} else {
		lsprintf(INFO, "Exception #%d (syscall or interrupt) @ ", number);
		print_eip(INFO, ls->eip);
		printf(INFO, "\n");
	}
}

/******************************************************************************
 * miscellaneous bug detection metrics
 ******************************************************************************/

/* How many transitions deeper than the average branch should this branch be
 * before we call it stuck? */
#define PROGRESS_DEPTH_FACTOR 20
/* Before this many branches are explored, we are less confident in the above
 * number, and will scale it up by this exponent for each lacking branch. */
#define PROGRESS_CONFIDENT_BRANCHES 10
#define PROGRESS_BRANCH_UNCERTAINTY_EXPONENT 1.1
/* How many times more instructions should a given transition be than the
 * average previous transition before we proclaim it stuck? */
#define PROGRESS_TRIGGER_FACTOR 4000
#define PROGRESS_AGGRESSIVE_TRIGGER_FACTOR 2000

#ifdef PREEMPT_EVERYWHERE
#define TOO_DEEP_0TH_BRANCH (1<<20)
#else
#define TOO_DEEP_0TH_BRANCH 4000
#endif

/* Avoid getting owned by DR PPs on e.g. memset which hose the average.
 * idk really what a good value for this is, but at least 100 is too small. */
#define PROGRESS_MIN_TRIGGER_AVERAGE ((uint64_t)1000)

static void wrong_panic_html(void *env, struct fab_html_env *html_env)
{
	HTML_PRINTF(html_env, "This shouldn't happen. This is more likely a "
		    "problem with the test configuration, or a reference "
		    "kernel bug, than a bug in your own code." HTML_NEWLINE);
}

static void wrong_panic(struct ls_state *ls, const char *panicked, const char *expected)
{
	lsprintf(BUG, COLOUR_BOLD COLOUR_YELLOW "********************************\n");
	lsprintf(BUG, COLOUR_BOLD COLOUR_YELLOW
		 "The %s panicked during a %sspace test. This shouldn't happen.\n",
		 panicked, expected);
	lsprintf(BUG, COLOUR_BOLD COLOUR_YELLOW
		 "This is more likely a problem with the test configuration,\n");
	lsprintf(BUG, COLOUR_BOLD COLOUR_YELLOW
		 "or a reference kernel bug, than a bug in your code.\n");
	lsprintf(BUG, COLOUR_BOLD COLOUR_YELLOW "********************************\n");
	char buf[BUF_SIZE];
	int len = scnprintf(buf, BUF_SIZE,
			    "Unexpected %s panic during %sspace test",
			    panicked, expected);
	FOUND_A_BUG_HTML_INFO(ls, buf, len, NULL, wrong_panic_html);
	assert(0 && "wrong panic");
}

static bool check_infinite_loop(struct ls_state *ls, char *message, unsigned int maxlen)
{
	/* Condition for stricter check. Less likely long-running non-infinite
	 * loops within user sync primitives, so be more aggressive checking
	 * there. However, the past instruction average must be non-0. */
	bool possible_to_check = ls->save.current != NULL &&
		ls->save.stats.total_triggers != 0;
	bool more_aggressive_check = possible_to_check && testing_userspace() &&
		(ls->save.stats.total_jumps == 0 || /* don't get owned on 0th branch */
		 IN_USER_SYNC_PRIMITIVES(ls->sched.cur_agent));

	/* Can't check for tight loops 0th branch. If one transition has an
	 * expensive operation like vm_free_pagedir() we don't want to trip on
	 * it; we want to incorporate it into the average. */
	if (ls->save.stats.total_jumps == 0 && !possible_to_check) {
		return false;
	}

	/* Have we been spinning since the last choice point? */
	unsigned long most_recent =
		ls->trigger_count - ls->save.current->trigger_count;
	unsigned long average_triggers =
		MAX(ls->save.stats.total_triggers / ls->save.stats.total_choices,
		    PROGRESS_MIN_TRIGGER_AVERAGE);
	unsigned long trigger_factor = more_aggressive_check ?
		PROGRESS_AGGRESSIVE_TRIGGER_FACTOR : PROGRESS_TRIGGER_FACTOR;
	unsigned long trigger_thresh = average_triggers * trigger_factor;

	/* print a message at 1% increments */
	if (most_recent > 0 && most_recent % (trigger_thresh / 100) == 0) {
		lsprintf(CHOICE, "progress sense%s: %lu%% (%lu/%lu)\n",
			 more_aggressive_check ? " (aggressive)" : "",
			 most_recent * 100 / trigger_thresh, most_recent, trigger_thresh);
	}

	if (most_recent >= trigger_thresh) {
		scnprintf(message, maxlen, "It's been %lu instructions since "
			  "the last preemption point; but the past average is "
			  "%lu -- I think you're stuck in an infinite loop.",
			  most_recent, average_triggers);
		return true;
	}

	if (ls->save.stats.total_jumps == 0) {
		/* We can't confidently FAB infinite loop here because we don't
		 * know how long a branch is supposed to be. But we can't make
		 * progress either because 1000 simics snapshots is swapping
		 * danger zone on the GHCXX (experimentally determined). */
		if (ls->save.current != NULL) {
			assert(ls->save.current->depth < TOO_DEEP_0TH_BRANCH &&
			       "0th branch too deep! Aborting to avoid swapping.");
		}
		return false;
	}

	/* Have we been spinning around a choice point (so this branch would
	 * end up being infinitely deep)? Compute a "depth factor" that
	 * reflects our confidence in how accurate the past average branch
	 * depth was -- the fewer branches explored so far, the less so. */
	long double depth_factor = PROGRESS_DEPTH_FACTOR;
	/* For each branch short of the magic number, become less confident. */
	for (unsigned int i = ls->save.stats.total_jumps;
	     i < PROGRESS_CONFIDENT_BRANCHES; i++) {
		depth_factor *= PROGRESS_BRANCH_UNCERTAINTY_EXPONENT;
	}
	unsigned int average_depth =
		ls->save.stats.depth_total / (1 + ls->save.stats.total_jumps);
	unsigned long depth_thresh = average_depth * depth_factor;
	/* we're right on top of the PP; if it's a DR PP, avoid emitting a bogus
	 * stack-address value as current eip (see dr eip logic in save.c). */
	bool during_dr_delay = ls->save.current->eip == ls->eip &&
	                       ls->save.current->data_race_eip != ADDR_NONE;
	if (ls->save.current->depth > depth_thresh && !during_dr_delay) {
		scnprintf(message, maxlen, "This interleaving has at least %d "
			  "preemption-points; but past branches on average were "
			  "only %d deep -- I think you're stuck in an infinite "
			  "loop.", ls->save.current->depth, average_depth);
		return true;
	}

	return false;
}

struct undesirable_loop_env { struct ls_state *ls; char *message; };

static void undesirable_loop_html(void *env, struct fab_html_env *html_env)
{
	struct ls_state *ls = ((struct undesirable_loop_env *)env)->ls;
	char *message       = ((struct undesirable_loop_env *)env)->message;
	HTML_PRINTF(html_env, "%s" HTML_NEWLINE, message);
	if (IN_USER_SYNC_PRIMITIVES(ls->sched.cur_agent)) {
		HTML_PRINTF(html_env, HTML_NEWLINE HTML_BOX_BEGIN);
		HTML_PRINTF(html_env, "<b>NOTE: I have run a loop in %s() an "
			    "alarming number of times." HTML_NEWLINE,
			    USER_SYNC_ACTION_STR(ls->sched.cur_agent));
#ifdef PSU
		HTML_PRINTF(html_env, "Landslide cannot tell if this is an "
			    "infinite loop, or a synchronization" HTML_NEWLINE);
		HTML_PRINTF(html_env, "with another thread. Please check the "
			    "indicated line of code, and if it's " HTML_NEWLINE);
		HTML_PRINTF(html_env, "not a bug, add a 'yield()' call within "
			    "the loop to help Landslide out. " HTML_NEWLINE);
#else
		HTML_PRINTF(html_env, "This version of Landslide cannot "
			    "distinguish between this loop " HTML_NEWLINE);
		HTML_PRINTF(html_env, "being infinite versus "
			    "undesirable." HTML_NEWLINE);
		HTML_PRINTF(html_env, "Please refer to the \"Synchronization "
			    "(2)\" lecture." HTML_NEWLINE);
#endif
		HTML_PRINTF(html_env, HTML_BOX_END HTML_NEWLINE);
	}
}

static bool ensure_progress(struct ls_state *ls)
{
	char *buf;
	unsigned int tid = ls->sched.cur_agent->tid;
	char message[BUF_SIZE];

	if (kern_panicked(ls->cpu0, ls->eip, &buf)) {
		if (testing_userspace()) {
			wrong_panic(ls, "kernel", "user");
		} else {
			FOUND_A_BUG(ls, "KERNEL PANIC: %s", buf);
		}
		MM_FREE(buf);
		return false;
	} else if (user_panicked(ls->cpu0, ls->eip, &buf) &&
		   check_user_address_space(ls) &&
		   !(TID_IS_INIT(tid) || TID_IS_SHELL(tid) || TID_IS_IDLE(tid))) {
		if (testing_userspace()) {
			FOUND_A_BUG(ls, "USERSPACE PANIC: %s", buf);
		} else {
			wrong_panic(ls, "user", "kernel");
		}
		MM_FREE(buf);
		return false;
	} else if (user_report_end_fail(ls->cpu0, ls->eip) &&
		   check_user_address_space(ls) &&
		   !(TID_IS_INIT(tid) || TID_IS_SHELL(tid) || TID_IS_IDLE(tid))) {
		FOUND_A_BUG(ls, "User test program reported failure!");
		return false;
	} else if (kern_page_fault_handler_entering(ls->eip)) {
		unsigned int from_eip = READ_STACK(ls->cpu0, 1);
		unsigned int from_cs  = READ_STACK(ls->cpu0, 2);
		unsigned int cr2 = GET_CR2(ls->cpu0);
		lsprintf(DEV, "page fault on address 0x%x from ", cr2);
		print_eip(DEV, from_eip);
		printf(DEV, "\n");
		/* did it come from kernel-space? */
		if (from_cs == SEGSEL_KERNEL_CS) {
			if (!testing_userspace()) {
				lsprintf(BUG, COLOUR_BOLD COLOUR_YELLOW "Note: "
					 "If kernel page faults are expected "
					 "behaviour, unset PAGE_FAULT_WRAPPER "
					 "in config.landslide.\n");
			}
			FOUND_A_BUG(ls, "Kernel page faulted! Faulting eip: "
				    "0x%x; address: 0x%x", from_eip, cr2);
			return false;
		} else {
			ls->sched.cur_agent->last_pf_eip = from_eip;
			ls->sched.cur_agent->last_pf_cr2 = cr2;
			return true;
		}
	} else if (kern_killed_faulting_user_thread(ls->cpu0, ls->eip)) {
#ifdef PINTOS_KERNEL
		if (true) {
#else
		if (testing_userspace()) {
#endif
			int exn_num = ls->sched.cur_agent->most_recent_syscall;
			unsigned int pf_eip = ls->sched.cur_agent->last_pf_eip;
			if (exn_num == 0 || pf_eip != ADDR_NONE) {
				if (pf_eip == ADDR_NONE) {
					lsprintf(DEV, COLOUR_BOLD COLOUR_YELLOW
						 "Warning: MRS = 0 during "
						 "CAUSE_FAULT. Probably bug or "
						 "annotation error?\n");
					FOUND_A_BUG(ls, "TID %d was killed!\n",
						    ls->sched.cur_agent->tid);
				} else {
					unsigned int pf_cr2 =
						ls->sched.cur_agent->last_pf_cr2;
					FOUND_A_BUG(ls, "TID %d was killed by "
						    "a page fault! (Faulting "
						    "eip: 0x%x; addr: 0x%x)\n",
						    ls->sched.cur_agent->tid,
						    pf_eip, pf_cr2);
				}
			} if (exn_num >= ARRAY_SIZE(exception_names)) {
				FOUND_A_BUG(ls, "TID %d was killed by a fault! "
					    "(unknown exception #%u)\n",
					    ls->sched.cur_agent->tid, exn_num);
			} else {
				FOUND_A_BUG(ls, "TID %d was killed by a fault! "
					    "(exception #%u: %s)\n",
					    ls->sched.cur_agent->tid, exn_num,
					    exception_names[exn_num]);
			}
			return false;
		} else {
			lsprintf(DEV, "Kernel kills faulting thread %d\n",
				 ls->sched.cur_agent->tid);
			return true;
		}
	} else if (check_infinite_loop(ls, message, BUF_SIZE)) {
		const char *headline = "NO PROGRESS (infinite loop?)";
		lsprintf(BUG, COLOUR_BOLD COLOUR_RED "%s\n", message);
		struct undesirable_loop_env env = { .ls = ls, .message = message };
		FOUND_A_BUG_HTML_INFO(ls, headline, strlen(headline),
				      (void *)(&env), undesirable_loop_html);
		return false;
#ifdef PINTOS_KERNEL
	} else if (ls->eip == GUEST_PRINTF) {
		unsigned int esp = GET_CPU_ATTR(ls->cpu0, esp);
		char *fmt = read_string(
			ls->cpu0, READ_MEMORY(ls->cpu0, esp + WORD_SIZE));
		lsprintf(ALWAYS, COLOUR_BOLD COLOUR_BLUE "klog: %s", fmt);
		if (fmt[strlen(fmt)-1] != '\n') {
			printf(ALWAYS, "\n");
		}
		if (strstr(fmt, "%") != NULL && strstr(fmt, "%")[1] == 's') {
			MM_FREE(fmt);
			fmt = read_string(
				ls->cpu0, READ_MEMORY(ls->cpu0, esp + (2*WORD_SIZE)));
			lsprintf(ALWAYS, COLOUR_BOLD COLOUR_BLUE
				 "klog fmt string: %s\n", fmt);
		}
		MM_FREE(fmt);
		return true;
	} else if (ls->eip == GUEST_DBG_PANIC) {
		unsigned int esp = GET_CPU_ATTR(ls->cpu0, esp);
		char *fmt = read_string(
			ls->cpu0, READ_MEMORY(ls->cpu0, esp + (4*WORD_SIZE)));
		lsprintf(ALWAYS, COLOUR_BOLD COLOUR_RED "panic: %s", fmt);
		if (fmt[strlen(fmt)-1] != '\n') {
			printf(ALWAYS, "\n");
		}
		MM_FREE(fmt);
		return true;
#endif
	} else {
		return true;
	}
}

static bool test_ended_safely(struct ls_state *ls)
{
	/* Anything that would indicate failure - e.g. return code... */

	// XXX: Check is broken. Can't reenable it meaningfully
	// XXX: (at least for pintos). See #181.
	// TODO: Test on pebbles; would "ifndef PINTOS_KERNEL" instead be ok?
#if 0
	// TODO: find the blocks that were leaked and print stack traces for them
	// TODO: the test could copy the heap to indicate which blocks
	// TODO: do some assert analogous to wrong_panic() for this
	if (ls->test.start_kern_heap_size < ls->kern_mem.heap_size) {
		FOUND_A_BUG(ls, "KERNEL MEMORY LEAK (%d bytes)!",
			    ls->kern_mem.heap_size - ls->test.start_kern_heap_size);
		return false;
	} else if (ls->test.start_user_heap_size < ls->user_mem.heap_size) {
		FOUND_A_BUG(ls, "USER MEMORY LEAK (%d bytes)!",
			    ls->user_mem.heap_size - ls->test.start_user_heap_size);
		return false;
	}
#endif

	return true;
}

static void found_no_bug(struct ls_state *ls)
{
	lsprintf(ALWAYS, COLOUR_BOLD COLOUR_GREEN
		 "**** Execution tree explored; you survived! ****\n"
		 COLOUR_DEFAULT);
	PRINT_TREE_INFO(DEV, ls);
	QUIT_SIMULATION(LS_NO_KNOWN_BUG);
}

static void check_should_abort(struct ls_state *ls)
{
	if (should_abort(&ls->mess)) {
		lsprintf(ALWAYS, COLOUR_BOLD COLOUR_YELLOW
			 "**** Abort requested by master process. ****\n"
			 COLOUR_DEFAULT);
		PRINT_TREE_INFO(DEV, ls);
		QUIT_SIMULATION(LS_NO_KNOWN_BUG);
	}
}

void landslide_assert_fail(const char *message, const char *file,
			   unsigned int line, const char *function)
{
	struct ls_state *ls = GET_LANDSLIDE();
	message_assert_fail(&ls->mess, message, file, line, function);
#if defined(ID_WRAPPER_MAGIC) || defined(BOCHS)
	lsprintf(ALWAYS, COLOUR_BOLD COLOUR_RED "%s:%u: %s: "
		 "Assertion '%s' failed.\n", file, line, function, message);
	QUIT_SIMULATION(LS_ASSERTION_FAILED);
#else
	/* Don't send SIGABRT to simics if running under the iterative deepening
	 * wrapper -- simics should quit, not drop into a prompt. */
	__assert_fail(message, file, line, function);
#endif
}

/******************************************************************************
 * main entrypoint and time travel logic
 ******************************************************************************/

static bool time_travel(struct ls_state *ls)
{
	/* find where we want to go in the tree, and choose what to do there */
	unsigned int tid;
	bool txn;
	unsigned int xabort_code = _XBEGIN_STARTED; /* illegal value */
	struct abort_set aborts;
	struct nobe *h = explore(ls, &tid, &txn, &xabort_code, &aborts);

	lsprintf(BRANCH, COLOUR_BOLD COLOUR_GREEN "End of branch #%" PRIu64
		 ".\n" COLOUR_DEFAULT, ls->save.stats.total_jumps + 1);
	print_estimates(ls);
	lsprintf(BRANCH, "ICB preemption count this branch = %u\n",
		 ls->sched.icb_preemption_count);
	check_should_abort(ls);

	// TODO: revamp for boxes
	if (h != NULL) {
		assert(!h->all_explored);
		save_longjmp(&ls->save, ls, h, tid, txn, xabort_code, &aborts);
		return true;
	} else if (ls->icb_need_increment_bound) {
		lsprintf(ALWAYS, COLOUR_BOLD COLOUR_YELLOW "ICB bound %u "
			 "wasn't enough: trying again with %u...\n",
			 ls->icb_bound, ls->icb_bound + 1);
		ls->icb_bound++;
		ls->icb_need_increment_bound = false;
		save_reset_tree(&ls->save, ls);
		return true;
	} else {
		return false;
	}
}

static void check_test_state(struct ls_state *ls)
{
	/* When a test case finishes, break the simulation so the wrapper can
	 * decide what to do. */
	if ((test_update_state(ls) && !ls->test.test_is_running) || ls->end_branch_early) {
		ls->end_branch_early = false;
		/* See if it's time to try again... */
		if (ls->test.test_ever_caused) {
			lsprintf(DEV, "test case ended!\n");

			if (DECISION_INFO_ONLY != 0) {
				DUMP_DECISION_INFO(ls);
			} else if (test_ended_safely(ls)) {
				save_setjmp(&ls->save, ls, TID_NONE, true, true,
					    false, ADDR_NONE, false, TID_NONE,
					    false, false, false);
				if (!time_travel(ls)) {
					found_no_bug(ls);
				}
			}
		} else {
#ifdef BOCHS
			cause_test(ls->kbd0, &ls->test, ls, TEST_CASE);
#else
			lsprintf(DEV, "ready to roll!\n");
			BREAK_SIMULATION();
#endif
		}
	} else {
		ensure_progress(ls);
	}
}

/* Main entry point. Called every instruction, data access, and extensible. */
void landslide_entrypoint(struct ls_state *ls, struct trace_entry *entry)
{
	ls->eip = GET_CPU_ATTR(ls->cpu0, eip);

#ifdef BOCHS
	if (ls->eip < 0x100000) {
		return;
	} else
#endif
	if (entry->type == TRACE_MEMORY) {
		if (ls->just_jumped) {
			/* stray access associated with the last instruction
			 * of a past branch. at this point the rest of our
			 * state has already been rewound, so it's too late to
			 * record the access where/when it belongs. */
			return;
		}
		assert(ls->last_instr_eip != 0);
#ifdef BOCHS
		/* bochs updates eip to the next instruction before tracing
		 * memory accesses, which would screw up data race PPs. */
		ls->eip = ls->last_instr_eip;
#else
		assert(ls->eip == ls->last_instr_eip);
#endif
		/* mem access - do heap checks, whether user or kernel */
		mem_check_shared_access(ls, entry->pa, entry->va, entry->write);
	} else if (entry->type == TRACE_EXCEPTION) {
		check_exception(ls, entry->exn_number);
	} else if (entry->type == TRACE_INSTRUCTION) {
		ls->last_instr_eip = ls->eip;
		memcpy(ls->instruction_text, entry->instruction_text,
		       TRACE_OPCODES_LEN);

		if (USER_MEMORY(ls->eip)) {
			check_user_syscall(ls);
		}
		ls->trigger_count++;
		ls->absolute_trigger_count++;

		if (ls->just_jumped) {
			sched_recover(ls);
			ls->just_jumped = false;
		}

		/* NB. mem update must come first because sched update contains
		 * the logic to create PPs, and snapshots must include state
		 * machine changes from mem update (tracking malloc/free). */
		mem_update(ls);
		sched_update(ls);
		check_test_state(ls);
	}
}
