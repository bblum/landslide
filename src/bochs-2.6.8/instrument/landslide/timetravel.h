/**
 * @file timetravel.h
 * @brief checkpoint and restore interface
 * @author Ben Blum
 */

#ifndef __LS_TIMETRAVEL_H
#define __LS_TIMETRAVEL_H

#ifdef BOCHS

struct timetravel_state {
	// TODO: nothing?
};

struct timetravel_hax {
	// TODO: implememp
};

#define timetravel_init(t) do { } while (0)

#else /* SIMICS */

struct timetravel_state { const char *cmd_file; };
struct timetravel_hax { };
#define timetravel_init(t) do { (t)->cmd_file = NULL; } while (0)

#endif

void timetravel_set(struct timetravel_state *ts, struct timetravel_hax *tm);
void timetravel_jump(struct timetravel_state *ts, struct timetravel_hax *tm);
void timetravel_delete(struct timetravel_state *ts, struct timetravel_hax *tm);

#endif /* __LS_TIMETRAVEL_H */
