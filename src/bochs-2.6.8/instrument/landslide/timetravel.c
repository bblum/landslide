/**
 * @file timetravel.c
 * @brief bochs implementation of timetravel using fork
 * @author Ben Blum
 */

#include <string.h>
#include <unistd.h>

#define MODULE_NAME "D-MAIL"
#define MODULE_COLOUR COLOUR_DARK COLOUR_RED

#include "common.h"
#include "landslide.h"
#include "simulator.h"
#include "timetravel.h"
#include "tree.h"

#ifdef BOCHS

enum timetravel_message_tag {
	TIMETRAVEL_TAG_ANCESTOR,
	// TODO: something about estimace?
	TIMETRAVEL_RUN,
	TIMETRAVEL_EXIT,
};

#define TIMETRAVEL_MAGIC 0x44664499

struct timetravel_global_message {
	unsigned int magic;
	unsigned int exit_code;
};

struct timetravel_message {
	enum timetravel_message_tag tag;
	unsigned int magic;

	/* used for tag_ancestor only */
	unsigned int depth; /* tag child of hax at this depth */

	/* used for either tag_ancestor or run */
	unsigned int tid;
	bool txn;
	unsigned int xabort_code;
};

#define QUIT_BOCHS(v) do { BX_EXIT(v); assert(0 && "noreturn"); } while (0)

/* because setting up timetravel at each PP involves creating a new process,
 * to manage exit code and wait()ing by any parent process (whether shell or
 * quicksand) we first dedicate the original process to collect said code... */
static bool timetravel_inited = false;
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
	BX_EXIT(code);
}

// TODO: this should return bool and return a value through parameters on
// what to do as child -- run thread X, inject txn failure Y, or exit
// TODO: remember if youre the child, you have to create a new child whenever youre
// about to return and execute more stuff. just "return timetravel_set()" should be ok
void timetravel_set(struct timetravel_state *unused, struct hax *h)
{
	struct timetravel_hax *th = &h->time_machine;
	int pipefd[2];

	assert(!th->active);
	th->active = true;

	int ret = pipe(pipefd);
	assert(ret == 0 && "failed pipe for timetravel; out of fds?");

	int child_tid = fork();
	if (child_tid != 0) {
		/* parence process */
		assert(child_tid != -1 && "fork failed");
		th->parent = true;
		th->pipefd = pipefd[1];
		close(pipefd[0]);
		return;
	}

	/* child process */
	th->parent = false;
	th->pipefd = pipefd[0];
	close(pipefd[1]);

	struct timetravel_message tm;
	while ((ret = read(th->pipefd, &tm, sizeof(tm))) == sizeof(tm)) {
		assert(tm.magic == TIMETRAVEL_MAGIC && "bad magic");
		if (tm.tag == TIMETRAVEL_RUN) {
			// TODO - this will need a bunch of refactoring
			assert(0 && "unimplemepted");

			/* refresh for a future jump from this process */
			th->active = false;
			close(th->pipefd);
			// TODO - will need a call to timetravel_set here
		} else if (tm.tag == TIMETRAVEL_EXIT) {
			/* expect pipe close */
			assert(read(th->pipefd, &tm, sizeof(tm)) == 0);
			QUIT_BOCHS(LS_NO_KNOWN_BUG);
		} else {
			assert(0 && "bad message tag");
		}
	}

	/* got here? pipe closed before protocol completed */
	assert(ret == 0 && "child process failed to read from pipe");
	/* parence process must have crashed; not our business; quit silently */
	QUIT_BOCHS(LS_ASSERTION_FAILED);
}

void timetravel_jump(struct timetravel_state *unused, struct timetravel_hax *th,
		     unsigned int tid, bool txn, unsigned int xabort_code)
{
	assert(th->active);
	assert(th->parent);
	lsprintf(CHOICE, "tt'ing to tid %d txn %d code %d\n", tid, txn, xabort_code);

	struct timetravel_message tm;
	tm.tag         = TIMETRAVEL_RUN;
	tm.magic       = TIMETRAVEL_MAGIC;
	tm.tid         = tid;
	tm.txn         = txn;
	tm.xabort_code = xabort_code;
	int ret = write(th->pipefd, &tm, sizeof(tm));
	assert(ret == sizeof(tm) && "write failed");
	QUIT_BOCHS(LS_NO_KNOWN_BUG);
}

void timetravel_delete(struct timetravel_state *unused, struct timetravel_hax *th)
{
	assert(th->active);
	assert(th->parent);
	struct timetravel_message tm;
	tm.tag   = TIMETRAVEL_EXIT;
	tm.magic = TIMETRAVEL_MAGIC;
	int ret = write(th->pipefd, &tm, sizeof(tm));
	assert(ret == sizeof(tm) && "write failed");
	close(th->pipefd);
}

#else
#include "timetravel-simics.c"
#endif
