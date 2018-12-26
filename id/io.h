/**
 * @file io.h
 * @brief i/o routines for communicating with other landslide processes
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

#ifndef __ID_IO_H
#define __ID_IO_H

#define _GNU_SOURCE

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

// FIXME make more flexible
#define LANDSLIDE_PROGNAME "landslide"
#define LANDSLIDE_PATH "../pebsim"
#define ROOT_PATH ".."

struct file {
	int fd;
	char *filename;
};

void create_file(struct file *f, const char *template);
void delete_file(struct file *f, bool do_remove);

char *create_fifo(const char *prefix, unsigned int id);
void open_fifo(struct file *f, char *name, int flags);
void delete_unused_fifo(char *name);

void move_file_to(struct file *f, const char *dirpath);
void unset_cloexec(int fd);

void set_logging_options(bool use_log, char *filename, const char *log_dir_name);
void log_msg(const char *pfx, const char *format, ...);

bool check_directory(const char *dirpath, int *errno_val);

#endif
