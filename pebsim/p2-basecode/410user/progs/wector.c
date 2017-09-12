/** @file 410user/progs/wector.c
 *  @author bblum
 *  @brief tests data races in a landslide-friendly way
 *  @public yes
 *  @for p2
 *  @covers 
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

DEF_TEST_NAME("wector:");

#define STACK_SIZE 4096

#define ERR REPORT_FAILOUT_ON_ERR

mutex_t lock;
static int this_should_race = 0;

void *waiter(void *dummy)
{
	
	this_should_race++;
	mutex_lock(&lock);
	mutex_unlock(&lock);
	return NULL;
}
	

int main(void)
{
	report_start(START_CMPLT);

	ERR(thr_init(STACK_SIZE));
	ERR(mutex_init(&lock));
	misbehave(BGND_BRWN >> FGND_CYAN); // for landslide

	ERR(thr_create(waiter, NULL));
	//ERR(thr_create(waiter, NULL));

	mutex_lock(&lock);
	mutex_unlock(&lock);
	this_should_race++;

	return 0;
}
