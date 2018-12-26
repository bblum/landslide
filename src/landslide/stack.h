/**
 * @file stack.h
 * @brief stack tracing
 * @author Ben Blum
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
