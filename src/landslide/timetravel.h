/**
 * @file timetravel.h
 * @brief checkpoint and restore interface
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

#ifndef __LS_TIMETRAVEL_H
#define __LS_TIMETRAVEL_H

#include <stdbool.h>

#include "student_specifics.h" /* for ABORT_SETS */

struct nobe;
struct abort_set;

#ifdef BOCHS

struct timetravel_state {
	int pipefd; /* used to communicate exit status */
};

struct timetravel_pp {
	bool active;
	bool parent;
	int pipefd;
};

void timetravel_init(struct timetravel_state *ts);
#define timetravel_pp_init(th) do { (th)->active = false; } while (0)

/* Time travel is implemented by fork()ing the simulation at each PP.
 * Accordingly, any landslide state which should "glow green" must be updated
 * very carefully -- i.e., the forked processes must see all changes by DPOR/
 * estimation/etc. To accomplish this, all nobes are protected by const, and
 * in order to change them, you need to go through this function. */
#include <type_traits>
void __modify_pps(void (*cb)(struct nobe *h_rw, void *), const struct nobe *h_ro,
		    void *arg, unsigned int arg_size);
template <typename T> inline void modify_pp(void (*cb)(struct nobe *h_rw, T *),
					     const struct nobe *h_ro, T arg)
{
	assert(h_ro != NULL);
#ifndef HTM_ABORT_SETS
	/* prevent sending pointers which might be invalid in another process;
	 * abort sets unfortunately needs to send a struct of 4 ints through
	 * (see explore.c), which is legal, but is-fundamental can't check */
	STATIC_ASSERT(std::is_fundamental<T>::value && "no pointers allowed!");
#endif
	__modify_pps((void (*)(struct nobe *, void *))cb, h_ro, &arg, sizeof(arg));
	/* also update the version in our local memory, of course */
	cb((struct nobe *)h_ro, &arg);
}

#else /* SIMICS */

struct timetravel_state { char *cmd_file; };
struct timetravel_pp { };

#define timetravel_init(ts)     do { (ts)->cmd_file = NULL; } while (0)
#define timetravel_pp_init(th) do { } while (0)
#define modify_pp(cb, h_ro, arg) ((cb)((struct nobe *)(h_ro), (arg)))

#endif

bool timetravel_set(struct ls_state *ls, struct nobe *h,
		    unsigned int *tid, bool *txn, unsigned int *xabort_code,
		    struct abort_set *aborts);
void timetravel_jump(struct ls_state *ls, const struct timetravel_pp *tt,
		     unsigned int tid, bool txn, unsigned int xabort_code,
		     struct abort_set *aborts);
void timetravel_delete(struct ls_state *ls, const struct timetravel_pp *tt);

#endif /* __LS_TIMETRAVEL_H */
