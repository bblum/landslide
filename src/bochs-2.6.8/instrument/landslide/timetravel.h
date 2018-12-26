/**
 * @file timetravel.h
 * @brief checkpoint and restore interface
 * @author Ben Blum
 */

#ifndef __LS_TIMETRAVEL_H
#define __LS_TIMETRAVEL_H

#include <stdbool.h>

#include "student_specifics.h" /* for ABORT_SETS */

struct nobe;
struct abort_set;

#ifdef BOCHS

struct timetravel_state {
	int pipefd; /* used to communicate exit status */
};

struct timetravel_nobe {
	bool active;
	bool parent;
	int pipefd;
};

void timetravel_init(struct timetravel_state *ts);
#define timetravel_nobe_init(th) do { (th)->active = false; } while (0)

/* Time travel is implemented by fork()ing the simulation at each PP.
 * Accordingly, any landslide state which should "glow green" must be updated
 * very carefully -- i.e., the forked processes must see all changes by DPOR/
 * estimation/etc. To accomplish this, all nobes are protected by const, and
 * in order to change them, you need to go through this function. */
#include <type_traits>
void __modify_nobes(void (*cb)(struct nobe *h_rw, void *), const struct nobe *h_ro,
		    void *arg, unsigned int arg_size);
template <typename T> inline void modify_nobe(void (*cb)(struct nobe *h_rw, T *),
					     const struct nobe *h_ro, T arg)
{
	assert(h_ro != NULL);
#ifndef HTM_ABORT_SETS
	/* prevent sending pointers which might be invalid in another process;
	 * abort sets unfortunately needs to send a struct of 4 ints through
	 * (see explore.c), which is legal, but is-fundamental can't check */
	STATIC_ASSERT(std::is_fundamental<T>::value && "no pointers allowed!");
#endif
	__modify_nobes((void (*)(struct nobe *, void *))cb, h_ro, &arg, sizeof(arg));
	/* also update the version in our local memory, of course */
	cb((struct nobe *)h_ro, &arg);
}

#else /* SIMICS */

struct timetravel_state { char *cmd_file; };
struct timetravel_nobe { };

#define timetravel_init(ts)     do { (ts)->cmd_file = NULL; } while (0)
#define timetravel_nobe_init(th) do { } while (0)
#define modify_nobe(cb, h_ro, arg) ((cb)((struct nobe *)(h_ro), (arg)))

#endif

bool timetravel_set(struct ls_state *ls, struct nobe *h,
		    unsigned int *tid, bool *txn, unsigned int *xabort_code,
		    struct abort_set *aborts);
void timetravel_jump(struct ls_state *ls, const struct timetravel_nobe *tt,
		     unsigned int tid, bool txn, unsigned int xabort_code,
		     struct abort_set *aborts);
void timetravel_delete(struct ls_state *ls, const struct timetravel_nobe *tt);

#endif /* __LS_TIMETRAVEL_H */
