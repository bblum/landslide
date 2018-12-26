/**
 * @file option.c
 * @brief command line options
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

#include <ctype.h> /* isprint */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h> /* get_nprocs */
#include <unistd.h> /* getopt */

#include "array_list.h"
#include "common.h"
#include "option.h"
#include "io.h"

#define MINTIME ((unsigned long)600) /* 10 mins */
#define DEFAULT_TIME "1h"
#define DEFAULT_TEST_CASE "thr_exit_join"
#define DEFAULT_PROGRESS_INTERVAL "10" /* seconds */

/* The ETA factor heuristic controls how optimistic/pessimistic we are about
 * descheduling "too large" state spaces. At any point if the ETA for a state
 * space is [this many] times bigger than the remaining time budget, we postpone
 * it and run a fresh job instead. (If no non-postponed jobs exist, we run the
 * postponed jobs, preferring the ones with lowest ETA.) */
#define DEFAULT_ETA_FACTOR "2"

/* ...However, very early on in a state space, the estimates are notoriously
 * inaccurate. We grant each job this many "free" branches to let its estimate
 * to stabilize somewhat, before applying the above heuristic. */
#define DEFAULT_ETA_STABILITY_THRESHOLD "32"

struct cmdline_option {
	char flag;
	bool requires_arg;
	char *name; /* iff requires_arg */
	char *description;
	char *default_value; /* iff requires arg */
	char **value_ptr; /* iff requires_arg */
	bool *bool_ptr; /* iff NOT requires_arg */
	bool is_secret; /* skip this option when printing help text */
};

static ARRAY_LIST(struct cmdline_option) cmdline_options;
static bool ready = false;
static bool secret_help = false;

void usage(char *execname)
{
	assert(ready);
	printf(COLOUR_BOLD "Usage: %s ", execname);
	unsigned int i;
	struct cmdline_option *optp;
	ARRAY_LIST_FOREACH(&cmdline_options, i, optp) {
		if (!optp->is_secret || secret_help) {
			if (optp->requires_arg) {
				printf("[-%c %s] ", optp->flag, optp->name);
			} else {
				printf("[-%c] ", optp->flag);
			}
		}
	}
	printf("\n");

	ARRAY_LIST_FOREACH(&cmdline_options, i, optp) {
		if (!optp->is_secret || secret_help) {
			if (optp->requires_arg) {
				printf("\t-%c %s:\t%s (default %s)\n", optp->flag,
				       optp->name, optp->description,
				       optp->default_value == NULL ? "<none>"
				       : optp->default_value);
			} else {
				printf("\t-%c:\t\t%s\n", optp->flag,
				       optp->description);
			}
		}
	}
	printf(COLOUR_DEFAULT);
}

static bool parse_time(char *str, unsigned long *result)
{
	char *endp;
	long time = strtol(str, &endp, 0);
	if (errno != 0) {
		ERR("Time must be a number (got '%s')\n", str);
		return false;
	}
	if (time < 0) {
		ERR("Cannot time travel\n");
		return false;
	}
	*result = (unsigned long)time;
	switch (*endp) {
		case 'y':
			WARN("%ld year%s, are you sure?\n", *result,
			     *result == 1 ? "" : "s");
			*result *= 365;
		case 'd':
			*result *= 24;
		case 'h':
			*result *= 60;
		case 'm':
			*result *= 60;
		case '\0':
		case 's':
			break;
		default:
			ERR("Unrecognized time format '%s'\n", str);
			return false;
	}
	return true;
}

