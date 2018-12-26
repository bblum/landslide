/** @file 410user/progs/rwlock_dont_starve_readers.c
 *  @author szz
 *  @note based on rwlock_dont_starve_writers by bblum
 *  @brief tests for a very common way of preventing reader starvation
 *         (NB: there may be valid implementations which fail this test, despite
 *         providing weaker, yet still finite, bounds on reader wait time; this
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
#include <sem.h>
#include <assert.h>
#include "410_tests.h"
#include <report.h>
#include <test.h>

DEF_TEST_NAME("rwlock_dont_starve_readers:");

#define STACK_SIZE 4096

#define ERR REPORT_FAILOUT_ON_ERR

rwlock_t lock;
sem_t reader_asleep_sem;
static int got_here = 0;

/* these 2 ignored by landslide using without_function */
void signal_release_ok() { sem_signal(&reader_asleep_sem); }
void wait_release_ok()   { sem_wait  (&reader_asleep_sem); }

void *writer(void *arg)
{
	int reader_tid = (int)arg;
	ERR(swexn(NULL, NULL, NULL, NULL));

	/* wait for reader to become blocked */
	while (yield(reader_tid) == 0) { continue; }
	/* alert main thread ok to release write lock (this sem ensures the
	 * reader doesn't exit before we even get here, in which case the above
	 * yield loop would simply deadlock waiting for a nonexistent thread) */
	signal_release_ok();
	/* try to sneak into wrlock before reader thread gets a turn */
	rwlock_lock(&lock, RWLOCK_WRITE);
	assert(got_here == 1 && "2nd writer cut in line before waiting reader :<");
	rwlock_unlock(&lock); /* not strictly necessary */
	report_end(END_SUCCESS);

	vanish();
	return NULL;
}

void *reader(void *dummy)
{
	ERR(swexn(NULL, NULL, NULL, NULL));

	rwlock_lock(&lock, RWLOCK_READ);
	got_here = 1;
	rwlock_unlock(&lock);

	// bypass thr_exit
	vanish();
	return NULL;
}

int main(void)
{
	int reader_tid;
	report_start(START_CMPLT);
	misbehave(BGND_BRWN >> FGND_CYAN);

	ERR(thr_init(STACK_SIZE));

	// turn off autostack growth; if we fault, we want to fail immediately
	// (and not confuse landslide with recursive mutex_lock() calls)
	ERR(swexn(NULL, NULL, NULL, NULL));

	ERR(rwlock_init(&lock));
	ERR(sem_init(&reader_asleep_sem, 0));

	rwlock_lock(&lock, RWLOCK_WRITE);
	reader_tid = thr_create(reader, NULL);
	ERR(reader_tid);
	ERR(thr_create(writer, (void *)reader_tid));
	/* wait for reader child to get blocked (as detected by writer child) */
	wait_release_ok();
	rwlock_unlock(&lock);

	vanish();
	return 0;
}
