/**
 * @file timetravel.h
 * @brief checkpoint and restore interface
 * @author Ben Blum
 */

#ifndef __LS_TIMETRAVEL_H
#define __LS_TIMETRAVEL_H

#include <stdbool.h>

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

#else /* SIMICS */

struct timetravel_state { char *cmd_file; };
struct timetravel_hax { };

#define timetravel_init(ts)     do { (ts)->cmd_file = NULL; } while (0)
#define timetravel_hax_init(th) do { } while (0)

#endif

struct hax;

void timetravel_set(struct timetravel_state *ts, struct hax *h);
void timetravel_jump(struct timetravel_state *ts, struct timetravel_hax *tt,
		     unsigned int tid, bool txn, unsigned int xabort_code);
void timetravel_delete(struct timetravel_state *ts, struct timetravel_hax *tt);

#endif /* __LS_TIMETRAVEL_H */
