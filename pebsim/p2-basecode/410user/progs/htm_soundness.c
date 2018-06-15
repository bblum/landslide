/** @file 410user/progs/htm_fig61.c
 *  @author bblum
 *  @brief see thesis proposal figure 6.1
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

DEF_TEST_NAME("htm_fig61:");

#define ERR REPORT_FAILOUT_ON_ERR

volatile static int foo = 0;
volatile static int bar = 0;

#define NITERS 3
#define NTHREADS 2

void *txn(void *arg)
{
	int status;
	if ((status = _xbegin()) == _XBEGIN_STARTED) {
		(*((int *)arg))++;
		_xend();
	} else {
		// nb.
		// assert(foo + bar < 2);
		// test for dpor sound treatment of xbegin failure injection
		// this should fail if parent thread gets failure injected
		// after child thread runs. see issue #5 re soundness
		assert(bar == 0);
		// add in a memory conflict on foo too
		// just to make the state space be the same as for htm_fig61.c
		assert(foo >= 0);
	}
	vanish();
	return NULL;
}

int main(void)
{
	report_start(START_CMPLT);

	ERR(thr_init(4096));
	misbehave(BGND_BRWN >> FGND_CYAN); // for landslide

	thr_create(txn, (void *)&bar);
	txn((void *)&foo);

	return 0;
}
