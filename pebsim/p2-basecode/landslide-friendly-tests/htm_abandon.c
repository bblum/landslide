/** @file 410user/progs/htm_abandon.c
 *  @author bblum
 *  @brief program that forces the scheduler to abandon abort sets due to a conflict
 *  see scheduler.c under "ABORT_SET_ACTIVE(upcoming_aborts)"
 *  and run with -X only (not -A -S)
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

DEF_TEST_NAME("htm_abandon:");

#define ERR REPORT_FAILOUT_ON_ERR

volatile static int foo = 0;

void *txn(void *arg)
{
	int status;
	if ((status = _xbegin()) == _XBEGIN_STARTED) {
		_xend();
	} else {
		foo++;
	}
	vanish();
	return NULL;
}

int main(void)
{
	report_start(START_CMPLT);

	ERR(thr_init(4096));
	misbehave(BGND_BRWN >> FGND_CYAN); // for landslide

	thr_create(txn, NULL);
	txn(NULL);

	return 0;
}
