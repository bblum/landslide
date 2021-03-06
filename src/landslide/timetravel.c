/**
 * @file timetravel.c
 * @brief bochs implementation of timetravel using fork
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

#include <string.h>
#include <unistd.h>

#define MODULE_NAME "D-MAIL"
#define MODULE_COLOUR COLOUR_DARK COLOUR_RED

#include "common.h"
#include "landslide.h"
#include "save.h"
#include "simulator.h"
#include "timetravel.h"
#include "tree.h"
#include "tsx.h"

#ifdef BOCHS

enum timetravel_message_tag {
	TIMETRAVEL_MODIFY_HAX,
	TIMETRAVEL_RUN,
};

#define TIMETRAVEL_MAGIC 0x44664499

struct timetravel_global_message {
	unsigned int magic;
	unsigned int exit_code;
};

struct timetravel_message {
	enum timetravel_message_tag tag;
	unsigned int magic;

	/* used for tag modify nobe only */
	void (*cb)(struct nobe *h_rw, void *arg);
	const struct nobe *h_ro;
	unsigned int h_depth;
	unsigned int arg_size; /* arg will be sent in a separate message */

	/* used for tag run only */
	unsigned int tid;
	bool txn;
	unsigned int xabort_code;
	struct abort_set aborts;
	struct save_statistics save_stats;
	unsigned int icb_bound;
	bool icb_need_increment_bound;
};

#define QUIT_BOCHS(v) do { ls_safe_exit = true; BX_EXIT(v); assert(0); } while (0)

static bool timetravel_inited = false;
bool active_world_line = true;
bool ls_safe_exit = false;

/* because setting up timetravel at each PP involves creating a new process,
 * to manage exit code and wait()ing by any parent process (whether shell or
 * quicksand) we first dedicate the original process to collect said code... */
void timetravel_init(struct timetravel_state *ts)
{
	int pipefd[2];
	int ret = pipe(pipefd);
	assert(ret == 0 && "failed pipe for global exit status retrieval");
	ts->pipefd = pipefd[1];

	assert(!timetravel_inited && "can't be called twice");
	timetravel_inited = true;

	int child_tid = fork();
	if (child_tid != 0) {
		/* parent process for collecting the exit code */
		assert(child_tid > 0 && "failed fork");
		active_world_line = false;
		struct timetravel_global_message gm;
		int ret = read(pipefd[0], &gm, sizeof(gm));
		assert(ret == sizeof(gm) && "failed to collect exit status");
		assert(gm.magic == TIMETRAVEL_MAGIC && "bad magic");
		QUIT_BOCHS(gm.exit_code);
	}
}

/* ...accordingly, any process which "exits" landslide must send the code. */
void quit_landslide(unsigned int code)
{
	if (timetravel_inited) {
		struct timetravel_state *ts = &GET_LANDSLIDE()->timetravel;
		struct timetravel_global_message gm;
		gm.magic     = TIMETRAVEL_MAGIC;
		gm.exit_code = code;
		int ret = write(ts->pipefd, &gm, sizeof(gm));
		/* don't assert in the function that implements assert, dummy */
		// assert(ret == sizeof(gm) && "failed write");
	}
	QUIT_BOCHS(code);
}

/* called by modify_pp to send changes to the forked processes */
void __modify_pps(void (*cb)(struct nobe *h_rw, void *), const struct nobe *h_ro,
		    void *arg, unsigned int arg_size)
{
	/* prevent recursive calls of this nobe-modification procedure by child
	 * processes. the actively-landsliding process is alone responsible for
	 * sending update messages to all dormant timetravel nobes. */
	assert(active_world_line && "recursive nobe tree changes forbidden");
	struct timetravel_message tm;
	tm.tag      = TIMETRAVEL_MODIFY_HAX;
	tm.magic    = TIMETRAVEL_MAGIC;
	tm.cb       = cb;
	tm.h_ro     = h_ro;
	tm.h_depth  = h_ro->depth;
	tm.arg_size = arg_size;
	/* send message to all processes in between current and target nobe; they
	 * all have a target nobe in their local memory that needs modified. */
	for (const struct nobe *ancestor = GET_LANDSLIDE()->save.current;
	     ancestor != h_ro->parent; ancestor = ancestor->parent) {
		assert(ancestor != NULL && "h_ro not an ancestor of current?");
		assert(ancestor->time_machine.active);
		assert(ancestor->time_machine.parent);
		int ret = write(ancestor->time_machine.pipefd, &tm, sizeof(tm));
		assert(ret == sizeof(tm) && "failed write to modify nobes");
		ret = write(ancestor->time_machine.pipefd, arg, arg_size);
		assert(ret == arg_size && "failed write arg to modify nobes");
	}
}

/* returns false if the parent process; true if the (just-jumped-to) child, in
 * which case the output pointers will be set to what thread to run instead,
 * whereupon this must be re-called to refresh the save for another jump. */
bool timetravel_set(struct ls_state *ls, struct nobe *h,
		    unsigned int *tid, bool *txn, unsigned int *xabort_code,
		    struct abort_set *aborts)
{
	struct timetravel_pp *th = &h->time_machine;
	int pipefd[2];

	assert(!th->active);
	assert(active_world_line && "not to be called from a timetravel child!");
	th->active = true;

