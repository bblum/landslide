/** @file new.h
 *  @brief defines C++ allocators backed by 410 userland malloc
 */

#ifndef __NEW_HPP
#define __NEW_HPP

extern "C" {
#include <malloc.h>
}

void* operator new (size_t size) { return malloc(size); }

void operator delete(void *p, unsigned int) { free(p); }

#endif /* __NEW_HPP */
