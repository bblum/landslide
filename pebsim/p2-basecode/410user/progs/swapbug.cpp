extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <thread.h>
// #include <atomic>
// #include <tools/cycle_timer.h>
#include <htm.h>
#include <rand.h>
}

#include <new.hpp>

#define NTHREADS 3
#define ITERS 2
// #define ITERS (1 << 25)

#define CAPACITY NTHREADS
#define MAX_CAPACITY (CAPACITY * 2) // must be this so it doesn't iterate
// #define CAPACITY (1 << 10)
// #define MAX_CAPACITY (1 << 16)

#define TSX_INLINING true
#define TSX_SUCCESS _XBEGIN_STARTED // 0xFFFFFFFF

typedef unsigned int u32;
typedef unsigned long int u64;

struct ArgPack { 
    u32  thread_id;
    u64  iters;
    u64* data;
    u64  cap;
    u32  seed;
    mutex_t* locks;
    bool* is_locked;
};

#if 0
void*
lockSwap(void* argpack) {
    ArgPack* args = (ArgPack*) argpack;
    u64 iters = args->iters;
    u64* data = args->data;
    u64  cap  = args->cap;
    u32  seed = args->seed;
    mutex_t* locks = args->locks;
    

    for (u64 i = 0; i < iters; i++) {
        u64 rand_idx1 = rand_r(&seed) % cap;
        u64 rand_idx2 = rand_r(&seed) % cap;
        // Ensure that rand_idx1 < rand_idx2
        if (rand_idx1 == rand_idx2) continue;
        if (rand_idx1 >  rand_idx2) {
            u64 tmp = rand_idx1;
            rand_idx1 = rand_idx2;
            rand_idx2 = tmp;
        }

        // LOCK ////////////////////////////////
        mutex_lock(&locks[rand_idx1]);
        mutex_lock(&locks[rand_idx2]);

        // SWAP ////////////////////////////////
        u64 tmp = data[rand_idx1];
        data[rand_idx1] = data[rand_idx2];
        data[rand_idx2] = tmp;

        // UNLOCK //////////////////////////////
        mutex_unlock(&locks[rand_idx1]);
        mutex_unlock(&locks[rand_idx2]);

    }

    return NULL;
}
#endif

void* 
tsxSwap(void* argpack) {
    ArgPack* args = (ArgPack*) argpack;
    u64 iters = args->iters;
    u64* data = args->data;
    //u64  cap  = args->cap;
    //u32  seed = args->seed;
    mutex_t* locks = args->locks;
    bool* is_locked = args->is_locked;

    for (u64 i = 0; i < iters; i++) {
        u64 rand_idx1 = args->thread_id; // rand_r(&seed) % cap;
        u64 rand_idx2 = (args->thread_id + 1 )% NTHREADS; // rand_r(&seed) % cap;
        // "oops"
        // // Ensure that rand_idx1 < rand_idx2
        // if (rand_idx1 == rand_idx2) continue;
        // if (rand_idx1 >  rand_idx2) {
        //     u64 tmp = rand_idx1;
        //     rand_idx1 = rand_idx2;
        //     rand_idx2 = tmp;
        // }

#ifdef TSX_INLINING
        // XXX: No retries!
        if ((unsigned)_xbegin() == TSX_SUCCESS) {
            if (is_locked[rand_idx1] || is_locked[rand_idx2]) {
                _xabort(0);
            }
            u64 tmp         = data[rand_idx1]; 
            data[rand_idx1] = data[rand_idx2];
            data[rand_idx2] = tmp;
            _xend();
#else
        Result status = _xbegin();
        if (status == SUCCESS) {
            if (is_locked[rand_idx1] || is_locked[rand_idx2]) {
                _xabort();
            }
            u64 tmp         = data[rand_idx1]; 
            data[rand_idx1] = data[rand_idx2];
            data[rand_idx2] = tmp;
            _xend();
#endif
        } else { 

            // LOCK ////////////////////////////////
            mutex_lock(&locks[rand_idx1]);
            is_locked[rand_idx1] = true;
            mutex_lock(&locks[rand_idx2]);
            is_locked[rand_idx2] = true;

            // SWAP ////////////////////////////////
            u64 tmp = data[rand_idx1];
            data[rand_idx1] = data[rand_idx2];
            data[rand_idx2] = tmp;

            // UNLOCK //////////////////////////////
            mutex_unlock(&locks[rand_idx1]);
            is_locked[rand_idx1] = false;
            mutex_unlock(&locks[rand_idx2]);
            is_locked[rand_idx2] = false;

        }
    }

    return NULL;
}

void
run_swap_job(void *(* job)(void *), u32 num_threads, u64 cap, u64 total_iters) {
    ArgPack args[num_threads];
    unsigned int threads[num_threads];

    u64* data = new u64[cap];
    mutex_t* locks = new mutex_t[cap];
    bool* is_locked = new bool[cap];
    for (u64 idx = 0; idx < cap; idx++) {
        data[idx] = idx;
        is_locked[idx] = false;
        mutex_init(&locks[idx]);
    }

    u64 iters = total_iters ; /// num_threads;
    for (u32 i = 0; i < num_threads; ++i) {
        args[i].thread_id = i;
        args[i].seed      = i;
        args[i].iters     = iters;
        args[i].data      = data;
        args[i].cap       = cap;
        args[i].locks     = locks;
        args[i].is_locked = is_locked;
    }

    // double start = CycleTimer::currentSeconds();

    for (u32 i = 0; i < num_threads; ++i) {
        threads[i] = thr_create(job, (void *)&args[i]);
    }

    for (u32 i = 0; i < num_threads; ++i) {
        thr_join(threads[i], (void **)NULL);
    }

    // double end = CycleTimer::currentSeconds();

    // TODO: Validate for correctness.
    delete[] data;
    delete[] locks;

    return ; //end - start;
}

int
main() {

    // double time; 

    	thr_init(4096);
    for (unsigned int capacity = CAPACITY; capacity < MAX_CAPACITY; capacity*=2) {
        //for (unsigned int num_threads = 1; num_threads <= 4; num_threads *= 2) {

            run_swap_job(tsxSwap, NTHREADS, capacity, ITERS);

        //}
    }
    return 0;

}
