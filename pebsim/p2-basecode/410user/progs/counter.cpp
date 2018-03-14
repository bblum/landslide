extern "C" {
#include <stdio.h>
#include <assert.h>
//#include <pthread.h>
#include <thread.h>
// #include <htm.hpp>
#include <htm.h>
}

#include <new.hpp> // for allocation

// #define ITERS (1 << 25)
#define ITERS 2
#define NTHREADS 2

/* This file increments a counter under high contention with
 * many different threads. First it does so locklessly and then with
 * Intel TSX.
 */

typedef unsigned int u32;
typedef unsigned long int u64;

struct ArgPack {
    u32 thread_id;
    u64 iters;
    u64* counter;
};

void*
locklessIncr(void* argpack) {
    ArgPack* args = (ArgPack*) argpack;
    u64 iters = args->iters;
    u64* counter = args->counter;

    for (u64 i = 0; i < iters; i++) {
        __sync_fetch_and_add(counter, 1);
    }
    return NULL;
}

void*
tsxIncr(void* argpack) {
    ArgPack* args = (ArgPack*) argpack;
    u64 iters = args->iters;
    u64* counter = args->counter;

    assert(iters != 0);
    for (u64 i = 0; i < iters; i++) {
        unsigned int status = _xbegin();
        if (status == _XBEGIN_STARTED) {
            (*counter)++;
            _xend();
        } else {
            __sync_fetch_and_add(counter, 1);
        }
    }
    return NULL;
}

void
run_incr_job(void *(* job)(void *), u32 num_threads, u64 total_iters) {
    ArgPack args[num_threads];
    int threads[num_threads];
    u64* counter = new u64(0);

    u64 iters = total_iters ;/// num_threads;
    for (u32 i = 0; i < num_threads; ++i) {
        args[i].thread_id = i;
        args[i].iters = iters;
        args[i].counter = counter;
    }

    // double start = CycleTimer::currentSeconds();

    for (u32 i = 0; i < num_threads; ++i) {
        // pthread_create(&threads[i], NULL, job, (void *)&args[i]);
        threads[i] = thr_create(job, (void *)&args[i]);
        assert(threads[i] > 0);
    }

    for (u32 i = 0; i < num_threads; ++i) {
        //pthread_join(threads[i], NULL);
        thr_join(threads[i], (void **)NULL);
    }
    assert((*counter) == iters * num_threads);

    // double end = CycleTimer::currentSeconds();

    if (*counter != total_iters) {
        printf("ERROR! *counter == %ld, total_iters == %ld\n",
                *counter, total_iters);
    }
    delete counter;
    return; // end - start;
}

int
main() {

    thr_init(4096);
    // double time;
    int time = 0;

    // for (unsigned int num_threads = 1; num_threads <= 4; num_threads *= 2) {
    { unsigned int num_threads = NTHREADS;

        // run_incr_job(locklessIncr, num_threads, ITERS);
        // printf("Ran lockless job with %d threads in %d\n", num_threads, time);

        run_incr_job(tsxIncr, num_threads, ITERS);
        printf("Ran tsx job with %d threads in %d\n", num_threads, time);

    }
    return 0;

}

// vim: ts=4
// vim: expandtab
