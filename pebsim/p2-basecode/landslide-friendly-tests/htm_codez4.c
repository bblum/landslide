/** @file 410user/progs/htm_codez4.c
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

DEF_TEST_NAME("htm-codez4:");

#define ERR REPORT_FAILOUT_ON_ERR

void *txn(void *dummy)
{
	int status;
	if ((status = _xbegin()) == _XBEGIN_STARTED) {
		yield(-1);
		assert(0 && "shouldn't get here");
		_xend();
	} else {
		// test with -X -A -S
		// should complete with 2 interleavings tested (not 1!!!)
		assert(status == _XABORT_CAPACITY);
		// assert(0); // if using -S
	}
	return NULL;
}

int main(void)
{
	report_start(START_CMPLT);

	txn(NULL);

	return 0;
}
