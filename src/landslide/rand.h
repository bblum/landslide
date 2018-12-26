/**
 * @file rand.h
 * @brief Random number generation
 * FIXME: find a bsd licensed prng for this;
 * but it's not like it's used in any non deprecated features though so
 */

#ifndef __LS_RAND_H
#define __LS_RAND_H

struct rand_state {
};

void rand_init(struct rand_state *r);
uint32_t rand32(struct rand_state *r);
uint64_t rand64(struct rand_state *r);

#endif
