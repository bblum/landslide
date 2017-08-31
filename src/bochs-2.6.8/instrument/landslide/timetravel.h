/**
 * @file timetravel.h
 * @brief checkpoint and restore interface
 * @author Ben Blum
 */

#ifndef __LS_TIMETRAVEL_H
#define __LS_TIMETRAVEL_H

#include <stdbool.h>

struct hax;

#ifdef BOCHS

struct timetravel_state {
	int pipefd; /* used to communicate exit status */
};

struct timetravel_hax {
	bool active;
	bool parent;
	int pipefd;
};

void timetravel_init(struct timetravel_state *ts);
#define timetravel_hax_init(th) do { (th)->active = false; } while (0)

/* Time travel is implemented by fork()ing the simulation at each PP.
 * Accordingly, any landslide state which should "glow green" must be updated
 * very carefully -- i.e., the forked processes must see all changes by DPOR/
 * estimation/etc. To accomplish this, all haxes are protected by const, and
 * in order to change them, you need to go through this function. */
#include <type_traits>
void __modify_haxes(void (*cb)(struct hax *h_rw, void *), const struct hax *h_ro,
		    void *arg, unsigned int arg_size);
template <typename T> inline void modify_hax(void (*cb)(struct hax *h_rw, T *),
					     const struct hax *h_ro, T arg)
{
	assert(h_ro != NULL);
	/* prevent sending pointers which might be invalid in another process */
	STATIC_ASSERT(std::is_fundamental<T>::value && "no pointers allowed!");
	__modify_haxes((void (*)(struct hax *, void *))cb, h_ro, &arg, sizeof(arg));
	/* also update the version in our local memory, of course */
	cb((struct hax *)h_ro, &arg);
}

#else /* SIMICS */

struct timetravel_state { char *cmd_file; };
struct timetravel_hax { };

#define timetravel_init(ts)     do { (ts)->cmd_file = NULL; } while (0)
#define timetravel_hax_init(th) do { } while (0)
#define modify_hax(cb, h_ro, arg) ((cb)((struct hax *)(h_ro), (arg)))

#endif

bool timetravel_set(struct ls_state *ls, struct hax *h,
		    unsigned int *tid, bool *txn, unsigned int *xabort_code);
void timetravel_jump(struct ls_state *ls, const struct timetravel_hax *tt,
		     unsigned int tid, bool txn, unsigned int xabort_code);
void timetravel_delete(struct ls_state *ls, const struct timetravel_hax *tt);

#endif /* __LS_TIMETRAVEL_H */
