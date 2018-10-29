/** @file 410user/progs/rwlock_dont_starve_writers.c
 *  @author bblum
 *  @brief tests for a very common way of preventing writer starvation
 *         (NB: there may be valid implementations which fail this test, despite
 *         providing weaker, yet still finite, bounds on writer wait time; this
 *         test's bug reports should be confirmed by manual inspection)
 *  @public yes
 *  @for p2
 *  @covers rwlock_lock
 *  @status done
 */

/* Includes */
#include <syscall.h>
#include <stdlib.h>
#include <thread.h>
#include <mutex.h>
//#include <cond.h>
#include <rwlock.h>
#include <assert.h>
#include "410_tests.h"
#include <report.h>
#include <test.h>

DEF_TEST_NAME("rwlock_dont_starve_writers:");

#define STACK_SIZE 4096

#define ERR REPORT_FAILOUT_ON_ERR

rwlock_t lock;
sem_t writer_asleep_sem;
static int got_here = 0;

/* these 2 ignored by landslide using without_function */
void signal_release_read_ok() { sem_signal(&writer_asleep_sem); }
void wait_release_read_ok()   { sem_wait  (&writer_asleep_sem); }

void *reader(void *arg)
{
	int writer_tid = (int)arg;
	ERR(swexn(NULL, NULL, NULL, NULL));

	/* wait for writer to become blocked */
	while (yield(writer_tid) == 0) { continue; }
	/* alert main thread ok to release read lock (this sem ensures the
	 * writer doesn't exit before we even get here, in which case the above
	 * yield loop would simply deadlock waiting for a nonexistent thread) */
	signal_release_read_ok();
	/* try to sneak into rwlock at same time as main thread */
	rwlock_lock(&lock, RWLOCK_READ);
	assert(got_here == 1 && "2nd reader cut in line before waiting writer :<");
	rwlock_unlock(&lock); /* not strictly necessary */
	report_end(END_SUCCESS);

	vanish();
	return NULL;
}

void *writer(void *dummy)
{
	ERR(swexn(NULL, NULL, NULL, NULL));

	rwlock_lock(&lock, RWLOCK_WRITE);
	got_here = 1;
	rwlock_unlock(&lock);

	// bypass thr_exit
	vanish();
	return NULL;
}

int main(void)
{
	int writer_tid;
	report_start(START_CMPLT);
	misbehave(BGND_BRWN >> FGND_CYAN);

	ERR(thr_init(STACK_SIZE));

	// turn off autostack growth; if we fault, we want to fail immediately
	// (and not confuse landslide with recursive mutex_lock() calls)
	ERR(swexn(NULL, NULL, NULL, NULL));

	ERR(rwlock_init(&lock));
	ERR(sem_init(&writer_asleep_sem, 0));

	rwlock_lock(&lock, RWLOCK_READ);
	writer_tid = thr_create(writer, NULL);
	ERR(writer_tid);
	ERR(thr_create(reader, (void *)writer_tid));
	/* wait for writer child to get blocked (as detected by reader child) */
	wait_release_read_ok();
	rwlock_unlock(&lock);

	vanish();
	return 0;
}
