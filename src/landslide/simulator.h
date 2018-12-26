/**
 * @file simulator.h
 * @brief gateway header to various simulator-specific apis
 * @author Ben Blum
 *
 * Copyright (c) 2018, Ben Blum
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
typedef void symtable_t;

typedef struct { } simulator_state_t;
typedef void simulator_object_t;

#define MM_MALLOC(x,t) ((t *)malloc((x) * sizeof(t)))
#define MM_STRDUP(s)   strdup(s)
#define MM_FREE(p)     free(p)

void quit_landslide(unsigned int code); /* defined in timetravel.c */
#define BREAK_SIMULATION() do { } while (0)
#define QUIT_SIMULATION(v) quit_landslide(v)

#define SIM_NAME "bochs"

#else
#include "simulator-simics.h"
#define SIM_NAME "simics"
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
