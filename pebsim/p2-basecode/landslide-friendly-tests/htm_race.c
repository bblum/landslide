/** @file 410user/progs/htm_race.c
 *  @author bblum
 *  @brief tests transactional memory
 *  @public yes
 *  @for p2
 *  @covers cond_wait,cond_broadcast
 *  @status done
 */

/* Includes */
#include <syscall.h>
#include <stdlib.h>
#include <stdbool.h>
#include <thread.h>
#include <mutex.h>
#include <cond.h>
#include "410_tests.h"
#include <report.h>
#include <test.h>
#include <htm.h>

DEF_TEST_NAME("htm-race:");

#define ERR REPORT_FAILOUT_ON_ERR

mutex_t lock;
static bool flag = false;
static int message = 0;

void txn()
{
	int status;
	message = 42;
	if ((status = _xbegin()) == _XBEGIN_STARTED) {
		flag = true;
		_xend();
	} else {
		assert(status == _XABORT_RETRY || status == _XABORT_CONFLICT);
	}
}

void *child(void *dummy)
{
	int status;
	bool the_flag = false;
	if ((status = _xbegin()) == _XBEGIN_STARTED) {
		the_flag = flag;
		_xend();
	} else {
		assert(status == _XABORT_RETRY || status == _XABORT_CONFLICT);
	}
	if (the_flag) {
		// this should identify as a data race with message above
		// only under limited HB, not under pure HB
		assert(message == 42);
	}
	return NULL;
}

int main(void)
{
	report_start(START_CMPLT);

	ERR(thr_init(4096));
	ERR(mutex_init(&lock));
	misbehave(BGND_BRWN >> FGND_CYAN); // for landslide

	int tid = thr_create(child, NULL);
	ERR(tid);

	txn();
	ERR(thr_join(tid, NULL));

	return 0;
}
