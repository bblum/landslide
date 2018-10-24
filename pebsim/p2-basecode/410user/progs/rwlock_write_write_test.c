/** @file 410user/progs/rwlock_write_write_test.c
 *  @author bblum
 *  @brief tests for write/write mutual exclusivity of rwlocks
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

DEF_TEST_NAME("rwlock_write_write_test:");

#define STACK_SIZE 4096

#define ERR REPORT_FAILOUT_ON_ERR

mutex_t little_lock; /* used to avoid spurious data race reports */
rwlock_t lock;
static int num_in_section = 0;
static int num_finished = 0;

int critical_section()
{
	mutex_lock(&little_lock);
	num_in_section++;
	mutex_unlock(&little_lock);
	yield(-1);
	mutex_lock(&little_lock);
	assert(num_in_section == 1 &&
	       "long is the way, and hard, that out of hell leads up to light");
	num_in_section--;
	num_finished++;
	int result = num_finished == 2;
	mutex_unlock(&little_lock);
	return result;
}

void *writer(void *dummy)
{
	ERR(swexn(NULL, NULL, NULL, NULL));

	rwlock_lock(&lock, RWLOCK_WRITE);
	int done = critical_section();
	rwlock_unlock(&lock);

	if (done) {
		report_end(END_SUCCESS);
	}
	// bypass thr_exit
	vanish();
	return NULL;
}

int main(void)
{
	report_start(START_CMPLT);
	misbehave(BGND_BRWN >> FGND_CYAN);

	ERR(thr_init(STACK_SIZE));

	// turn off autostack growth; if we fault, we want to fail immediately
	// (and not confuse landslide with recursive mutex_lock() calls)
	ERR(swexn(NULL, NULL, NULL, NULL));

	ERR(mutex_init(&little_lock));
	ERR(rwlock_init(&lock));
	ERR(thr_create(writer, NULL));
	yield(-1);
	writer(NULL);

	return 0;
}
