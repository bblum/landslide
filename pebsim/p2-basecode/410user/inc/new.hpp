/** @file new.h
 *  @brief defines C++ allocators backed by 410 userland malloc
 */

#ifndef __NEW_HPP
#define __NEW_HPP

extern "C" {
#include <malloc.h>
#include <assert.h>
}

void* operator new (size_t size) { return calloc(1, size); }

void operator delete(void *p, unsigned int) { free(p); }

void* operator new[](unsigned int size) { return calloc(1, size); }
void operator delete[](void*p) { free(p); }

void __cxa_throw_bad_array_new_length() { assert(0 && "bad array length"); }

#endif /* __NEW_HPP */
