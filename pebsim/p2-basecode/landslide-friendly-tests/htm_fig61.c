/** @file 410user/progs/htm_fig61.c
 *  @author bblum
 *  @brief see thesis proposal figure 6.1;
 *  should test exactly 6 interleavings with optimal abort set reduction
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

void *txn(void *arg)
{
	int status;
	if ((status = _xbegin()) == _XBEGIN_STARTED) {
		(*((int *)arg))++;
		_xend();
	} else {
		// nb. make sure to conflict with both threads' txn paths
		assert(foo + bar < 2);
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
