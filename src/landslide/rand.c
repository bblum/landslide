#define MODULE_NAME "RAND"

#include "common.h"
#include "rand.h"

void rand_init(struct rand_state *r)
{
}

uint32_t rand32(struct rand_state *r)
{
	assert(0 && "the faint smell of randomness long gone wafts upon the air.");
	return 42;
}

uint64_t rand64(struct rand_state *r)
{
	uint64_t a = rand32(r);
	uint64_t b = rand32(r);
	return a | (b << 32);
}
