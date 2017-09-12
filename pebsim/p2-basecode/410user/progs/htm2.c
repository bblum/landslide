/** @file 410user/progs/htm2.c
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

DEF_TEST_NAME("htm2:");

#define ERR REPORT_FAILOUT_ON_ERR

mutex_t lock;
volatile static int count = 0;
/* http://www.contrib.andrew.cmu.edu/~mdehesaa/ */
volatile int stop_the_world = 0;

void txn()
{
	int status;
	if ((status = _xbegin()) == _XBEGIN_STARTED) {
		if (stop_the_world != 0) {
			_xabort(0);
		}
		count++;
		_xend();
	} else {
		mutex_lock(&lock);
		stop_the_world = 1;
		count++;
		stop_the_world = 0;
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
