/**
 * @file option.h
 * @brief command line options
 * @author Ben Blum <bblum@andrew.cmu.edu>
 */

#ifndef __ID_OPTION_H
#define __ID_OPTION_H

void usage(char *execname);

bool get_options(int argc, char **argv, char *test_name, unsigned int test_name_len,
		 unsigned long *max_time, unsigned long *num_cpus, bool *verbose,
		 bool *leave_logs, bool *control_experiment, bool *use_wrapper_log,
		 char *wrapper_log, unsigned int wrapper_log_len, bool *pintos,
		 bool *use_icb, bool *preempt_everywhere, bool *pure_hb,
		 bool *avoid_recompile,
		 bool *txn, bool *txn_abort_codes, bool *txn_dont_retry,
		 bool *txn_retry_sets, bool *verif_mode,
		 bool *pathos, unsigned long *progress_report_interval,
		 char *trace_dir, unsigned int trace_dir_len,
		 unsigned long *eta_factor, unsigned long *eta_thresh);

#endif
