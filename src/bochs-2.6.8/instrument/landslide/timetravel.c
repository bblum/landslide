/**
 * @file timetravel.c
 * @brief bochs implementation of timetravel using fork
 * @author Ben Blum
 */

#define MODULE_NAME "TT"
#define MODULE_COLOUR COLOUR_DARK COLOUR_MAGENTA

#include "common.h"
#include "simulator.h"
#include "tree.h"

#ifdef BOCHS

void timetravel_set(struct timetravel_state *ts, struct timetravel_hax *th)
{
	lsprintf(CHOICE, "implement me! fork here\n");
}

void timetravel_jump(struct timetravel_state *ts, struct timetravel_hax *th)
{
	assert(0 && "unimplememped :(\n");
}

void timetravel_delete(struct timetravel_state *ts, struct timetravel_hax *th)
{
	lsprintf(CHOICE, "imeplement me! kill a process\n");
}

#else
#include "timetravel-simics.c"
#endif
