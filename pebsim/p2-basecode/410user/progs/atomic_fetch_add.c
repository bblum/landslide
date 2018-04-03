/**
 * @file atomic_fetch_add.c
 * @author Ben Blum
 * @brief PSU-only test for libatomic
 */

#include <stdio.h>
#include <thread.h>
#include <syscall.h>
#include <atomic.h>

#define START_VALUE 3
#define INCR_VALUE 7
#define BOGUS_VALUE 0x15410de0u

// magic variable names
// after all threads exit, landslide will check:
// - global_value == global_expected_result
// - thread_local_value_parent + thread_local_value_child == expected_sum
// this allows us to avoid relying on cond_wait to synchronize a final value
// between the two threads, so you can run this before implementing cond_wait
volatile int magic_global_value              = START_VALUE;
volatile int magic_global_expected_result    = START_VALUE + (2 * INCR_VALUE);
volatile int magic_thread_local_value_parent = BOGUS_VALUE;
volatile int magic_thread_local_value_child  = BOGUS_VALUE;
volatile int magic_expected_sum              = (2 * START_VALUE) + INCR_VALUE;

void *child(void *p)
{
	magic_thread_local_value_child =
		atomic_fetch_add(&magic_global_value, INCR_VALUE);

	vanish(); /* bypass thr_exit */

	while(1) continue; /* placate compiler portably */
}

int main()
{
	thr_init(PAGE_SIZE);
	misbehave(BGND_BRWN >> FGND_CYAN); // for landslide
	(void) thr_create(child, (void *)0);

	magic_thread_local_value_parent =
		atomic_fetch_add(&magic_global_value, INCR_VALUE);

	vanish(); /* bypass thr_exit */

	while(1) continue; /* placate compiler portably */
}
