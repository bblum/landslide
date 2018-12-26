/**
 * @file student.c
 * @brief Guest-implementation-specific things landslide needs to know
 *        Implementation for the POBBLES kernel.
 *
 * @author Ben Blum, and you, the student :)
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

#define MODULE_NAME "student_glue"

#include "common.h"
#include "kernel_specifics.h"
#include "schedule.h"
#include "x86.h"

/* List of primitives to help you implement these functions:
 * READ_MEMORY(cpu, addr) - Returns 4 bytes from kernel memory at addr
 * READ_BYTE(cpu, addr)   - Returns 1 byte from kernel memory at addr
 * GET_CPU_ATTR(cpu, eax) - Returns the value of a CPU register (esp, eip, ...)
 * GET_ESP0(cpu)          - Returns the current value of esp0
 */

/* Is the currently-running thread not on the runqueue, and is runnable
 * anyway? For kernels that keep the current thread on the runqueue, this
 * function should return false always. */
bool kern_current_extra_runnable(cpu_t *cpu)
{
#ifdef CURRENT_THREAD_LIVES_ON_RQ
	assert(CURRENT_THREAD_LIVES_ON_RQ == 1 ||
	       CURRENT_THREAD_LIVES_ON_RQ == 0);
	return !(bool)CURRENT_THREAD_LIVES_ON_RQ;
#else
	STATIC_ASSERT(false && "CURRENT_THREAD_LIVES_ON_RQ config missing -- must implement in student.c manually.");
	return false;
#endif
}

bool kern_ready_for_timer_interrupt(cpu_t *cpu)
{
#ifdef PREEMPT_ENABLE_FLAG
#ifndef PREEMPT_ENABLE_VALUE
	STATIC_ASSERT(false && "preempt flag but not value defined");
	return false;
#else
	return READ_MEMORY(cpu, (unsigned int)PREEMPT_ENABLE_FLAG) == PREEMPT_ENABLE_VALUE;
#endif
#else
	/* no preempt enable flag. assume the scheduler protects itself by
	 * disabling interrupts wholesale. */
	return true;
#endif
}

