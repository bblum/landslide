/**
 * @file instrument.cc
 * @author Ben Blum
 * @author Stanislav Shwartsman [sshwarts at sourceforge net]
 * @brief hooks into bochs for landslide
 */

#include <assert.h>

#include "bochs.h"

#define MODULE_NAME "baby landslide"
#define MODULE_COLOUR COLOUR_BOLD COLOUR_WHITE

#include "common.h"
#include "landslide.h"
#include "instrument.h"
#include "x86.h"

#if BX_INSTRUMENTATION

/* stuff to go in x86.c eventually */

void landslide_assert_fail(char const*, char const*, unsigned int, char const*)
{
	assert(0);
}

void cause_test(const char *test_name)
{
	for (const char *c = &test_name[0]; *c != '\0'; c++) {
		cause_keypress(NULL, *c);
	}
	cause_keypress(NULL, '\n');
}

/******************************** useful hooks ********************************/

void bx_instr_initialize(unsigned cpu)
{
	// TODO: initialize landslide struct and all
	assert(cpu == 0 && "smp landslide not supported");
}

static unsigned int timer_ret_eip = 0;
static bool entering_timer = false;
static bool allow_readline = false;

#define DEVWRAP_TIMER_START 0x10181e

void bx_instr_before_execution(unsigned cpu, bxInstruction_c *i)
{
	unsigned int eip = GET_CPU_ATTR(BX_CPU(0), eip);
	if (eip < 0x100000) {
		return;
	} else if (timer_ret_eip != 0) {
		assert(eip == timer_ret_eip && "failed suppress bochs's timer");
		timer_ret_eip = 0;
	} else if (entering_timer) {
		assert(eip == DEVWRAP_TIMER_START && "bochs missed our timer");
		entering_timer = false;
	} else if (eip == DEVWRAP_TIMER_START) {
		// TODO: future optmz - hack bochs to tick less frequently
		lsprintf(ALWAYS, "entering devwrap timer... squelching it\n");
		timer_ret_eip = READ_PHYS_MEMORY(BX_CPU(0),
						 GET_CPU_ATTR(BX_CPU(0), esp), 4);
		avoid_timer_interrupt_immediately(BX_CPU(0));
	} else if (eip == 0x107824 /* sys_exec, just for testimg */) {
		lsprintf(ALWAYS, "testing timer injection on sys_exec()\n");
		cause_timer_interrupt(BX_CPU(0), NULL, NULL);
		entering_timer = true;
	} else if (eip == 0x01001f00 /* shell readline wrapper */) {
		if (allow_readline) {
			allow_readline = false;
			lsprintf(ALWAYS, "immediate timer: allowing readline proceed\n");
		} else {
			allow_readline = true;
			lsprintf(ALWAYS, "immediate timer injection on readline()\n");
			cause_timer_interrupt_immediately(BX_CPU(0));
		}
	} else if (eip == 0x0010687e /* sys_readline */) {
		lsprintf(ALWAYS, "typing the test name atm\n");
		cause_test("vanish_vanish");
	} else if (eip == 0x00101867) {
		lsprintf(ALWAYS, "a wild keyboard interrupt appears\n");
	} else if (eip == 0x105930) {
		lsprintf(ALWAYS, "switched threads -> %d\n",
		       READ_MEMORY(BX_CPU(0), GET_CPU_ATTR(BX_CPU(0), esp)));
	}
}

void bx_instr_after_execution(unsigned cpu, bxInstruction_c *i)
{
}

void bx_instr_lin_access(unsigned cpu, bx_address lin, bx_address phy, unsigned len, unsigned memtype, unsigned rw) {}

/******************************** unused hooks ********************************/

void bx_instr_init_env(void) {}
void bx_instr_exit_env(void) {}

void bx_instr_exit(unsigned cpu) {}
void bx_instr_reset(unsigned cpu, unsigned type) {}
void bx_instr_hlt(unsigned cpu) {}
void bx_instr_mwait(unsigned cpu, bx_phy_address addr, unsigned len, Bit32u flags) {}

void bx_instr_debug_promt() {}
void bx_instr_debug_cmd(const char *cmd) {}

void bx_instr_cnear_branch_taken(unsigned cpu, bx_address branch_eip, bx_address new_eip) {}
void bx_instr_cnear_branch_not_taken(unsigned cpu, bx_address branch_eip) {}
void bx_instr_ucnear_branch(unsigned cpu, unsigned what, bx_address branch_eip, bx_address new_eip) {}
void bx_instr_far_branch(unsigned cpu, unsigned what, Bit16u prev_cs, bx_address prev_eip, Bit16u new_cs, bx_address new_eip) {}

void bx_instr_opcode(unsigned cpu, bxInstruction_c *i, const Bit8u *opcode, unsigned len, bx_bool is32, bx_bool is64) {}

// TODO: which interrupce do i care aboub
void bx_instr_interrupt(unsigned cpu, unsigned vector) {}
void bx_instr_exception(unsigned cpu, unsigned vector, unsigned error_code) {}
void bx_instr_hwinterrupt(unsigned cpu, unsigned vector, Bit16u cs, bx_address eip) {}

void bx_instr_tlb_cntrl(unsigned cpu, unsigned what, bx_phy_address new_cr3) {}
void bx_instr_clflush(unsigned cpu, bx_address laddr, bx_phy_address paddr) {}
void bx_instr_cache_cntrl(unsigned cpu, unsigned what) {}
void bx_instr_prefetch_hint(unsigned cpu, unsigned what, unsigned seg, bx_address offset) {}

void bx_instr_repeat_iteration(unsigned cpu, bxInstruction_c *i) {}

void bx_instr_inp(Bit16u addr, unsigned len) {}
void bx_instr_inp2(Bit16u addr, unsigned len, unsigned val) {}
void bx_instr_outp(Bit16u addr, unsigned len, unsigned val) {}

void bx_instr_phy_access(unsigned cpu, bx_address phy, unsigned len, unsigned memtype, unsigned rw) {}

void bx_instr_wrmsr(unsigned cpu, unsigned addr, Bit64u value) {}

void bx_instr_vmexit(unsigned cpu, Bit32u reason, Bit64u qualification) {}

#endif
