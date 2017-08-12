/**
 * @file x86.c
 * @brief x86-specific utilities
 * @author Ben Blum
 */

#define MODULE_NAME "X86"
#define MODULE_COLOUR COLOUR_DARK COLOUR_GREEN

#include "common.h"
#include "kernel_specifics.h"
#include "kspec.h"
#include "student_specifics.h"
#include "x86.h"

#ifdef BOCHS
#include "x86-bochs.c"
#else
#include "x86-simics.c"
#endif

static bool mem_translate(cpu_t *cpu, unsigned int addr, unsigned int *result)
{
#ifdef PINTOS_KERNEL
	/* In pintos the kernel is mapped at 3 GB, not direct-mapped. Luckily,
	 * paging is enabled in start(), while landslide enters at main(). */
	assert((GET_CR0(cpu) & CR0_PG) != 0 &&
	       "Expected Pintos to have paging enabled before landslide entrypoint.");
#else
	/* In pebbles the kernel is direct-mapped and paging may not be enabled
	 * until after landslide starts recording instructions. */
	if (KERNEL_MEMORY(addr)) {
		/* assume kern mem direct-mapped -- not strictly necessary */
		*result = addr;
		return true;
	} else if ((GET_CR0(cpu) & CR0_PG) == 0) {
		/* paging disabled; cannot translate user address */
		return false;
	}
#endif

	unsigned int upper = addr >> 22;
	unsigned int lower = (addr >> 12) & 1023;
	unsigned int offset = addr & 4095;
	unsigned int cr3 = GET_CR3(cpu);
	unsigned int pde_addr = cr3 + (4 * upper);
	unsigned int pde = READ_PHYS_MEMORY(cpu, pde_addr, WORD_SIZE);
	/* check present bit of pde to not anger the simics gods */
	if ((pde & 0x1) == 0) {
		return false;
#ifdef PDE_PTE_POISON
	} else if (pde == PDE_PTE_POISON) {
		return false;
#endif
	}
	unsigned int pte_addr = (pde & ~4095) + (4 * lower);
	unsigned int pte = READ_PHYS_MEMORY(cpu, pte_addr, WORD_SIZE);
	/* check present bit of pte to not anger the simics gods */
	if ((pte & 0x1) == 0) {
		return false;
#ifdef PDE_PTE_POISON
	} else if (pte == PDE_PTE_POISON) {
		return false;
#endif
	}
	*result = (pte & ~4095) + offset;
	return true;
}

unsigned int read_memory(cpu_t *cpu, unsigned int addr, unsigned int width)
{
	unsigned int phys_addr;
	if (mem_translate(cpu, addr, &phys_addr)) {
		return READ_PHYS_MEMORY(cpu, phys_addr, width);
	} else {
		return 0; /* :( */
	}
}

bool write_memory(cpu_t *cpu, unsigned int addr, unsigned int val, unsigned int width)
{
	unsigned int phys_addr;
	if (mem_translate(cpu, addr, &phys_addr)) {
		WRITE_PHYS_MEMORY(cpu, phys_addr, val, width);
		return true;
	} else {
		return false;
	}
}

char *read_string(cpu_t *cpu, unsigned int addr)
{
	unsigned int length = 0;

	while (READ_BYTE(cpu, addr + length) != 0) {
		length++;
	}

	char *buf = MM_XMALLOC(length + 1, char);

	for (unsigned int i = 0; i <= length; i++) {
		buf[i] = READ_BYTE(cpu, addr + i);
	}

	return buf;
}

/* will read at most 3 opcodes */
bool opcodes_are_atomic_swap(uint8_t *ops) {
	unsigned int offset = 0;
	if (ops[offset] == 0xf0) {
		/* lock prefix */
		offset++;
	}

	if (ops[offset] == 0x86 || ops[offset] == 0x87) {
		/* xchg */
		return true;
	} else if (ops[offset] == 0x0f) {
		offset++;
		if (ops[offset] == 0xb0 || ops[offset] == 0xb1) {
			/* cmpxchg */
			return true;
		} else {
			// FIXME: Shouldn't 0F C0 and 0F C1 (xadd) be here?
			return false;
		}
	} else {
		return false;
	}
}

bool instruction_is_atomic_swap(cpu_t *cpu, unsigned int eip) {
	uint8_t opcodes[3];
	opcodes[0] = READ_BYTE(cpu, eip);
	opcodes[1] = READ_BYTE(cpu, eip + 1);
	opcodes[2] = READ_BYTE(cpu, eip + 2);
	return opcodes_are_atomic_swap(opcodes);
}

unsigned int cause_transaction_failure(cpu_t *cpu, unsigned int status)
{
#ifdef HTM
	/* it'd work in principle but explore/sched shouldn't use it this way */
	assert(status != _XBEGIN_STARTED && "i don't swing like that");
	SET_CPU_ATTR(cpu, eax, status);
	/* because of the 1-instruction delay on timer interrupce after a PP,
	 * we'll be injecting the failure after ebp is pushed in _xbegin. */
	assert(GET_CPU_ATTR(cpu, eip) == HTM_XBEGIN + 1);
	SET_CPU_ATTR(cpu, eip, HTM_XBEGIN_END - 1);
	return HTM_XBEGIN_END - 1;
#else
	assert(0 && "how did this get here? i am not good with HTM");
	return 0;
#endif
}
