/** @file 410user/progs/htm_codez2.c
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

DEF_TEST_NAME("htm-codez2:");

#define ERR REPORT_FAILOUT_ON_ERR

static int count = 0;

void txn()
{
	int status;
	if ((status = _xbegin()) == _XBEGIN_STARTED) {
		count++;
		_xend();
	} else {
		assert(status == _XABORT_RETRY && "this assert should not trip under any combination of options");
		assert(0 && "this abort should not trip with -X -A -S set; without -S it is expected to trip tho");
	}
}

int main(void)
{
	report_start(START_CMPLT);

	txn();

	return 0;
}
