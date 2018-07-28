/** @file 410user/progs/htm_fig63.c
 *  @author bblum
 *  @brief like fig61 but generalized for n/k,
 *  and renamed bc in the thesis it's now figure 6.3 instead of 6.1
 *  should test exactly 6 interleavings with optimal retry set reduction (on (2,1))
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

DEF_TEST_NAME("htm_fig63:");

#define ERR REPORT_FAILOUT_ON_ERR

/* many thread version of fig61.c parameterized to have more threads,
 * with same X-shaped conflict pattern b/w txns and aborts */
#define NTHREADS 2
#define NITERS 2

volatile static int foobar[NTHREADS];

void *txn(void *arg)
{
	int status;
	int i = (int)arg;
	for (int j = 0; j < NITERS; j++) {
		if ((status = _xbegin()) == _XBEGIN_STARTED) {
			// doesn't conflict with any other transactions
			// (ok well, ignoring false sharing of cache lines)
			foobar[i]++;
			_xend();
		} else {
			// nb. make sure to conflict with all other threads' txn
			// paths but has reads only here, so doesn't conflict
			// with other abort paths
			int sum = 0;
			for (int k = 0; k < NTHREADS; k++) {
				sum += foobar[k];
			}
			assert(sum < NTHREADS * NITERS);
			// if you wanna make it more realistic, like a data
			// structure which does a consistency check and then
			// adds, but doesn't get as much reduction, try this
			//__sync_fetch_and_add(&foobar[i], 1);
		}
	}
	vanish();
	return NULL;
}

int main(void)
{
	report_start(START_CMPLT);

	ERR(thr_init(4096));
	misbehave(BGND_BRWN >> FGND_CYAN); // for landslide

	for (int i = 0; i < NTHREADS; i++) {
		foobar[i] = 0;
	}
	for (int i = 0; i < NTHREADS; i++) {
		thr_create(txn, (void *)i);
	}
	vanish();
	return 0;
}
