/**
 * @file compiler.h
 * @brief Defines some useful macros that Dave might object to. Inspired by
 *        linux's compiler*.h and kernel.h.
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

#ifndef __LS_COMPILER_H
#define __LS_COMPILER_H

/* Function annotations */
// XXX: simics's headers need this not to be defined here
// # define NORETURN __attribute__((noreturn))
#define MUST_CHECK __attribute__((warn_unused_result))

#define MAYBE_UNUSED __attribute__((unused))

/* Force a compilation error if condition is false, but also produce a result
 * (of value 0 and type size_t), so it can be used e.g. in a structure
 * initializer (or whereever else comma expressions aren't permitted). */
/* Linux calls these BUILD_BUG_ON_ZERO/_NULL, which is rather misleading. */
#define STATIC_ZERO_ASSERT(condition) ({ struct { int:-!(condition); } __z; sizeof(__z); })
#define STATIC_NULL_ASSERT(condition) ((void *)STATIC_ZERO_ASSERT(condition))

/* Force a compilation error if condition is false */
#define STATIC_ASSERT(condition) ((void)STATIC_ZERO_ASSERT(condition))

/* Force a compilation error if a constant expression is not a power of 2 */
#define STATIC_ASSERT_POWER_OF_2(n)          \
	STATIC_ASSERT(!((n) == 0 || (((n) & ((n) - 1)) != 0)))

/* Type and array checking */
#ifdef BOCHS
/* doesn't work in c++ :( */
#define SAME_TYPE(a, b) true
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#else
#define SAME_TYPE(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))
#define MUST_BE_ARRAY(a) STATIC_ZERO_ASSERT(!SAME_TYPE((a), &(a)[0]))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + MUST_BE_ARRAY(arr))
#endif

/* MIN()/MAX() implementations that avoid the MAX(x++,y++) problem and provide
 * strict typechecking. */
#ifdef BOCHS // XXX: simics's headers need this not to be defined here
#define MIN(x, y) ({			\
	typeof(x) _min1 = (x);		\
	typeof(y) _min2 = (y);		\
	(void) (&_min1 == &_min2);	\
	_min1 < _min2 ? _min1 : _min2; })

#define MAX(x, y) ({			\
	typeof(x) _max1 = (x);		\
	typeof(y) _max2 = (y);		\
	(void) (&_max1 == &_max2);	\
	_max1 > _max2 ? _max1 : _max2; })
#endif

#define container_of(ptr, type, member) ({          \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define ASSERT_UNSIGNED(addr) do {					\
		typeof(addr) __must_not_be_signed = (typeof(addr))(-1);	\
		assert(__must_not_be_signed > 0 &&			\
		       "type error: unsigned type required here");	\
	} while (0)

#endif /* !_INC_COMPILER_H */
