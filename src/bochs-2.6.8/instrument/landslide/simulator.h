/**
 * @file simulator.h
 * @brief gateway header to various simulator-specific apis
 * @author Ben Blum
 */

#ifndef __LS_SIMULATOR_H
#define __LS_SIMULATOR_H

#ifdef BOCHS
/*********************************** bochs  ***********************************/

#include <stdlib.h>

#include "bochs.h"

typedef BX_CPU_C cpu_t;
typedef void symtable_t; // XXX
typedef void keyboard_t; // TODO
typedef void apic_t; // TODO
typedef void pic_t; // TODO

#define __LS_MALLOC(x,t) ((t *)malloc((x) * sizeof(t)))
#define __LS_STRDUP(s)   strdup(s)

#define MM_FREE(p) free(p)

/******************************************************************************/
#else
/*********************************** simics ***********************************/

#include <simics/api.h>

typedef conf_object_t cpu_t;
typedef conf_object_t symtable_t;
typedef conf_object_t keyboard_t;
typedef conf_object_t apic_t;
typedef conf_object_t pic_t;

#define __LS_MALLOC(x,t) MM_MALLOC(x,t)
#define __LS_STRDUP(s)   MM_STRDUP(s)

/******************************************************************************/
#endif

#define MM_XMALLOC(x,t) ({					\
	typeof(t) *__xmalloc_ptr = __LS_MALLOC(x,t);		\
	assert(__xmalloc_ptr != NULL && "malloc failed");	\
	__xmalloc_ptr; })

#define MM_XSTRDUP(s) ({					\
	char *__xstrdup_ptr = __LS_STRDUP(s);			\
	assert(__xstrdup_ptr != NULL && "strdup failed");	\
	__xstrdup_ptr; })

#endif /* __LS_SIMULATOR_H */
