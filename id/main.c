/**
 * @file main.c
 * @brief iterative deepening framework for landslide
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

#define _XOPEN_SOURCE 700

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include "bug.h"
#include "common.h"
#include "job.h"
#include "option.h"
#include "pp.h"
#include "signals.h"
#include "time.h"
#include "work.h"

bool control_experiment;
unsigned long eta_factor;
unsigned long eta_threshold;
bool avoid_recompile;

int main(int argc, char **argv)
{
	char test_name[BUF_SIZE];
	char trace_dir[BUF_SIZE];
	unsigned long max_time;
	unsigned long num_cpus;
	bool verbose;
	bool leave_logs;
	bool use_wrapper_log;
	char wrapper_log[BUF_SIZE];
	bool pintos;
	bool pathos;
	bool use_icb;
	bool preempt_everywhere;
	bool pure_hb;
	bool txn;
	bool txn_abort_codes;
	bool txn_dont_retry;
	bool txn_retry_sets;
	bool txn_weak_atomicity;
	bool verif_mode;
	unsigned long progress_interval;

	if (!get_options(argc, argv, test_name, BUF_SIZE, &max_time, &num_cpus,
			 &verbose, &leave_logs, &control_experiment,
			 &use_wrapper_log, wrapper_log, BUF_SIZE, &pintos,
			 &use_icb, &preempt_everywhere, &pure_hb,
			 &avoid_recompile,
			 &txn, &txn_abort_codes, &txn_dont_retry,
			 &txn_retry_sets, &txn_weak_atomicity,
			 &verif_mode, &pathos, &progress_interval,
			 trace_dir, BUF_SIZE, &eta_factor, &eta_threshold)) {
		usage(strcmp(argv[0], "./landslide-id") == 0 ? "./landslide" : argv[0]);
		exit(ID_EXIT_USAGE);
	}

	set_logging_options(use_wrapper_log, wrapper_log, trace_dir);

	DBG("will run for at most %lu seconds\n", max_time);

	set_job_options(test_name, trace_dir, verbose, leave_logs, pintos, use_icb, preempt_everywhere, pure_hb, txn, txn_abort_codes, txn_dont_retry, txn_retry_sets, txn_weak_atomicity, verif_mode, pathos);
	init_signal_handling();
	start_time(max_time * 1000000, num_cpus);

	if (!control_experiment && !verif_mode &&
	    !(strstr(test_name, "atomic_") == test_name)) {
		add_work(new_job(create_pp_set(PRIORITY_NONE), false));
		add_work(new_job(create_pp_set(PRIORITY_MUTEX_LOCK), true));
		add_work(new_job(create_pp_set(PRIORITY_MUTEX_UNLOCK), true));
		if (testing_pintos()) {
			add_work(new_job(create_pp_set(PRIORITY_CLI), true));
			add_work(new_job(create_pp_set(PRIORITY_STI), true));
		}
	}
	add_work(new_job(create_pp_set(PRIORITY_MUTEX_LOCK | PRIORITY_MUTEX_UNLOCK | PRIORITY_CLI | PRIORITY_STI), true));
	start_work(num_cpus, progress_interval);
	wait_to_finish_work();
	print_live_data_race_pps();
	print_free_re_malloc_false_positives();

	unsigned long cputime = total_cpu_time();
	unsigned long saturation = (cputime / num_cpus) * 100 / time_elapsed();
	struct human_friendly_time cputime_hft;
	human_friendly_time(cputime, &cputime_hft);
	PRINT("total CPU time consumed: ");
	print_human_friendly_time(&cputime_hft);
	PRINT(" (%lu usecs) (core saturation: %lu%%)\n", cputime, saturation);

	return found_any_bugs() ? ID_EXIT_BUG_FOUND : ID_EXIT_SUCCESS;
}
