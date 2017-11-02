#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"

static void child_thread_func(void *lock_)
{
    struct lock *lock = lock_;

    lock_acquire (lock);
    lock_release (lock);
}

void test_priority_donate_multiple(void)
{
    struct lock lock;

    ASSERT(!thread_mlfqs);
    ASSERT(thread_get_priority() == PRI_DEFAULT);

    lock_init(&lock);

    lock_acquire(&lock);

    thread_create("t1", PRI_DEFAULT + 1, child_thread_func, &lock);
    ASSERT (PRI_DEFAULT + 1 == thread_get_priority());

    /*
     * Don't preempt between thread_create() and lock_release()
     */
    thread_create ("t2", PRI_DEFAULT + 1, child_thread_func, &lock);
    ASSERT (PRI_DEFAULT + 1 == thread_get_priority());

    /*
     * Preempt inside lock_release() between clear_donor_list() and
     * sema_up() in favor of `t2`. `t2` will see that the lock is
     * acquired and donate its priority to this thread, but we've
     * already cleared our donated priorities. At this point, we'll
     * continue with priority PRI_DEFAULT + 1 instead of PRI_DEFAULT.
     */
    lock_release(&lock);
    ASSERT (PRI_DEFAULT + 0 == thread_get_priority());
}
