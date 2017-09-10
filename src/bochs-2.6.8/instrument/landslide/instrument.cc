/**
 * @file instrument.cc
 * @author Ben Blum
 * @author Stanislav Shwartsman [sshwarts at sourceforge net]
 * @brief hooks into bochs for landslide
 */

#include <stdlib.h>
#include <sys/time.h>

#define MODULE_NAME "bochs glue"
#define MODULE_COLOUR COLOUR_BOLD COLOUR_WHITE

#include "common.h"
#include "landslide.h"
#include "instrument.h"
#include "simulator.h"

#include "cpu/instr.h"

#ifndef BX_INSTRUMENTATION
#error "what even?"
#endif

/******************************** useful hooks ********************************/

void bx_instr_initialize(unsigned cpu)
{
	assert(cpu == 0 && "smp landslide not supported");
	struct ls_state *ls = new_landslide();
	assert(ls == GET_LANDSLIDE());

	struct timeval tv;
	gettimeofday(&tv, NULL);
	char buf[BUF_SIZE];
	scnprintf(buf, BUF_SIZE, "landslide-trace-%lu.%lu.html", tv.tv_sec, tv.tv_usec);
	ls->html_file = MM_XSTRDUP(buf);

	char *quicksand_pps = getenv("QUICKSAND_CONFIG_TEMP");
	if (quicksand_pps != NULL) {
		bool pps_loaded = load_dynamic_pps(ls, quicksand_pps);
		assert(pps_loaded && "somehow failed to grok quicksands pps");
	}
}

void bx_instr_before_execution(unsigned cpu, bxInstruction_c *i)
{
	struct trace_entry entry;
	entry.type = TRACE_INSTRUCTION;
	entry.instruction_text = i->opcode_bytes;
	landslide_entrypoint(GET_LANDSLIDE(), &entry);
}

void bx_instr_lin_access(unsigned cpu, bx_address lin, bx_address phy, unsigned len, unsigned memtype, unsigned rw)
{
	struct trace_entry entry;
	entry.type = TRACE_MEMORY;
	entry.va = lin;
	entry.pa = phy;
	entry.write = rw == BX_WRITE || rw == BX_RW;
	landslide_entrypoint(GET_LANDSLIDE(), &entry);
}

void bx_instr_interrupt(unsigned cpu, unsigned vector)
{
	struct trace_entry entry;
	entry.type = TRACE_EXCEPTION;
	entry.exn_number = (int)vector;
	landslide_entrypoint(GET_LANDSLIDE(), &entry);
}

void bx_instr_exception(unsigned cpu, unsigned vector, unsigned error_code)
{
	bx_instr_interrupt(cpu, vector);
}

void bx_instr_hwinterrupt(unsigned cpu, unsigned vector, Bit16u cs, bx_address eip)
{
	bx_instr_interrupt(cpu, vector);
}

extern bool ls_safe_exit;
void bx_instr_exit_env(void)
{
	assert(ls_safe_exit && "ono, bochs panicked on us!");
}

/******************************** unused hooks ********************************/

void bx_instr_init_env(void) {}

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

void bx_instr_tlb_cntrl(unsigned cpu, unsigned what, bx_phy_address new_cr3) {}
void bx_instr_clflush(unsigned cpu, bx_address laddr, bx_phy_address paddr) {}
void bx_instr_cache_cntrl(unsigned cpu, unsigned what) {}
void bx_instr_prefetch_hint(unsigned cpu, unsigned what, unsigned seg, bx_address offset) {}

void bx_instr_repeat_iteration(unsigned cpu, bxInstruction_c *i) {}
void bx_instr_after_execution(unsigned cpu, bxInstruction_c *i) {}

void bx_instr_inp(Bit16u addr, unsigned len) {}
void bx_instr_inp2(Bit16u addr, unsigned len, unsigned val) {}
void bx_instr_outp(Bit16u addr, unsigned len, unsigned val) {}

void bx_instr_phy_access(unsigned cpu, bx_address phy, unsigned len, unsigned memtype, unsigned rw) {}

void bx_instr_wrmsr(unsigned cpu, unsigned addr, Bit64u value) {}

void bx_instr_vmexit(unsigned cpu, Bit32u reason, Bit64u qualification) {}
