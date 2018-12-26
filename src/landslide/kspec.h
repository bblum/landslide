/**
 * @file kspec.h
 * @brief Kernel specification
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

#ifndef __LS_KSPEC_H
#define __LS_KSPEC_H

#include "compiler.h"
#include "student_specifics.h" /* to know whether pebbles or pintos */

#ifdef PINTOS_KERNEL

/******** PINTOS ********/

#define SEGSEL_KERNEL_CS 0x08
#define SEGSEL_KERNEL_DS 0x10
#define SEGSEL_USER_CS 0x1B
#define SEGSEL_USER_DS 0x23

#define KERN_MEM_START 0xc0000000
#define KERNEL_MEMORY(addr) ({ ASSERT_UNSIGNED(addr); (addr) >= KERN_MEM_START; })
#define USER_MEMORY(addr)   ({ ASSERT_UNSIGNED(addr); (addr) <  KERN_MEM_START; })

// TODO - different syscall interface. Numbers defined in lib/syscall-nr.h.
#define FORK_INT            0x41
#define EXEC_INT            0x42
#define WAIT_INT            0x44
#define YIELD_INT           0x45
#define DESCHEDULE_INT      0x46
#define MAKE_RUNNABLE_INT   0x47
#define GETTID_INT          0x48
#define NEW_PAGES_INT       0x49
#define REMOVE_PAGES_INT    0x4A
#define SLEEP_INT           0x4B
#define GETCHAR_INT         0x4C
#define READLINE_INT        0x4D
#define PRINT_INT           0x4E
#define SET_TERM_COLOR_INT  0x4F
#define SET_CURSOR_POS_INT  0x50
#define GET_CURSOR_POS_INT  0x51
#define THREAD_FORK_INT     0x52
#define GET_TICKS_INT       0x53
#define MISBEHAVE_INT       0x54
#define HALT_INT            0x55
#define LS_INT              0x56
#define TASK_VANISH_INT     0x57
#define SET_STATUS_INT      0x59
#define VANISH_INT          0x60
#define CAS2I_RUNFLAG_INT   0x61
#define SWEXN_INT           0x74

#else

/******** PEBBLES ********/

#define SEGSEL_KERNEL_CS 0x10
#define SEGSEL_KERNEL_DS 0x18
#define SEGSEL_USER_CS 0x23
#define SEGSEL_USER_DS 0x2b

#define USER_MEM_START 0x01000000
#define KERNEL_MEMORY(addr) ({ ASSERT_UNSIGNED(addr); (addr) <  USER_MEM_START; })
#define USER_MEMORY(addr)   ({ ASSERT_UNSIGNED(addr); (addr) >= USER_MEM_START; })

#define FORK_INT            0x41
#define EXEC_INT            0x42
/* #define EXIT_INT            0x43 */
#define WAIT_INT            0x44
#define YIELD_INT           0x45
#define DESCHEDULE_INT      0x46
#define MAKE_RUNNABLE_INT   0x47
#define GETTID_INT          0x48
#define NEW_PAGES_INT       0x49
#define REMOVE_PAGES_INT    0x4A
#define SLEEP_INT           0x4B
#define GETCHAR_INT         0x4C
#define READLINE_INT        0x4D
#define PRINT_INT           0x4E
#define SET_TERM_COLOR_INT  0x4F
#define SET_CURSOR_POS_INT  0x50
#define GET_CURSOR_POS_INT  0x51
#define THREAD_FORK_INT     0x52
#define GET_TICKS_INT       0x53
#define MISBEHAVE_INT       0x54
#define HALT_INT            0x55
#define LS_INT              0x56
#define TASK_VANISH_INT     0x57 /* previously known as TASK_EXIT_INT */
#define SET_STATUS_INT      0x59
#define VANISH_INT          0x60
#define CAS2I_RUNFLAG_INT   0x61
#define SWEXN_INT           0x74

#endif

#endif