bool get_options(int argc, char **argv, char *test_name, unsigned int test_name_len,
		 unsigned long *max_time, unsigned long *num_cpus, bool *verbose,
		 bool *leave_logs, bool *control_experiment, bool *use_wrapper_log,
		 char *wrapper_log, unsigned int wrapper_log_len, bool *pintos,
		 bool *use_icb, bool *preempt_everywhere, bool *pure_hb,
		 bool *avoid_recompile,
		 bool *txn, bool *txn_abort_codes, bool *txn_dont_retry,
		 bool *txn_retry_sets, bool *txn_weak_atomicity,
		 bool *verif_mode,
		 bool *pathos, unsigned long *progress_report_interval,
		 char *trace_dir, unsigned int trace_dir_len,
		 unsigned long *eta_factor, unsigned long *eta_thresh)
{
	/* Set up cmdline options & their default values */
	unsigned int system_cpus = get_nprocs();
	assert(system_cpus > 0);
	char half_the_cpus[BUF_SIZE];
	scnprintf(half_the_cpus, BUF_SIZE, "%u",
		  MAX((unsigned int)1, system_cpus / 2));

	ARRAY_LIST_INIT(&cmdline_options, 16);

	char getopt_buf[BUF_SIZE];
	getopt_buf[0] = '\0';

#define DEF_CMDLINE_FLAG(flagname, secret, varname, descr)		\
	bool arg_##varname = false;					\
	/* define global option struct */				\
	struct cmdline_option __arg_##varname##_struct;			\
	__arg_##varname##_struct.flag          = flagname;		\
	__arg_##varname##_struct.requires_arg  = false;			\
	__arg_##varname##_struct.name          = NULL;			\
	__arg_##varname##_struct.default_value = NULL;			\
	__arg_##varname##_struct.description   = XSTRDUP(descr);	\
	__arg_##varname##_struct.value_ptr     = NULL;			\
	__arg_##varname##_struct.bool_ptr      = &arg_##varname;	\
	__arg_##varname##_struct.is_secret     = secret;		\
	ARRAY_LIST_APPEND(&cmdline_options, __arg_##varname##_struct);	\
	/* append getopt_buf with flagname */				\
	unsigned int __buf_index_##varname = strlen(getopt_buf);	\
	assert(__buf_index_##varname + 1 < BUF_SIZE);			\
	getopt_buf[__buf_index_##varname] = flagname;			\
	getopt_buf[__buf_index_##varname + 1] = '\0';			\

	DEF_CMDLINE_FLAG('v', false, verbose, "Verbose output");
	DEF_CMDLINE_FLAG('h', false, help, "Print this help text and exit");
	DEF_CMDLINE_FLAG('s', false, secret_help, "Also show 'secret' options for advanced users");
	DEF_CMDLINE_FLAG('l', false, leave_logs, "Don't delete log files from bug-free state spaces");
	DEF_CMDLINE_FLAG('C', true, control_experiment, "Control mode, i.e., test only 1 maximal state space");
	DEF_CMDLINE_FLAG('P', true, pintos, "Pintos (not for 15-410 use)");
	DEF_CMDLINE_FLAG('4', true, pathos, "Pathos (for 15-410 TA use only)");
	DEF_CMDLINE_FLAG('I', true, icb, "Use Iterative Context Bounding (ICB) to order the search (-C only)");
	DEF_CMDLINE_FLAG('0', true, everywhere, "Preempt unconditionally on all heap/global accesses (-C only)");
	// LHB is default if testing pintos or pathos.
	// PHB is default if testing P2s (new as of s17!).
	DEF_CMDLINE_FLAG('H', true, limited_hb, "Use \"limited\" happens-before data-race analysis");
	DEF_CMDLINE_FLAG('V', true, pure_hb, "Use vector clocks for \"pure\" happens-before data-races");
	// "o" for "hOt"; see work.c/messaging.c >.>
	DEF_CMDLINE_FLAG('o', true, avoid_recompile, "Try to abort if Landslide needed to recompile itself");
	// HTM options
	DEF_CMDLINE_FLAG('X', true, txn, "Enable transactional-memory testing options");
	DEF_CMDLINE_FLAG('A', true, txn_abort_codes, "Support multiple xabort failure codes (warning: exponential)");
	DEF_CMDLINE_FLAG('S', true, txn_dont_retry, "STM semantics (suppress _XABORT_RETRY failures) (requires -A)");
	DEF_CMDLINE_FLAG('R', true, txn_retry_sets, "Retry set reduction (incompatible with -A/-S)");
	DEF_CMDLINE_FLAG('W', true, txn_weak_atomicity, "Weak atomicity (non-txn can preempt txn) (requires -S)");
	DEF_CMDLINE_FLAG('M', false, verif_mode, "Optimize for faster verification (maximal state space only)");
#undef DEF_CMDLINE_FLAG

#define DEF_CMDLINE_OPTION(flagname, secret, varname, descr, value)	\
	char *arg_##varname = value;					\
	/* define global option struct */				\
	struct cmdline_option __arg_##varname##_struct;			\
	__arg_##varname##_struct.flag          = flagname;		\
	__arg_##varname##_struct.requires_arg  = true;			\
	__arg_##varname##_struct.name          = XSTRDUP(#varname);	\
	__arg_##varname##_struct.default_value = XSTRDUP(value);	\
	__arg_##varname##_struct.description   = XSTRDUP(descr);	\
	__arg_##varname##_struct.value_ptr     = &arg_##varname;	\
	__arg_##varname##_struct.bool_ptr      = NULL;			\
	__arg_##varname##_struct.is_secret     = secret;		\
	ARRAY_LIST_APPEND(&cmdline_options, __arg_##varname##_struct);	\
	/* append getopt_buf with flagname (and a ":") */		\
	unsigned int __buf_index_##varname = strlen(getopt_buf);	\
	assert(__buf_index_##varname + 2 < BUF_SIZE);			\
	getopt_buf[__buf_index_##varname] = flagname;			\
	getopt_buf[__buf_index_##varname + 1] = ':';			\
	getopt_buf[__buf_index_##varname + 2] = '\0';			\

	DEF_CMDLINE_OPTION('p', false, test_name, "Userspace test program name", DEFAULT_TEST_CASE);
	DEF_CMDLINE_OPTION('t', false, max_time, "Total time budget (suffix s/m/d/h/y)", DEFAULT_TIME);
	DEF_CMDLINE_OPTION('c', false, num_cpus, "How many CPUs to use", half_the_cpus);
	DEF_CMDLINE_OPTION('i', false, interval, "Progress report interval", DEFAULT_PROGRESS_INTERVAL);
	DEF_CMDLINE_OPTION('d', false, trace_dir, "Directory for trace file output", "");
	DEF_CMDLINE_OPTION('e', true, eta_factor, "ETA factor heuristic", DEFAULT_ETA_FACTOR);
	DEF_CMDLINE_OPTION('E', true, eta_thresh, "ETA threshold heuristic", DEFAULT_ETA_STABILITY_THRESHOLD);
	/* Log file to output PRINT/DBG messages to in addition to console.
	 * Used by wrapper file to tie together which bug traces go where, etc.,
	 * for purpose of snapshotting. */
	DEF_CMDLINE_OPTION('L', true, log_name, "Log filename", NULL);
#undef DEF_CMDLINE_OPTION

	ready = true;

	bool options_valid = true;
	int option_char;
	unsigned int i;
	struct cmdline_option *optp;
	while ((option_char = getopt(argc, argv, getopt_buf)) != -1) {
		bool found = false;
		if (option_char == '?') {
			options_valid = false;
			break;
		}
		ARRAY_LIST_FOREACH(&cmdline_options, i, optp) {
			if (option_char == optp->flag) {
				if (optp->requires_arg) {
					*optp->value_ptr = optarg;
				} else {
					*optp->bool_ptr = true;
				}
				found = true;
				break;
			}
		}
		if (!found) {
			if (isprint(option_char)) {
				WARN("Unrecognized option '%c'\n", option_char);
			} else {
				WARN("Unrecognized option 0x%x\n", option_char);
			}
			options_valid = false;
		}
	}
	
	/* unset dangerous value_ptr field */
	ARRAY_LIST_FOREACH(&cmdline_options, i, optp) {
		optp->value_ptr = NULL;
		optp->bool_ptr  = NULL;
	}

	/* check for junk */
	if (optind < argc) {
		ERR("Unprocessed command line argument: '%s'; "
		    "did you forget to put '-p' before it?\n", argv[optind]);
		options_valid = false;
	}

	/* Interpret string versions of arguments (or their defaults) */

	if (!parse_time(arg_max_time, max_time)) {
		options_valid = false;
	} else if (*max_time >= ULONG_MAX / 1000000) {
		ERR("%ld seconds is too much time for unsigned long\n",
		    *max_time);
		options_valid = false;
	} else if (*max_time < MINTIME) {
		WARN("%ld seconds (%s) not enough time; defaulting to %ld\n",
		     *max_time, arg_max_time, MINTIME);
		*max_time = MINTIME;
	}

	*num_cpus = strtol(arg_num_cpus, NULL, 0);
	if (errno != 0) {
		ERR("num_cpus must be a number (got '%s')\n", arg_num_cpus);
		options_valid = false;
	} else if (*num_cpus == 0) {
		ERR("Cannot use 0 CPUs (%u CPUs available)\n", system_cpus);
		options_valid = false;
	} else if (*num_cpus > system_cpus) {
		WARN("%lu CPUs is too many; we can only use %u\n",
		     *num_cpus, system_cpus);
		*num_cpus = system_cpus;
	}

	if (!parse_time(arg_interval, progress_report_interval)) {
		options_valid = false;
	}

	*eta_factor = strtol(arg_eta_factor, NULL, 0);
	if (errno != 0) {
		ERR("ETA factor heuristic must be a number (got '%s')\n", arg_eta_factor);
		options_valid = false;
	} else if (*eta_factor == 0) {
		ERR("ETA factor heuristic must be >= 1\n");
		options_valid = false;
	}

	*eta_thresh = strtol(arg_eta_thresh, NULL, 0);
	if (errno != 0) {
		ERR("ETA threshold heuristic must be a number (got '%s')\n", arg_eta_factor);
		options_valid = false;
	} else if (*eta_thresh == 0) {
		ERR("ETA stability threshold heuristic must be >= 1\n");
		options_valid = false;
	}

	if (arg_icb && !arg_control_experiment && !arg_verif_mode) {
		ERR("Iterative Deepening & ICB not supported at same time.\n");
		WARN("Perhaps either '-C -I' or '-M -I' may suit your needs?\n");
		options_valid = false;
	}
	if (arg_everywhere && !arg_control_experiment) {
		ERR("Iterative Deepening & Preempt-Everywhere mode not supported at same time.\n");
		options_valid = false;
	}
	if (arg_verif_mode && arg_control_experiment) {
		ERR("Verification mode not supported without iterative deepening.\n");
		options_valid = false;
	}
	if (arg_pintos && arg_pathos) {
		ERR("Make up your mind (pintos/pathos)!\n");
		options_valid = false;
	}
	if (arg_help) {
		options_valid = false;
	}
	if (arg_limited_hb && arg_pure_hb) {
		ERR("Make up your mind (limited/pure happens-before)!\n");
		options_valid = false;
	}

	scnprintf(test_name, test_name_len, "%s", arg_test_name);

	if (arg_txn) {
		if (arg_pintos || arg_pathos) {
			ERR("Can't test TM and kernels at same time\n");
			options_valid = false;
		}
	} else if (arg_txn_abort_codes) {
		ERR("-A (txn abort codes) supplied without -X (txn)\n");
		options_valid = false;
	} else if (arg_txn_retry_sets) {
		ERR("-R (txn retry sets) supplied without -X (txn)\n");
		options_valid = false;
	} else if (strstr(test_name, "htm") == test_name) {
		ERR("You want to use -X with that HTM test case, right?\n");
		options_valid = false;
	}
	if (arg_txn_dont_retry) {
		if (!arg_txn_abort_codes) {
			/* suppressing retry aborts without also enabling the
			 * other transaction failure reasons would basically
			 * make transactions never fail.. logical, but silly */
			ERR("-S (txn dont retry) supplied without -A (txn abort codes)\n");
			options_valid = false;
		}
	} else if (strstr(test_name, "stm") == test_name) {
		ERR("You want to use -X -A -S with that STM test case, right?\n");
		options_valid = false;
	}
	if (arg_txn_retry_sets && arg_txn_abort_codes) {
		ERR("-R (txn retry sets) incompatible with -A (txn abort codes)\n");
		options_valid = false;
	}
	if (arg_txn_weak_atomicity && !arg_txn_dont_retry) {
		ERR("-W (txn weak atomicity) requires -S (txn dont retry)\n");
		options_valid = false;
	}
	if (arg_txn_retry_sets && arg_icb) {
		WARN("-R (txn retry sets) is not known to be sound "
		     "in conjunction with -I (ICB)!\n");
		WARN("Proceed at your own risk...\n");
	}

	if (strstr(test_name, "cyclone") == test_name ||
	    strstr(test_name, "racer") == test_name ||
	    strstr(test_name, "largetest") == test_name ||
	    strstr(test_name, "agility_drill") == test_name ||
	    strstr(test_name, "mandelbrot") == test_name ||
	    strstr(test_name, "bistromath") == test_name ||
	    strstr(test_name, "juggle") == test_name) {
		ERR("%s is not a Landslide-friendly test.\n", test_name);
		WARN("Please consult slides 33 and 34 of the Landslide lecture.\n");
		WARN("If you're sure you want to proceed with this test,\n");
		WARN("email bblum@cs.cmu.edu for advice.\n");
		options_valid = false;
	}

	if ((*use_wrapper_log = (arg_log_name != NULL))) {
		scnprintf(wrapper_log, wrapper_log_len, "%s", arg_log_name);
	}

	scnprintf(trace_dir, trace_dir_len, "%s", arg_trace_dir);
	int errno_val;
	if (trace_dir[0] != 0) {
		if (!check_directory(trace_dir, &errno_val)) {
			ERR("Couldn't open specified trace directory %s: %s\n",
			    trace_dir, strerror(errno_val));
			options_valid = false;
		} else {
			WARN("HTML trace files will be relocated to %s\n", trace_dir);
		}
	}

	*verbose = arg_verbose;
	secret_help = arg_secret_help;
	*leave_logs = arg_leave_logs;
	*control_experiment = arg_control_experiment;
	*pintos = arg_pintos;
	*pathos = arg_pathos;
	*use_icb = arg_icb;
	*preempt_everywhere = arg_everywhere;
	/* purehb is the default for pintos and p2; lhb default for pathos */
	*pure_hb = (!arg_pathos && !arg_limited_hb) || arg_pure_hb;
	*avoid_recompile = arg_avoid_recompile;
	*txn = arg_txn;
	*txn_abort_codes = arg_txn_abort_codes;
	*txn_dont_retry = arg_txn_dont_retry;
	*txn_retry_sets = arg_txn_retry_sets;
	*txn_weak_atomicity = arg_txn_weak_atomicity;
	*verif_mode = arg_verif_mode;

	return options_valid;
}
