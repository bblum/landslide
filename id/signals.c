/**
 * @file signals.c
 * @brief handling ^C etc
 * @author Ben Blum <bblum@andrew.cmu.edu>
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

#define _XOPEN_SOURCE 700
#define _GNU_SOURCE

#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "common.h"
#include "pp.h"
#include "signals.h"

static void handle_sigint(int MAYBE_UNUSED signum)
{
	pid_t me = syscall(SYS_gettid);
	DBG("ctrl-C press handled by thread %u\n", me);
	ERR("ctrl-C pressed, aborting...\n");
	try_print_live_data_race_pps();
	WARN("\n");
	WARN("some landslide processes may be left hanging; please 'killall bochs'.\n");
	exit(ID_EXIT_CRASH);
}

void init_signal_handling()
{
	struct sigaction act;
	act.sa_handler = handle_sigint;
	sigemptyset(&act.sa_mask);
	int ret = sigaction(SIGINT, &act, NULL);
	assert(ret == 0 && "failed init signal handling");
	pid_t me = syscall(SYS_gettid);
	DBG("signal handling inited by thread %u\n", me);
}
