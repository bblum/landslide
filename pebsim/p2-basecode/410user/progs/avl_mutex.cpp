extern "C" {
#include <assert.h>
#include <thread.h>
#include <stdlib.h>
#include <syscall.h>
// #include <tools/cycle_timer.h>

// #include <iostream>

// #include <htm.hpp>
}
#include <htm/htmavl_mutex.hpp>
// #include <htmtree.hpp> // provides fgt/treap, locking, not needed

/* one curiosity about this test.... each child thread is just doing literally
 * one mutex protected critical section per iteration, so you'd think that maybe
 * each (K,1) test would just have exactly K! interleavings. i looked into this,
 * and it turns out there are a bunch of dpor memory conflicts on free/re-malloc
 * arising between the free() of main thread's join(), and insert()'s 'new'.
 * i tried fixing this by having the refp2 preallocate 16 tcbs, which made the
 * state space a little smaller, but also mysteriously made others bigger (idk
 * why; i put the ignore/thrlib functions in correctly). also, there were other
 * malloc conflicts because insert() allocates the nobe before taking the lock,
 * which causes more than just the critical section transitions to conflict,
 * so i'm just leaving it is, and saying it's not landslide's problem. */
#define ITERS 1
#define NUM_THREADS 4

int tsx_inserts = ITERS;
// int fg_inserts = 40000000;

struct ArgPack {
    unsigned int seed;
    void *tree;
};

u64 rand_r(unsigned int *seed) {
	return (u64)(*seed = (*seed + 0x38262fc3) * 0x038fc875);
}

void * tsxWork(void *arg) {
    ArgPack *args = (ArgPack *)arg;
    AVL<int> *tree = (AVL<int> *) args->tree;

    // int last_print = 40000000;
    // unsigned int tid = args->seed;
    unsigned int seed = args->seed;
    //while (1) {
    //   int work_left = __sync_fetch_and_add(&tsx_inserts, -1);
    //   if (work_left <= 0) {
    //       break;
    //   }
    for (int i = 0; i < tsx_inserts; i++) {

       int val = rand_r(&seed);
       tree->insert(val, val);

       // if (tid == 0 && (last_print - tsx_inserts > 1000000)) {
       //     printf("Work Done %d\n", tsx_inserts);
       //     printf("Diags %s\n", showDiags().c_str());
       //     last_print = tsx_inserts;
       // }
    }

    return NULL;
}

#if 0
void * fgWork(void *arg) {
    ArgPack *args = (ArgPack *)arg;
    FineGrainBST *tree =(FineGrainBST *) args->tree;

    unsigned int seed = args->seed;
    while (1) {
        int work_left = __sync_fetch_and_add(&fg_inserts, -1);
        if (work_left <= 0) {
            break;
        }

       int val = rand_r(&seed);
       tree->insert(val);
    }

    return NULL;
}
#endif

void run_job(void *tree, void *(* job)(void *)) {
    ArgPack args[NUM_THREADS];
    int threads[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; ++i) {
        args[i].seed = i;
        args[i].tree = tree;
    }

    //double start = CycleTimer::currentSeconds();

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads[i] = thr_create(job, (void *)&args[i]);
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        thr_join(threads[i], (void **)NULL);
    }

    //double end = CycleTimer::currentSeconds();
    return ; //end - start;
}

// make gcc link with these functions so landslide thinks they at least exist
void unused() { _xbegin(); _xabort(0); _xend(); }

int main() {
    // Run transactional tree benchmark
    thr_init(4096);
    AVL<int> tsxAVL;
    run_job((void *)&tsxAVL, tsxWork);

    // printf("RunTime: %lf\n", tsxTime * 1000.f);
    // printf("Validating...");
    __attribute__((unused))
    TreeInfo info = tsxAVL.validate();

    // printf("Passed!\n");

    // printf("Tree Size %d Tree Depth %d \n", info.size, info.depth);
    // printf("Diagnostics: %s \n", showDiags().c_str());

    // Run fine grain locking benchmark
   //  FineGrainBST fgBst;
   //  double fgTime = run_job((void *)&fgBst, fgWork);
   //  printf("Run Time: %lf\n", fgTime * 1000.f);

   //  printf("Validating...");
   //  TreeInfo fgInfo = fgBst.validate();
   //  printf("Passed!\n");
   //  printf("Tree Size %d Tree Depth %d \n", fgInfo.size, fgInfo.depth);

    return 0;
}
