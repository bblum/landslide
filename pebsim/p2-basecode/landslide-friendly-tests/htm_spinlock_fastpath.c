// landslide/410 environment
#include "htm.h"
#include <thread.h>
#include <syscall.h>
#define pthread_t int
#define pthread_create(tidp, attr, func, arg) ({ *(tidp) = thr_create((func),(arg)); tidp < 0 ? -1 : 0; })
#define pthread_join(tid, statusp) thr_join(tid, statusp)
#define NDEBUG
#define fprintf(...)

// verification test parameters
#define NTHREADS 2
#define NITERS 1
// end landslide/410 environment

// the following code is from https://gist.github.com/thoughtpolice/7123036
// several other htm spinlocks (clearly derived from this implementation)
// can also be found on github with some cursory searches

/*
 * spinlock-rtm.c: A spinlock implementation with dynamic lock elision.
 * Copyright (c) 2013 Austin Seipp
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* -- spinlock-rtm.c contains code from tsx-tools, with this license: ------- */
/* -- https://github.com/andikleen/tsx-tools -------------------------------- */

/*
 * Copyright (c) 2012,2013 Intel Corporation
 * Author: Andi Kleen
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>

/* -------------------------------------------------------------------------- */
/* -- Utilities ------------------------------------------------------------- */

/* Clang compatibility for GCC */
#define LOCKABLE
#define SCOPED_LOCKABLE
#define GUARDED_BY(x)
#define GUARDED_VAR
#define PT_GUARDED_BY(x)
#define PT_GUARDED_VAR
#define ACQUIRED_AFTER(...)
#define ACQUIRED_BEFORE(...)
#define EXCLUSIVE_LOCK_FUNCTION(...)
#define SHARED_LOCK_FUNCTION(...)
#define ASSERT_EXCLUSIVE_LOCK(...)
#define ASSERT_SHARED_LOCK(...)
#define EXCLUSIVE_TRYLOCK_FUNCTION(...)
#define SHARED_TRYLOCK_FUNCTION(...)
#define UNLOCK_FUNCTION(...)
#define LOCKS_RETURNED(x)
#define LOCKS_EXCLUDED(...)
#define EXCLUSIVE_LOCKS_REQUIRED(...)
#define SHARED_LOCKS_REQUIRED(...)
#define NO_THREAD_SAFETY_ANALYSIS

/* -------------------------------------------------------------------------- */
/* -- HLE/RTM compatibility code for older binutils/gcc etc ----------------- */

#define _mm_pause()                             \
  ({                                            \
    __asm__ __volatile__("pause" ::: "memory"); \
  })                                            \

/* -------------------------------------------------------------------------- */
/* -- A simple spinlock implementation with lock elision -------------------- */

// XXX: this should probably be volatile
typedef struct LOCKABLE spinlock { int v; } spinlock_t;

void
dyn_spinlock_init(spinlock_t* lock)
{
  lock->v = 0;
}

void
hle_spinlock_acquire(spinlock_t* lock) EXCLUSIVE_LOCK_FUNCTION(lock)
{
  while (__sync_lock_test_and_set(&lock->v, 1) != 0)
  {
    int val;
    do {
      _mm_pause();
      val = __sync_val_compare_and_swap(&lock->v, 1, 1);
    } while (val == 1);
  }
}

bool
hle_spinlock_isfree(spinlock_t* lock)
{
  // FIXME: unnecessary write can cause aborts even w/disjoint critical sections
  return (__sync_bool_compare_and_swap(&lock->v, 0, 0) ? true : false);
  //return lock->v == 0;
}

void
hle_spinlock_release(spinlock_t* lock) UNLOCK_FUNCTION(lock)
{
  __sync_lock_release(&lock->v);
}

void
rtm_spinlock_acquire(spinlock_t* lock) EXCLUSIVE_LOCK_FUNCTION(lock)
{
  unsigned int tm_status = 0;

 tm_try:
  if ((tm_status = _xbegin()) == _XBEGIN_STARTED) {
    /* If the lock is free, speculatively elide acquisition and continue. */
    if (hle_spinlock_isfree(lock)) return;

    /* Otherwise fall back to the spinlock by aborting. */
    _xabort(0xff); /* 0xff canonically denotes 'lock is taken'. */
  } else {
    /* _xbegin could have had a conflict, been aborted, etc */
    if (tm_status & _XABORT_RETRY) {
      //__sync_add_and_fetch(&g_rtm_retries, 1);
      goto tm_try; /* Retry */
    }
    if (tm_status & _XABORT_EXPLICIT) {
      int code = _XABORT_CODE(tm_status);
      if (code == 0xff) goto tm_fail; /* Lock was taken; fallback */
    }

#ifndef NDEBUG
    fprintf(stderr, "TSX RTM: failure; (code %d)\n", tm_status);
#endif /* NDEBUG */
  tm_fail:
    //__sync_add_and_fetch(&g_locks_failed, 1);
    hle_spinlock_acquire(lock);
  }
}

void
rtm_spinlock_release(spinlock_t* lock) UNLOCK_FUNCTION(lock)
{
  /* If the lock is still free, we'll assume it was elided. This implies
     we're in a transaction. */
  if (hle_spinlock_isfree(lock)) {
    //g_locks_elided += 1;
    // if we're not in a transaction, landslide will found-a-bug on this,
    // so it serves as the assertion for its own invariant
    _xend(); /* Commit transaction */
  } else {
    /* Otherwise, the lock was taken by us, so release it too. */
    hle_spinlock_release(lock);
  }
}

/* -------------------------------------------------------------------------- */
/* -- Spinlock verification a la mutex_test --------------------------------- */

static spinlock_t g_lock;
//static int num_in_section GUARDED_BY(g_lock);

static void*
thread(void* arg)
{
  swexn(0,0,0,0);
  for (int i = 0; i < NITERS; i++)
  {
    rtm_spinlock_acquire(&g_lock);
    if (!_xtest()) {
    	    assert(0 && "this test requires landslide -X -A -S"
    	    	   "to suppress XABORT_RETRY"
    	    	   "...either that, or your htm-lock sucks!!");
    }
    rtm_spinlock_release(&g_lock);
  }
  return NULL;
}

int
main(int ac, char **av)
{
  // landslide-misbehave + running thread0 in main thread
  // produces the same state space as
  // no misbehave + creating N children + main thread joins on children
  // (thanks to the thrlib_function directive)
  // but if you create and join N children,
  // the misbehave adds irrelevant stuff to the state space
  // (i.e., testing interleavings between the main thread and worker threads)
  // so i'm just leaving it the way i started with
  // (the thread0-in-main approach would be faster-per-execution but the
  // actual set of interleavings in the whole state space is the same)
  // misbehave(BGND_BRWN >> FGND_CYAN);
  thr_init(8192);
  swexn(0,0,0,0);
  pthread_t threads[NTHREADS];

  dyn_spinlock_init(&g_lock);
  for (int i = 0; i < NTHREADS; i++) {
    if (pthread_create(&threads[i], NULL, thread, (void*)i)) {
      assert(0 && "failed thr create");
    }
  }
  for (int i = 0; i < NTHREADS; i++) {
    pthread_join(threads[i], NULL);
  }
  // this actually has no effect on the state space size,
  // in the no-join setup described above (even in the child threads)
  // vanish(); // bypass thr_exit
  return 0;
}