	int ret = pipe(pipefd);
	if (ret != 0) {
		if (errno == EMFILE) {
			char msg[BUF_SIZE];
			if (ls->test.current_test != NULL &&
			    0 == strcmp(ls->test.current_test,
			                "alarm-simultaneous")) {
				scnprintf(msg, BUF_SIZE, "ran out of file "
					  "descriptors at the %dth PP on this "
					  "branch; are you sure thread_yield "
					  "is implemented?\n", h->depth);
			} else {
				scnprintf(msg, BUF_SIZE, "ran out of file "
					  "descriptors at %dth PP\n", h->depth);
			}
			landslide_assert_fail(msg, __FILE__, __LINE__, __func__);
		} else {
			assert(0 && "pipe failed for a mysterious reason!");
		}
	}

	int child_tid = fork();
	if (child_tid == -1) {
		char msg[BUF_SIZE];
		scnprintf(msg, BUF_SIZE, "fork failed at #%d/tid%d: errno %d (%s)",
			  h->depth, h->chosen_thread, errno, strerror(errno));
		landslide_assert_fail(msg, __FILE__, __LINE__, __func__);
	} else if (child_tid != 0) {
		/* parence process */
		th->parent = true;
		th->pipefd = pipefd[1];
		close(pipefd[0]);
		return false;
	}

	/* child process */
	th->parent = false;
	th->pipefd = pipefd[0];
	close(pipefd[1]);
	active_world_line = false;

	struct timetravel_message tm;
	while ((ret = read(th->pipefd, &tm, sizeof(tm))) == sizeof(tm)) {
		assert(tm.magic == TIMETRAVEL_MAGIC && "bad magic");
		if (tm.tag == TIMETRAVEL_RUN) {
			active_world_line = true;
			/* receive "glowing green" state from previous line */
			memcpy(&ls->save.stats, &tm.save_stats,
			       sizeof(struct save_statistics));
			if (ls->icb_bound == tm.icb_bound) {
				/* normal jump within same ICB bound; expect
				 * "need increment" flag to be monotonic */
				assert(!ls->icb_need_increment_bound ||
				       tm.icb_need_increment_bound);
				ls->icb_need_increment_bound =
					tm.icb_need_increment_bound;
			} else {
				/* special ICB tree-reset jump */
				assert(h->depth == 0);
				assert(!tm.icb_need_increment_bound);
				assert(!ls->icb_need_increment_bound);
				ls->icb_bound = tm.icb_bound;
			}
			/* refresh for a future jump from this process */
			th->active = false;
			close(th->pipefd);
			/* indicate what to do */
			*tid = tm.tid;
			*txn = tm.txn;
			*xabort_code = tm.xabort_code;
			*aborts = tm.aborts;
			return true;
		} else if (tm.tag == TIMETRAVEL_MODIFY_HAX) {
			/* check sanity of the nobe we're asked to modify */
			assert(tm.h_depth <= h->depth && "nobe from the future");
			for (const struct nobe *h2 = h; h2 != tm.h_ro;
			     h2 = h2->parent) {
				assert(h2 != NULL && "h_ro not an ancestor?");
			}
			/* get the argument */
			char arg[tm.arg_size];
			ret = read(th->pipefd, arg, tm.arg_size);
			assert(ret == tm.arg_size && "failed read arg");
			/* update our local version of this nobe */
			tm.cb((struct nobe *)tm.h_ro, arg);
		} else {
			assert(0 && "bad message tag");
		}
	}

	/* got here? expect pipe closed; delete this timetravel nobe */
	assert(ret == 0 && "child process failed to read from pipe");
	/* parence process must have crashed; not our business; quit silently */
	QUIT_BOCHS(LS_NO_KNOWN_BUG);
}

void timetravel_jump(struct ls_state *ls, const struct timetravel_pp *th,
		     unsigned int tid, bool txn, unsigned int xabort_code,
		     struct abort_set *aborts)
{
	assert(th->active);
	assert(th->parent);
	assert(active_world_line && "not to be called from a timetravel child!");
	lsprintf(CHOICE, "tt'ing to tid %d txn %d code %d\n", tid, txn, xabort_code);
	if (ABORT_SET_ACTIVE(aborts)) {
		lsprintf(CHOICE, "tt'ing abort set: ");
		print_abort_set(CHOICE, aborts);
		printf(CHOICE, "\n");
	}

	struct timetravel_message tm;
	tm.tag         = TIMETRAVEL_RUN;
	tm.magic       = TIMETRAVEL_MAGIC;
	/* info about what to do */
	tm.tid         = tid;
	tm.txn         = txn;
	tm.xabort_code = xabort_code;
	tm.aborts      = *aborts;
	/* anything else that needs to "glow green" */
	memcpy(&tm.save_stats, &ls->save.stats, sizeof(struct save_statistics));
	tm.icb_bound = ls->icb_bound;
	tm.icb_need_increment_bound = ls->icb_need_increment_bound;
	/* send the message */
	int ret = write(th->pipefd, &tm, sizeof(tm));
	assert(ret == sizeof(tm) && "write failed");
	QUIT_BOCHS(LS_NO_KNOWN_BUG);
}

void timetravel_delete(struct ls_state *ls, const struct timetravel_pp *th)
{
	assert(th->active);
	assert(th->parent);
	assert(active_world_line && "not to be called from a timetravel child!");
	close(th->pipefd);
}

#else
#include "timetravel-simics.c"
#endif
