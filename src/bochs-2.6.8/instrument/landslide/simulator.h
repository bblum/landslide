/**
 * @file simulator.h
 * @brief gateway header to various simulator-specific apis
 * @author Ben Blum
 */

#ifndef __LS_SIMULATOR_H
#define __LS_SIMULATOR_H

#ifdef BOCHS

#include <stdlib.h>

#include "bochs.h"

typedef BX_CPU_C cpu_t;
typedef void symtable_t; // XXX
typedef void keyboard_t;
typedef void apic_t;
typedef void pic_t;

typedef struct { } simulator_state_t;
typedef void simulator_object_t;

#define MM_MALLOC(x,t) ((t *)malloc((x) * sizeof(t)))
#define MM_STRDUP(s)   strdup(s)
#define MM_FREE(p)     free(p)

#define BREAK_SIMULATION() BX_EXIT(0)
#define QUIT_SIMULATION(v) BX_EXIT(v)

#else
#include "simulator-simics.h"
#endif

#define MM_XMALLOC(x,t) ({					\
	typeof(t) *__xmalloc_ptr = MM_MALLOC(x,t);		\
	assert(__xmalloc_ptr != NULL && "malloc failed");	\
	__xmalloc_ptr; })

#define MM_XSTRDUP(s) ({					\
	char *__xstrdup_ptr = MM_STRDUP(s);			\
	assert(__xstrdup_ptr != NULL && "strdup failed");	\
	__xstrdup_ptr; })

#endif /* __LS_SIMULATOR_H */
