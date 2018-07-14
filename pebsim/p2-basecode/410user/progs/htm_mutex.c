#include <thread.h>
#include <syscall.h>
#include <stdbool.h>
#include <assert.h>
#include "htm.h"

#define NTHREADS 2
#define NITERS 1
//#define ASYMMETRIC_ITERS

typedef struct htm_mutex { mutex_t m; volatile bool held; } htm_mutex_t;

void htm_mutex_init(htm_mutex_t *hm)
{
	mutex_init(&hm->m);
	hm->held = false;
}

// must be tested with landslide -X -A -S
void htm_mutex_lock(htm_mutex_t *hm)
{
	int status;
	const int conflict_code = 0xff;
	while (1) {
		if ((status = _xbegin()) == _XBEGIN_STARTED) {
			if (hm->held) {
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
			assert(!hm->held);
			hm->held = true;
			__sync_synchronize();
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
		assert(!hm->held);
		_xend();
	} else {
		assert(hm->held);
		hm->held = false;
		mutex_unlock(&hm->m);
	}
}

/**** test case below *********************************************************/

static htm_mutex_t htm_lock;
static int num_in_section;

/* this function's accesses to num-in-section are ignored w/ ignore_dr_function
 * (see id/job.c) so they won't create DR PPs (we yield() ourselves instead),
 * but NOT using thrlib_function, which would oops make DPOR ignore these,
 * which would not even cause the transactions to be seen to conflict at all */
static void
critical_section()
{
	num_in_section++;
	if (!_xtest()) {
		thr_yield(-1);
	}
	assert(num_in_section == 1 && "o noes, htmutex didn't mutually exclude");
	num_in_section--;
}

static void *thread(void *arg)
{
	swexn(0,0,0,0);
#ifdef ASYMMETRIC_ITERS
	int id = (int)arg;
	for (int i = 0; i < id+1; i++)
#else
	for (int i = 0; i < NITERS; i++)
#endif
	{
		htm_mutex_lock(&htm_lock);
		critical_section();
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
