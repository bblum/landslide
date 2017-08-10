/**
 * @file instrument.cc
 * @author Ben Blum
 * @author Stanislav Shwartsman [sshwarts at sourceforge net]
 * @brief hooks into bochs for landslide
 */

#include <assert.h>

#include "bochs.h"
#include "iodev/iodev.h"

#include "instrument.h"
#include "x86.h"

#if BX_INSTRUMENTATION

/* stuff to go in x86.c eventually */

#define DEVWRAP_TIMER_START 0x10181e
#define DEVWRAP_TIMER_END   0x101866

void cause_timer_interrupt()
{
	DEV_pic_lower_irq(0);
	DEV_pic_raise_irq(0);
	assert(BX_CPU(0)->async_event);
}

void cause_timer_interrupt_immediately()
{
	DEV_pic_lower_irq(0);
	DEV_pic_raise_irq(0);
	assert(BX_CPU(0)->async_event);
	assert(!BX_CPU(0)->interrupts_inhibited(BX_INHIBIT_INTERRUPTS));
	assert(BX_CPU(0)->is_unmasked_event_pending(BX_EVENT_PENDING_INTR));
	bool rv = BX_CPU(0)->handleAsyncEvent(); /* modifies eip */
	assert(!rv); /* not need break out of cpu loop */
	assert(!BX_CPU(0)->async_event);
	assert(GET_REG(EIP) == DEVWRAP_TIMER_START);
}

void avoid_timer_interrupt_immediately()
{
	BX_OUTP(0x20, 0x20, 1);
	SET_REG(EIP, DEVWRAP_TIMER_END);
}

static void do_scan(int key_event, bool shift)
{
	if (shift) DEV_kbd_gen_scancode(BX_KEY_SHIFT_L);
	DEV_kbd_gen_scancode(key_event);
	DEV_kbd_gen_scancode(key_event | BX_KEY_RELEASED);
	if (shift) DEV_kbd_gen_scancode(BX_KEY_SHIFT_L | BX_KEY_RELEASED);
}

void cause_keypress(char key)
{
	switch (key) {
		case '\n': do_scan(BX_KEY_KP_ENTER, 0); break;
		case '_': do_scan(BX_KEY_MINUS, 1); break;
		case ' ': do_scan(BX_KEY_SPACE, 0); break;
		case 'a': do_scan(BX_KEY_A, 0); break;
		case 'b': do_scan(BX_KEY_B, 0); break;
		case 'c': do_scan(BX_KEY_C, 0); break;
		case 'd': do_scan(BX_KEY_D, 0); break;
		case 'e': do_scan(BX_KEY_E, 0); break;
		case 'f': do_scan(BX_KEY_F, 0); break;
		case 'g': do_scan(BX_KEY_G, 0); break;
		case 'h': do_scan(BX_KEY_H, 0); break;
		case 'i': do_scan(BX_KEY_I, 0); break;
		case 'j': do_scan(BX_KEY_J, 0); break;
		case 'k': do_scan(BX_KEY_K, 0); break;
		case 'l': do_scan(BX_KEY_L, 0); break;
		case 'm': do_scan(BX_KEY_M, 0); break;
		case 'n': do_scan(BX_KEY_N, 0); break;
		case 'o': do_scan(BX_KEY_O, 0); break;
		case 'p': do_scan(BX_KEY_P, 0); break;
		case 'q': do_scan(BX_KEY_Q, 0); break;
		case 'r': do_scan(BX_KEY_R, 0); break;
		case 's': do_scan(BX_KEY_S, 0); break;
		case 't': do_scan(BX_KEY_T, 0); break;
		case 'u': do_scan(BX_KEY_U, 0); break;
		case 'v': do_scan(BX_KEY_V, 0); break;
		case 'w': do_scan(BX_KEY_W, 0); break;
		case 'x': do_scan(BX_KEY_X, 0); break;
		case 'y': do_scan(BX_KEY_Y, 0); break;
		case 'z': do_scan(BX_KEY_Z, 0); break;
		case '0': do_scan(BX_KEY_0, 0); break;
		case '1': do_scan(BX_KEY_1, 0); break;
		case '2': do_scan(BX_KEY_2, 0); break;
		case '3': do_scan(BX_KEY_3, 0); break;
		case '4': do_scan(BX_KEY_4, 0); break;
		case '5': do_scan(BX_KEY_5, 0); break;
		case '6': do_scan(BX_KEY_6, 0); break;
		case '7': do_scan(BX_KEY_7, 0); break;
		case '8': do_scan(BX_KEY_8, 0); break;
		case '9': do_scan(BX_KEY_9, 0); break;
		default: assert(0 && "keypress not implemented");
	}
}

void cause_test(const char *test_name)
{
	for (const char *c = &test_name[0]; *c != '\0'; c++) {
		cause_keypress(*c);
	}
	cause_keypress('\n');
}

/******************************** useful hooks ********************************/

#define MODULE "\033[01;31m[baby landslide]\033[00m "

void bx_instr_initialize(unsigned cpu)
{
	// TODO: initialize landslide struct and all
	assert(cpu == 0 && "smp landslide not supported");
}

static unsigned int timer_ret_eip = 0;
static bool entering_timer = false;
static bool allow_readline = false;

void bx_instr_before_execution(unsigned cpu, bxInstruction_c *i)
{
	unsigned int eip = GET_REG(EIP);
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
		printf(MODULE "entering devwrap timer... squelching it\n");
		timer_ret_eip = READ_WORD(GET_REG(ESP));
		avoid_timer_interrupt_immediately();
	} else if (eip == 0x107824 /* sys_exec, just for testimg */) {
		printf(MODULE "testing timer injection on sys_exec()\n");
		cause_timer_interrupt();
		entering_timer = true;
	} else if (eip == 0x01001f00 /* shell readline wrapper */) {
		if (allow_readline) {
			allow_readline = false;
			printf(MODULE "immediate timer: allowing readline proceed\n");
		} else {
			allow_readline = true;
			printf(MODULE "immediate timer injection on readline()\n");
			cause_timer_interrupt_immediately();
		}
	} else if (eip == 0x0010687e /* sys_readline */) {
		printf(MODULE "typing the test name atm\n");
		cause_test("vanish_vanish");
	} else if (eip == 0x00101867) {
		printf(MODULE "a wild keyboard interrupt appears\n");
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
