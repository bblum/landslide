/**
 * @file user_specifics.h
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

#ifndef __LS_USER_SPECIFICS_H
#define __LS_USER_SPECIFICS_H

#include "simulator.h"
#include "student_specifics.h"

struct ls_state;

/* Userspace */

bool testing_userspace();
bool ignore_dr_function(unsigned int eip);
bool ignore_thrlib_function(unsigned int eip);
/* syscalls / misc */
bool user_report_end_fail(cpu_t *cpu, unsigned int eip);
bool user_yielding(struct ls_state *ls);
bool user_make_runnable_entering(unsigned int eip);
bool user_sleep_entering(unsigned int eip);
/* malloc */
bool user_mm_init_entering(unsigned int eip);
bool user_mm_init_exiting(unsigned int eip);
bool user_mem_sbrk_entering(unsigned int eip);
bool user_mem_sbrk_exiting(unsigned int eip);
bool user_mm_malloc_entering(cpu_t *cpu, unsigned int eip, unsigned int *size);
bool user_mm_malloc_exiting(cpu_t *cpu, unsigned int eip, unsigned int *base);
bool user_mm_free_entering(cpu_t *cpu, unsigned int eip, unsigned int *base);
bool user_mm_free_exiting(unsigned int eip);
bool user_mm_realloc_entering(cpu_t *cpu, unsigned int eip,
			      unsigned int *orig_base, unsigned int *size);
bool user_mm_realloc_exiting(cpu_t *cpu, unsigned int eip, unsigned int *base);
bool user_locked_malloc_entering(unsigned int eip);
bool user_locked_malloc_exiting(unsigned int eip);
bool user_locked_free_entering(unsigned int eip);
bool user_locked_free_exiting(unsigned int eip);
bool user_locked_calloc_entering(unsigned int eip);
bool user_locked_calloc_exiting(unsigned int eip);
bool user_locked_realloc_entering(unsigned int eip);
bool user_locked_realloc_exiting(unsigned int eip);
/* elf regions */
bool user_address_in_heap(unsigned int addr);
bool user_address_global(unsigned int addr);
bool user_panicked(cpu_t *cpu, unsigned int addr, char **buf);
/* thread lifecycle */
bool user_thr_init_entering(unsigned int eip);
bool user_thr_init_exiting(unsigned int eip);
bool user_thr_create_entering(unsigned int eip);
bool user_thr_create_exiting(unsigned int eip);
bool user_thr_join_entering(unsigned int eip);
bool user_thr_join_exiting(unsigned int eip);
bool user_thr_exit_entering(unsigned int eip);
/* mutexes */
bool user_mutex_init_entering(cpu_t *cpu, unsigned int eip, unsigned int *addr);
bool user_mutex_init_exiting(unsigned int eip);
bool user_mutex_lock_entering(cpu_t *cpu, unsigned int eip, unsigned int *addr);
bool user_mutex_lock_exiting(unsigned int eip);
bool user_mutex_trylock_entering(cpu_t *cpu, unsigned int eip, unsigned int *addr);
bool user_mutex_trylock_exiting(cpu_t *cpu, unsigned int eip, bool *succeeded);
bool user_mutex_unlock_entering(cpu_t *cpu, unsigned int eip, unsigned int *addr);
bool user_mutex_unlock_exiting(unsigned int eip);
bool user_mutex_destroy_entering(cpu_t *cpu, unsigned int eip, unsigned int *addr);
bool user_mutex_destroy_exiting(unsigned int eip);
/* cvars */
bool user_cond_wait_entering(cpu_t *cpu, unsigned int eip, unsigned int *addr);
bool user_cond_wait_exiting(unsigned int eip);
bool user_cond_signal_entering(cpu_t *cpu, unsigned int eip, unsigned int *addr);
bool user_cond_signal_exiting(unsigned int eip);
bool user_cond_broadcast_entering(cpu_t *cpu, unsigned int eip, unsigned int *addr);
bool user_cond_broadcast_exiting(unsigned int eip);
/* sems */
bool user_sem_wait_entering(cpu_t *cpu, unsigned int eip, unsigned int *addr);
bool user_sem_wait_exiting(unsigned int eip);
bool user_sem_signal_entering(cpu_t *cpu, unsigned int eip, unsigned int *addr);
bool user_sem_signal_exiting(unsigned int eip);
/* rwlox */
bool user_rwlock_lock_entering(cpu_t *cpu, unsigned int eip, unsigned int *addr, bool *write);
bool user_rwlock_lock_exiting(unsigned int eip);
bool user_rwlock_unlock_entering(cpu_t *cpu, unsigned int eip, unsigned int *addr);
bool user_rwlock_unlock_exiting(unsigned int eip);

bool user_xbegin_entering(unsigned int eip);
bool user_xbegin_exiting(unsigned int eip);
bool user_xend_entering(unsigned int eip);
bool user_xabort_entering(cpu_t *cpu, unsigned int eip, unsigned int *xabort_code);
bool user_xtest_exiting(unsigned int eip);

#endif
