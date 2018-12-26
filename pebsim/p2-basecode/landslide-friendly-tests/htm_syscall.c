/** @file 410user/progs/htm-syscall.c
 *  @author bblum
 *  @brief like htm1 but without the bug
 *  @public yes
 *  @for p2
 *  @covers lol
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

DEF_TEST_NAME("htm-syscall:");

#define ERR REPORT_FAILOUT_ON_ERR

void txn()
{
	int failed = 0;
	int status;
	if ((status = _xbegin()) == _XBEGIN_STARTED) {
		yield(-1);
		assert(0);
	} else {
		failed = 1;
	}
	assert(failed);
}

int main(void)
{
	report_start(START_CMPLT);
	ERR(thr_init(4096));

	txn();

	return 0;
}
