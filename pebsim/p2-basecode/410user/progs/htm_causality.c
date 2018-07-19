/** @file 410user/progs/htm_causality.c
 *  @author bblum
 *  @brief checks that landslide's _XABORT_CONFLICT injection doesn't violate causality
 *  @public yes
 *  @for p2
 *  @covers lol
 *  @status done
 */

/* Includes */
#include <assert.h>
#include <thread.h>
#include "410_tests.h"
#include <report.h>
#include <test.h>
#include <htm.h>

DEF_TEST_NAME("htm_causality:");

#define ERR REPORT_FAILOUT_ON_ERR

volatile static int count = 0;

void *child(void *dummy)
{
	int status;
retry:
	if ((status = _xbegin()) == _XBEGIN_STARTED) {
		count = 0;
		_xend();
	} else if ((status & _XABORT_RETRY) != 0) {
		assert(0 && "please run with -X -A -S to suppress retries");
		goto retry;
	} else {
		assert((status & _XABORT_CONFLICT) != 0 && "unexpected abort mode");
		// it would be a causality violation for this to be visible to
		// the parent's xadd, which has to happen first for us to abort
		__sync_fetch_and_add(&count, 1);
	}
	return NULL;
}

int main(void)
{
	report_start(START_CMPLT);

	ERR(thr_init(4096));

	int tid = thr_create(child, NULL);
	thr_yield(-1);
	int value = __sync_fetch_and_add(&count, 1);
	assert(value == 0 && "causality violation");

	ERR(thr_join(tid, NULL));

	return 0;
}
