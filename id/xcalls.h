/**
 * @file xcalls.h
 * @brief convenience wrapper macros for syscalls
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

#ifndef __ID_SYSCALL_H
#define __ID_SYSCALL_H

#include <errno.h>
#include <string.h>
#include <sys/time.h>

#define EXPECT(cond, ...) do {						\
		int __last_errno = errno;				\
		if (!(cond)) {						\
			ERR("Assertion failed: '%s'\n", #cond);		\
			ERR(__VA_ARGS__);				\
			ERR("Error: %s\n", strerror(__last_errno));	\
		}							\
	} while (0)

// FIXME: convert to eval-argument-once form ("int __arg = (arg);")

/* you couldn't make this shit up if you tried */
#define scnprintf(buf, maxlen, ...) ({					\
	int __snprintf_ret = snprintf((buf), (maxlen), __VA_ARGS__);	\
	if (__snprintf_ret > (int)(maxlen)) {				\
		__snprintf_ret = (maxlen);				\
	}								\
	__snprintf_ret; })


#define XMALLOC(x,t) ({							\
	typeof(t) *__xmalloc_ptr = malloc((x) * sizeof(t));		\
	EXPECT(__xmalloc_ptr != NULL, "malloc failed");			\
	__xmalloc_ptr; })

#define XSTRDUP(s) ({							\
	char *__s = (s);						\
	__s == NULL ? NULL : ({						\
	int __len = strlen(__s);					\
	char *__buf = XMALLOC(__len + 1, char);				\
	int __ret = scnprintf(__buf, __len + 1, "%s", __s);		\
	EXPECT(__ret == __len, "scprintf failed");			\
	__buf; }); })

#define FREE(x) free(x)

// FIXME: deal with short writes
#define XWRITE(file, ...) do {						\
		char __buf[BUF_SIZE];					\
		int __len = scnprintf(__buf, BUF_SIZE, __VA_ARGS__);	\
		int __ret = write((file)->fd, __buf, __len);		\
		EXPECT(__ret == __len, "failed write to file '%s'\n",	\
		       (file)->filename);				\
	} while (0)

#define XCLOSE(fd) do {							\
		int __ret = close(fd);					\
		EXPECT(__ret == 0, "failed close fd %d\n", fd);		\
	} while (0)

#define XREMOVE(filename) do {						\
		int __ret = remove(filename);				\
		EXPECT(__ret == 0, "failed remove '%s'\n", (filename));	\
	} while (0)

#define XPIPE(pipefd) do {						\
		int __ret = pipe2(pipefd, O_CLOEXEC);			\
		EXPECT(__ret == 0, "failed create pipe\n");		\
	} while (0)

#define XCHDIR(path) do {						\
		int __ret = chdir(path);				\
		EXPECT(__ret == 0, "failed chdir to '%s'\n", (path));	\
	} while (0)

#define XRENAME(oldpath, newpath) do {					\
		int __ret = rename((oldpath), (newpath));		\
		EXPECT(__ret == 0, "failed rename '%s' to '%s'\n",	\
		       (oldpath), (newpath));				\
	} while (0)

#define XDUP2(oldfd, newfd) do {					\
		int __newfd = (newfd);					\
		int __ret = dup2((oldfd), __newfd);			\
		EXPECT(__ret == __newfd, "failed dup2 %d <- %d\n",	\
		       (oldfd), __newfd);				\
	} while (0)

#define XGETTIMEOFDAY(tv) do {						\
		int __ret = gettimeofday((tv), NULL);			\
		EXPECT(__ret == 0, "failed gettimeofday");		\
	} while (0)

#endif
