/**
 * @file x86.h
 * @brief bochs-specific machine manipulations
 * @author Ben Blum
 */

#ifndef __LS_X86_BOCHS_H
#define __LS_X86_BOCHS_H

#include "cpu/cpu.h"
#include "bochs.h"

#ifndef __LS_X86_h
#error "include x86.h, do not include this directly"
#endif

/* simics's register names are lowercase; this allows us to be consistent */
#define BX_32BIT_REG_eax BX_32BIT_REG_EAX
#define BX_32BIT_REG_ecx BX_32BIT_REG_ECX
#define BX_32BIT_REG_edx BX_32BIT_REG_EDX
#define BX_32BIT_REG_ebx BX_32BIT_REG_EBX
#define BX_32BIT_REG_esp BX_32BIT_REG_ESP
#define BX_32BIT_REG_ebp BX_32BIT_REG_EBP
#define BX_32BIT_REG_esi BX_32BIT_REG_ESI
#define BX_32BIT_REG_edi BX_32BIT_REG_EDI
#define BX_32BIT_REG_eip BX_32BIT_REG_EIP
/* bochs is too elite for us to need the segment selector registers */

/* more generalized SET_ATTR is only used in simics for apics and so forth */
#define GET_CPU_ATTR(cpu, name) ((cpu)->gen_reg[BX_32BIT_REG_##name].dword.erx)
#define SET_CPU_ATTR(cpu, name, val) do {				\
		(cpu)->gen_reg[BX_32BIT_REG_##name].dword.erx = (val);	\
	} while (0)

#define GET_CR0(cpu) ((unsigned int)((cpu)->read_CR0()))
#define GET_CR3(cpu) ((unsigned int)((cpu)->cr3))

#define READ_PHYS_MEMORY(cpu, addr, width) ({				\
	unsigned int __data;						\
	unsigned int __w = (width);					\
	assert((__w) <= 4 && "cant read so much at once");		\
	BX_MEM(0)->readPhysicalPage((cpu), (addr), __w, &__data);	\
	__data; })

#define WRITE_PHYS_MEMORY(cpu, addr, val, width) do {			\
		unsigned int __w = (width);				\
		unsigned int __v = (val);				\
		assert((__w) <= 4 && "cant write so much at once");	\
		BX_MEM(0)->writePhysicalPage((cpu), (addr), __w, &__v);	\
	} while (0)

#endif /* __LS_X86_H */
