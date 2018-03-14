extern "C" {
#include <assert.h>
#include <thread.h>
#include <stdlib.h>
#include <syscall.h>
//#include <htm.hpp>
}
#include <htm/htmmap.hpp>


// This is roughly 8 million
#define ITERS 1 //(1 << 23)
#define NUM_THREADS 2
#define MAX_MULT 1 // 8

struct ArgPack {
    unsigned int seed;
    unsigned int iters;
    void *map;
};

u64 rand_r(unsigned int *seed) {
	return (u64)(*seed = (*seed + 0x38262fc3) * 0x038fc875);
}

void * tsxWork(void *arg) {
    ArgPack *args = (ArgPack *)arg;
    SepChainMap<int> *map = (SepChainMap<int> *) args->map;
    unsigned int iters = args->iters;

    unsigned int seed = args->seed;
    for (unsigned int i = 0; i < iters; ++i) {
       u64 val = rand_r(&seed);

       map->insert(val, val);
    }

    return NULL;
}

/*
void * fgWork(void *arg) {
    ArgPack *args = (ArgPack *)arg;
    Map<int>::OpenAddrMap *map =(Map<int>::OpenAddrMap *) args->map;

    unsigned int seed = args->seed;
    for (int i = 0; i < ITERS; ++i) {
       int val = rand_r(&seed);

       map->insert(val);
    }

    return NULL;
}
*/

void run_job(void *map, void *(* job)(void *), unsigned int num_threads) {
    ArgPack args[num_threads];
    int threads[num_threads];

    unsigned int iters = ITERS ;/// num_threads;
    for (unsigned int i = 0; i < num_threads; ++i) {
        args[i].seed  = i;
        args[i].map   = map;
        args[i].iters = iters;
    }
    //printf("Starting job with %d threads and %d iterations per thread.\n",
    //    num_threads, iters);

    for (unsigned int i = 0; i < num_threads; ++i) {
        threads[i] = thr_create(job, (void *)&args[i]);
    }

    for (unsigned int i = 0; i < num_threads; ++i) {
        thr_join(threads[i], (void **)NULL);
    }

    return ;//end - start;
}

int main() {

    thr_init(4096);
    misbehave(BGND_BRWN >> FGND_CYAN); // for landslide

    for (unsigned int mult = 1; mult <= MAX_MULT; mult *= 2) {
        // Run transactional map benchmark
        SepChainMap<int> map;
        // Ensure we're big enough.
        map.initialize(ITERS / 2);
        run_job((void *)&map, tsxWork, mult*NUM_THREADS);

        //printf("%d-threaded runtime: %lf\n", mult*NUM_THREADS,
        //        tsxTime * 1000.f);

        //printf("Validating...");
        MapInfo mapInfo = map.validate();
        // printf("Passed!\n");
        // printf("Map size = %d, Smallest Bucket = %d, "
        //         "Largest Bucket = %d, Average Bucket = %f \n",
        //         mapInfo.number_entries, mapInfo.smallest_bucket_size,
        //         mapInfo.largest_bucket_size, mapInfo.average_bucket_size);
        // printf("Diagnostics: %s \n", showDiags().c_str());
    }

    // Run fine grain locking benchmark
    /*
    OpenAddrMap oamap;
    double fgTime = run_job((void *)&oamap, fgWork);
    printf("Run Time: %lf\n", fgTime * 1000.f);

    printf("Validating...");
    // TreeInfo fgInfo = fgBst.validate();
    printf("Passed!\n");
    // printf("Tree Size %d Tree Depth %d \n", fgInfo.size, fgInfo.depth);
    */

    return 0;
}
