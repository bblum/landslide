/** @file 410user/progs/htm-deadlock.c
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
#include <thread.h>
#include <mutex.h>
#include <cond.h>
#include "410_tests.h"
#include <report.h>
#include <test.h>
#include <htm.h>

DEF_TEST_NAME("htm-deadlock:");

#define ERR REPORT_FAILOUT_ON_ERR

mutex_t lock;
static int count = 0;

void txn()
{
	int status;
	if ((status = _xbegin()) == _XBEGIN_STARTED) {
		count++;
		// TODO - 
		// omitting xend should not cause a "deadlock" in which the
		// second thread "blocks" forever at xbegin,
		// but rather should force the 1st txn to abort.
		// double bonus TODO: how should we behave if the "last" thread
		// exits w/o xending, without any contender to abort it?
		// _xend();
	} else {
		mutex_lock(&lock);
		count++;
		mutex_unlock(&lock);
	}
}

void *child(void *dummy)
{
	txn();
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
	assert(count == 2);


	return 0;
}
