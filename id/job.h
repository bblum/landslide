/**
 * @file job.h
 * @brief job management
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

#ifndef __ID_JOB_H
#define __ID_JOB_H

#include <pthread.h>

#include "io.h"
#include "messaging.h"
#include "time.h"

struct pp_set;

struct job {
	/* local state */
	struct pp_set *config; /* shared but read-only after init */
	unsigned int id;
	unsigned int generation; /* max among generations of pps + 1 */
	bool should_reproduce;
	/* static config should not change between jobs, and defines cpp macros
	 * that cause landslide recompiles. dynamic config defines pps and such
	 * and is interpreted more "at runtime" by the build glue, to avoid
	 * costly recompiles each time a new job starts. */
	struct file config_static;
	struct file config_dynamic;
	struct file log_stdout;
	struct file log_stderr;

	/* stats -- writable by owner, readable by display thread.
	 * LOCK NOTICE: this is taken while workqueue lock is held. */
	pthread_rwlock_t stats_lock;
	unsigned int elapsed_branches;
	long double estimate_proportion;
	struct human_friendly_time estimate_elapsed;
	struct human_friendly_time estimate_eta;
	long double estimate_eta_numeric;
	/* job lifecycle */
	bool cancelled;
	bool complete;
	bool timed_out;
	bool kill_job;
	/* associated files */
	char *log_filename;
	char *trace_filename;
	bool need_rerun;
	unsigned long fab_timestamp;
	unsigned long fab_cputime;
	unsigned long current_cpu;
	/* used iff -C option (control_experiment) is provided */
	unsigned int icb_current_bound; /* last completed bound = this - 1 */
	unsigned int icb_fab_preemptions; /* used only when FAB */

	/* misc shared state */
	enum { JOB_NORMAL, JOB_BLOCKED, JOB_DONE } status;
	pthread_cond_t done_cvar; /* workqueue thread waits on this */
	pthread_cond_t blocking_cvar; /* job thread waits on this */
	pthread_mutex_t lifecycle_lock;
};

void set_job_options(char *test_name, char *trace_dir, bool verbose,
		     bool leave_logs, bool pintos, bool use_icb,
		     bool preempt_everywhere, bool pure_hb,
		     bool txn, bool txn_abort_codes, bool txn_dont_retry,
		     bool txn_retry_sets, bool txn_weak_atomicity,
		     bool veirf_mode, bool pathos);
bool testing_pintos();
bool testing_pathos();

struct job *new_job(struct pp_set *config, bool should_reproduce);
void start_job(struct job *j);
bool wait_on_job(struct job *j); /* true if job blocked, false if done */
void resume_job(struct job *j);

void job_block(struct job *j); /* to be called by job itself */
void print_job_stats(struct job *j, bool pending, bool blocked);
int compare_job_eta(struct job *j0, struct job *j1);

#endif
