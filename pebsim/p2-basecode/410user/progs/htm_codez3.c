/** @file 410user/progs/htm_codez3.c
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

DEF_TEST_NAME("htm-codez3:");

#define ERR REPORT_FAILOUT_ON_ERR

mutex_t lock;
static int count = 0;

void *txn(void *dummy)
{
	int status;
	if ((status = _xbegin()) == _XBEGIN_STARTED) {
		_xend();
	} else {
		assert(status != _XABORT_CONFLICT);
		// assert(0); // if using -S
	}
	// this data race should not be considered part of the xbegin above
	count++;
	return NULL;
}

int main(void)
{
	report_start(START_CMPLT);

	ERR(thr_init(4096));
	ERR(mutex_init(&lock));
	misbehave(BGND_BRWN >> FGND_CYAN); // for landslide

	int tid = thr_create(txn, NULL);
	ERR(tid);

	txn(NULL);
	ERR(thr_join(tid, NULL));

	return 0;
}
