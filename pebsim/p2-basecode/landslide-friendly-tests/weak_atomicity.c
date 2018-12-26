/** @file 410user/progs/weak_atomicity.c
 *  @author bblum
 *  @brief tests nontxns can interleave inside of txns under -W -- should find bug
 *  @public yes
 *  @for p2
 *  @covers lol
 *  @status done
 */

/* Includes */
#include <assert.h>
#include <syscall.h>
#include <stdlib.h>
#include <thread.h>
#include <mutex.h>
#include <cond.h>
#include "410_tests.h"
#include <report.h>
#include <test.h>
#include <htm.h>

DEF_TEST_NAME("weak_atomicity:");

#define ERR REPORT_FAILOUT_ON_ERR

volatile static int count = 0;

void *child(void *dummy)
{
	int status;
	if ((status = _xbegin()) == _XBEGIN_STARTED) {
		count++;
		_xend();
	} else {
		// actually would be ok to be nonatomic in this case
		// bc of causality, i think
		__sync_fetch_and_add(&count, 1);
	}
	return NULL;
}

int main(void)
{
	report_start(START_CMPLT);

	ERR(thr_init(4096));
	misbehave(BGND_BRWN >> FGND_CYAN); // for landslide

	int tid;
	tid = thr_create(child, NULL);
	ERR(tid);
	yield(tid); // FIXME: have landslide automatically put create-exiting and join-entering PPs when TRUSTED_THR_JOIN is enabled, so test cases don't need to put these yield statemence

	__sync_fetch_and_add(&count, 1);
	yield(tid); // FIXME as above
	ERR(thr_join(tid, NULL));
	assert(count == 2);


	return 0;
}
