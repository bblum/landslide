/**
 * @file x86-simics.c
 * @brief x86-specific utilities
 * @author Ben Blum
 */

#ifndef MODULE_NAME
#error "this file cannot be compiled directly; see x86.c"
#endif

#include "iodev/iodev.h"

unsigned int cause_timer_interrupt_immediately(cpu_t *cpu)
{
	// TODO: replace with kern get timer wrpa begin
	unsigned int handler = 0x10181e;
	DEV_pic_lower_irq(0);
	DEV_pic_raise_irq(0);
	assert(cpu->async_event);
	assert(!cpu->interrupts_inhibited(BX_INHIBIT_INTERRUPTS));
	assert(cpu->is_unmasked_event_pending(BX_EVENT_PENDING_INTR));
	bool rv = cpu->handleAsyncEvent(); /* modifies eip */
	assert(!rv); /* not need break out of cpu loop */
	assert(!cpu->async_event);
	assert(GET_CPU_ATTR(cpu, eip) == handler);
	return handler;
}

void cause_timer_interrupt(cpu_t *cpu, apic_t *apic, pic_t *pic)
{
	assert(apic == NULL && "not needed");
	assert(pic  == NULL && "not needed");
	DEV_pic_lower_irq(0);
	DEV_pic_raise_irq(0);
	assert(cpu->async_event);
}

unsigned int avoid_timer_interrupt_immediately(cpu_t *cpu)
{
	BX_OUTP(0x20, 0x20, 1);
	// TODO: replace with kerne get timer wrap end
	SET_CPU_ATTR(cpu, eip, 0x101866);
}

static void do_scan(int key_event, bool shift)
{
	if (shift) DEV_kbd_gen_scancode(BX_KEY_SHIFT_L);
	DEV_kbd_gen_scancode(key_event);
	DEV_kbd_gen_scancode(key_event | BX_KEY_RELEASED);
	if (shift) DEV_kbd_gen_scancode(BX_KEY_SHIFT_L | BX_KEY_RELEASED);
}

void cause_keypress(keyboard_t *kbd, char key)
{
	assert(kbd == NULL && "not needed");
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

bool interrupts_enabled(cpu_t *cpu)
{
	assert(0 && "unimplemented");
	return false;
}

unsigned int delay_instruction(cpu_t *cpu)
{
	assert(0 && "unimplemented");
}
