#include <thread.h>
#include <syscall.h>
#include <assert.h>
#include "htm.h"

#define NTHREADS 9
#define NITERS 9
//#define ASYMMETRIC_ITERS

typedef struct htm_mutex { mutex_t m; volatile int held; } htm_mutex_t;

void htm_mutex_init(htm_mutex_t *hm)
{
	mutex_init(&hm->m);
	hm->held = 0;
}

// must be tested with landslide -X -A -S
void htm_mutex_lock(htm_mutex_t *hm)
{
	int status;
	const int conflict_code = 0xff;
	while (1) {
		if ((status = _xbegin()) == _XBEGIN_STARTED) {
			if (hm->held == 1) {
				_xabort(conflict_code);
				assert(0 && "xabort didn't");
			}
			break;
		} else if ((status & _XABORT_RETRY) == 0) {
			if ((status & _XABORT_EXPLICIT) != 0) {
				assert(_XABORT_CODE(status) == conflict_code &&
				       "this test requires landslide -A for"
				       "xabort codes... or the user aborted?");
			}
			mutex_lock(&hm->m);
			// don't assert before the write; oopses the state space
			// assert(hm->held == 0);
			// barrier necessary even on x86; see tsx-barrier.c
			int washeld = __sync_lock_test_and_set(&hm->held, 1);
			assert(washeld == 0);
			break;
		} else {
			assert(0 && "this test requires landslide -S"
			       "to suppress _XABORT_RETRY");
		}
	}
}

void htm_mutex_unlock(htm_mutex_t *hm)
{
	if (_xtest()) {
		assert(hm->held == 0);
		_xend();
	} else {
		// i also tested without this assertion to make sure it doesn't
		// oops the state space
		assert(hm->held == 1);
		// this is just a normal write on x86 but just for style
		__sync_lock_release(&hm->held);
		mutex_unlock(&hm->m);
	}
}

/**** test case below *********************************************************/

static htm_mutex_t htm_lock;

static void *thread(void *arg)
{
	swexn(0,0,0,0);
	for (int i = 0; i < NITERS; i++)
	{
		htm_mutex_lock(&htm_lock);
		if (!_xtest()) {
			assert(0 && "this test requires landslide -X -A -S"
			       "to suppress XABORT_RETRY"
			       "...either that, or your htm-lock sucks!!");
		}
		htm_mutex_unlock(&htm_lock);
	}
	return NULL;
}

int main()
{
	// see similar htm_spinlock.c for why not to do this
	// misbehave(BGND_BRWN >> FGND_CYAN);
	thr_init(8192);
	swexn(0,0,0,0);
	int threads[NTHREADS];

	htm_mutex_init(&htm_lock);
	for (int i = 0; i < NTHREADS; i++) {
		threads[i] = thr_create(thread, (void *)i);
		assert(threads[i] >= 0 && "failed thr create");
	}
	for (int i = 0; i < NTHREADS; i++) {
		thr_join(threads[i], NULL);
	}
	return 0;
}
