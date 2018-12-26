/**
 * @file kernel_specifics.h
 * @brief Guest-implementation-specific things landslide needs to know.
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

#ifndef __LS_KERNEL_SPECIFICS_H
#define __LS_KERNEL_SPECIFICS_H

#include "simulator.h"
#include "student_specifics.h"

struct sched_state;
struct ls_state;

/* Miscellaneous simple information */
bool kern_decision_point(unsigned int eip);
bool kern_thread_switch(cpu_t *cpu, unsigned int eip, unsigned int *new_tid);
bool kern_timer_entering(unsigned int eip);
bool kern_timer_exiting(unsigned int eip);
int kern_get_timer_wrap_begin(void);
bool kern_context_switch_entering(unsigned int eip);
bool kern_context_switch_exiting(unsigned int eip);
bool kern_sched_init_done(unsigned int eip);
bool kern_in_scheduler(cpu_t *cpu, unsigned int eip);
bool kern_access_in_scheduler(unsigned int addr);
bool kern_ready_for_timer_interrupt(cpu_t *cpu);
bool kern_within_functions(struct ls_state *ls);
bool kern_enter_disk_io_fn(unsigned int eip);
bool kern_exit_disk_io_fn(unsigned int eip);
void read_panic_message(cpu_t *cpu, unsigned int eip, char **buf);
bool kern_panicked(cpu_t *cpu, unsigned int eip, char **buf);
bool kern_page_fault_handler_entering(unsigned int eip);
bool kern_killed_faulting_user_thread(cpu_t *cpu, unsigned int eip);
bool kern_kernel_main(unsigned int eip);

/* Yielding-mutex interactions. */
bool kern_mutex_initing(cpu_t *cpu, unsigned int eip, unsigned int *mutex, bool *isnt_mutex);
bool kern_mutex_locking(cpu_t *cpu, unsigned int eip, unsigned int *mutex);
bool kern_mutex_blocking(cpu_t *cpu, unsigned int eip, unsigned int *tid);
bool kern_mutex_locking_done(cpu_t *cpu, unsigned int eip, unsigned int *mutex);
bool kern_mutex_unlocking(cpu_t *cpu, unsigned int eip, unsigned int *mutex);
bool kern_mutex_unlocking_done(unsigned int eip);
bool kern_mutex_trylocking(cpu_t *cpu, unsigned int eip, unsigned int *mutex);
bool kern_mutex_trylocking_done(cpu_t *cpu, unsigned int eip, unsigned int *mutex, bool *success);

/* Lifecycle */
bool kern_forking(unsigned int eip);
bool kern_vanishing(unsigned int eip);
bool kern_sleeping(unsigned int eip);
bool kern_readline_enter(unsigned int eip);
bool kern_readline_exit(unsigned int eip);
bool kern_exec_enter(unsigned int eip);
bool kern_thread_runnable(cpu_t *cpu, unsigned int eip, unsigned int *);
bool kern_thread_descheduling(cpu_t *cpu, unsigned int eip, unsigned int *);
bool kern_beginning_vanish_before_unreg_process(unsigned int eip);

/* Dynamic memory allocation */
bool kern_lmm_alloc_entering(cpu_t *cpu, unsigned int eip, unsigned int *size);
bool kern_lmm_alloc_exiting(cpu_t *cpu, unsigned int eip, unsigned int *base);
bool kern_lmm_free_entering(cpu_t *cpu, unsigned int eip, unsigned int *base);
bool kern_lmm_free_exiting(unsigned int eip);
bool kern_lmm_init_entering(unsigned int eip);
bool kern_lmm_init_exiting(unsigned int eip);

bool kern_page_alloc_entering(cpu_t *cpu, unsigned int eip, unsigned int *size);
bool kern_page_alloc_exiting(cpu_t *cpu, unsigned int eip, unsigned int *base);
bool kern_page_free_entering(cpu_t *cpu, unsigned int eip, unsigned int *base);
bool kern_page_free_exiting(unsigned int eip);

/* Other memory operations */
bool kern_address_in_heap(unsigned int addr);
bool kern_address_global(unsigned int addr);

/* Other / init */
int kern_get_init_tid(void);
int kern_get_idle_tid(void);
int kern_get_shell_tid(void);
int kern_get_first_tid(void);
bool kern_has_shell(void);
bool kern_has_idle(void);
#define TID_IS_INIT(tid) ((tid) == kern_get_init_tid())
#define TID_IS_SHELL(tid) (kern_has_shell() && (tid) == kern_get_shell_tid())
#define TID_IS_IDLE(tid) (kern_has_idle() && (tid) == kern_get_idle_tid())
void kern_init_threads(struct sched_state *,
		       void (*)(struct sched_state *, unsigned int, bool));
bool kern_current_extra_runnable(cpu_t *cpu);
bool kern_wants_us_to_dump_stack(unsigned int eip);
bool kern_vm_user_copy_enter(unsigned int eip);
bool kern_vm_user_copy_exit(unsigned int eip);

#endif
