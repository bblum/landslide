/**
 * @file stack.h
 * @brief stack tracing
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

#ifndef __LS_STACK_H
#define __LS_STACK_H

#include "common.h"
#include "simulator.h"
#include "array_list.h"

struct ls_state;

/* stack trace data structures. */
struct stack_frame {
	unsigned int eip;
	unsigned int actual_eip; /* not corrected for noreturn funcs */
	char *name; /* may be null if symtable lookup failed */
	char *file; /* may be null, as above */
	int line;   /* valid iff above fields are not null */
};

struct stack_trace {
	unsigned int tid;
	ARRAY_LIST(const struct stack_frame) frames;
};

typedef ARRAY_LIST(struct stack_frame) mutable_stack_frames_t;
static inline mutable_stack_frames_t *mutable_stack_frames(struct stack_trace *st)
	{ return (mutable_stack_frames_t *)&st->frames; }

/* interface */

/* utilities / glue */
unsigned int sprint_frame(char *buf, unsigned int maxlen, const struct stack_frame *f, bool colours);
bool eip_to_frame(unsigned int eip, struct stack_frame *f);
void destroy_frame(struct stack_frame *f);
void print_stack_frame(verbosity v, const struct stack_frame *f);
void print_eip(verbosity v, unsigned int eip);
void print_stack_trace(verbosity v, const struct stack_trace *st);
unsigned int html_stack_trace(char *buf, unsigned int maxlen, const struct stack_trace *st);
struct stack_trace *copy_stack_trace(const struct stack_trace *src);
void free_stack_trace(struct stack_trace *st);

/* actual logic */
struct stack_trace *stack_trace(struct ls_state *ls);
bool within_function_st(const struct stack_trace *st, unsigned int func, unsigned int func_end);
bool within_function(struct ls_state *ls, unsigned int func, unsigned int func_end);

/* convenience */
void dump_stack();
#define LS_ABORT() do { dump_stack(); assert(0); } while (0)

#endif
