/**
 * @file atomic_compare_swap.c
 * @author Ben Blum
 * @brief PSU-only test for libatomic
 */

#include <stdio.h>
#include <thread.h>
#include <syscall.h>
#include <atomic.h>

#define START_VALUE 3
#define SWAP_VALUE_1 7
#define SWAP_VALUE_2 9
#define BOGUS_VALUE 0x15410de0u

// in this test, two outcomes are possible:
// if parent succeeds its cmpxchg and child fails:
// -> magic_global_value              == SWAP_VALUE_1
// -> magic_thread_local_value_parent == START_VALUE
// -> magic_thread_local_value_child  == SWAP_VALUE_1
// or, if child succeeds its cmpxchg and parent fails:
// -> magic_global_value              == SWAP_VALUE_2
// -> magic_thread_local_value_parent == SWAP_VALUE_2
// -> magic_thread_local_value_child  == START_VALUE
// in both cases, the invariant is that (parent + child - global) has to equal
// the original START_VALUE after both threads are finished.

// magic variable names
volatile int magic_global_value              = START_VALUE;
// don't check this: which thread succeeds is random, so both outcomes are possible
// volatile int magic_global_expected_result = START_VALUE + SWAP_VALUE_1;
// volatile int magic_global_expected_result = START_VALUE + SWAP_VALUE_2;
volatile int magic_thread_local_value_parent = BOGUS_VALUE;
volatile int magic_thread_local_value_child  = BOGUS_VALUE;
// likewise, which thread will see the other's update, both outcomes are possible
// volatile int magic_expected_sum           = START_VALUE + SWAP_VALUE_2;
// volatile int magic_expected_sum           = START_VALUE + SWAP_VALUE_1;

// INSTEAD... landslide will check the sum of the two threads' local values
// *minus* the final global_value, as described above, against this value.
volatile int magic_expected_sum_minus_result = START_VALUE;

void *child(void *p)
{
	magic_thread_local_value_child =
		atomic_compare_swap(&magic_global_value, START_VALUE, SWAP_VALUE_2);

	vanish(); /* bypass thr_exit */

	while(1) continue; /* placate compiler portably */
}

int main()
{
	thr_init(PAGE_SIZE);
	misbehave(BGND_BRWN >> FGND_CYAN); // for landslide
	(void) thr_create(child, (void *)0);

	magic_thread_local_value_parent =
		atomic_compare_swap(&magic_global_value, START_VALUE, SWAP_VALUE_1);

	vanish(); /* bypass thr_exit */

	while(1) continue; /* placate compiler portably */
}
