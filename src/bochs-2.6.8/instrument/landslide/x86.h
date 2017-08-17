/**
 * @file x86.h
 * @brief x86-specific utilities
 * @author Ben Blum
 */

#ifndef __LS_X86_H
#define __LS_X86_H

#include <stdint.h>

#include "compiler.h"
#include "simulator.h"

#ifdef BOCHS

#include "cpu/cpu.h"
#include "bochs.h"

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
#define GET_CR2(cpu) ((unsigned int)((cpu)->cr2))
#define GET_CR3(cpu) ((unsigned int)((cpu)->cr3))

#define CPL_USER(cpu) ((cpu)->user_pl)

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

#else /* SIMICS */
#include "x86-simics.h"
#endif

#define WORD_SIZE 4
#define PAGE_SIZE 4096
#define PAGE_ALIGN(x) ((x) & ~(PAGE_SIZE-1))

#define CR0_PG (1 << 31)
#define TIMER_INTERRUPT_NUMBER 0x20
#define INT_CTL_PORT 0x20 /* MASTER_ICW == ADDR_PIC_BASE + OFF_ICW */
#define INT_ACK_CURRENT 0x20 /* NON_SPEC_EOI */
#define EFL_IF          0x00000200 /* from 410kern/inc/x86/eflags.h */
#define OPCODE_PUSH_EBP 0x55
#define OPCODE_RET  0xc3
#define OPCODE_IRET 0xcf
#define OPCODE_CALL 0xe8
#define IRET_BLOCK_WORDS 3
#define OPCODE_HLT 0xf4
#define OPCODE_INT 0xcd
#define OPCODE_IS_POP_GPR(o) ((o) >= 0x58 && (o) < 0x60)
#define OPCODE_POPA 0x61
#define POPA_WORDS 8
#define OPCODE_INT_ARG(cpu, eip) READ_BYTE(cpu, eip + 1)
#define OPCODE_MOV_R32_M32 0x8b
#define OPCODE_CLI 0xfa
#define OPCODE_STI 0xfb

#define _XBEGIN_STARTED    (~0u)
#define _XABORT_EXPLICIT   (1 << 0)
#define _XABORT_RETRY      (1 << 1)
#define _XABORT_CONFLICT   (1 << 2)
#define _XABORT_CAPACITY   (1 << 3)
#define _XABORT_DEBUG      (1 << 4)
#define _XABORT_NESTED     (1 << 5)
#define _XABORT_CODE(x)    (((x) >> 24) & 0xFF)

void cause_timer_interrupt(cpu_t *cpu, apic_t *apic, pic_t *pic);
unsigned int cause_timer_interrupt_immediately(cpu_t *cpu);
unsigned int avoid_timer_interrupt_immediately(cpu_t *cpu);
void cause_keypress(keyboard_t *kbd, char);
bool interrupts_enabled(cpu_t *cpu);
unsigned int read_memory(cpu_t *cpu, unsigned int addr, unsigned int width);
bool write_memory(cpu_t *cpu, unsigned int addr, unsigned int val, unsigned int width);
char *read_string(cpu_t *cpu, unsigned int eip);
bool instruction_is_atomic_swap(cpu_t *cpu, unsigned int eip); /* slower; uses READ_MEMORY */
bool opcodes_are_atomic_swap(uint8_t *opcodes); /* faster; ok to use every instruction */
unsigned int delay_instruction(cpu_t *cpu);
unsigned int cause_transaction_failure(cpu_t *cpu, unsigned int status);

#define READ_BYTE(cpu, addr) \
	({ ASSERT_UNSIGNED(addr); read_memory(cpu, addr, 1); })
#define READ_MEMORY(cpu, addr) \
	({ ASSERT_UNSIGNED(addr); read_memory(cpu, addr, WORD_SIZE); })

/* reading the stack. can be used to examine function arguments, if used either
 * at the very end or the very beginning of a function, when esp points to the
 * return address. */
#define READ_STACK(cpu, offset) \
	READ_MEMORY(cpu, GET_CPU_ATTR(cpu, esp) + ((offset) * WORD_SIZE))

/* in simics we need a special call to flush the icache;
 * whereas we hacked bochs to do it on its own in cpu/cpu.cc */
#define SET_EIP_FLUSH_ICACHE(cpu, buf) SET_CPU_ATTR((cpu), eip, (buf))

#endif /* __LS_X86_h */
