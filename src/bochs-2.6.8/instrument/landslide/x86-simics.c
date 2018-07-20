/**
 * @file x86-simics.c
 * @brief x86-specific utilities
 * @author Ben Blum
 */

#ifndef MODULE_NAME
#error "this file cannot be compiled directly; see x86.c"
#endif

#define INT SIM_make_attr_integer

/* two possible methods for causing a timer interrupt - the "immediately"
 * version makes the simulation immediately jump to some assembly on the stack
 * that directly invokes the timer interrupt INSTEAD of executing the pending
 * instruction; the other way just manipulates the cpu's interrupt pending
 * flags to make it do the interrupt itself. */
bool cause_timer_interrupt_immediately(conf_object_t *cpu, unsigned int *new_eip)
{
	unsigned int esp = GET_CPU_ATTR(cpu, esp);
	unsigned int eip = GET_CPU_ATTR(cpu, eip);
	unsigned int eflags = GET_CPU_ATTR(cpu, eflags);
	unsigned int handler = kern_get_timer_wrap_begin();

	if (KERNEL_MEMORY(eip)) {
		/* Easy mode. Just make a small iret stack frame. */
		assert(GET_SEGSEL(cpu, cs) == SEGSEL_KERNEL_CS);
		assert(GET_SEGSEL(cpu, ss) == SEGSEL_KERNEL_DS);

		lsprintf(DEV, "tock! (0x%x)\n", eip);

		/* 12 is the size of an IRET frame only when already in kernel mode. */
		unsigned int new_esp = esp - 12;
		SET_CPU_ATTR(cpu, esp, new_esp);
		write_memory(cpu, new_esp + 8, eflags, 4);
		write_memory(cpu, new_esp + 4, SEGSEL_KERNEL_CS, 4);
		write_memory(cpu, new_esp + 0, eip, 4);
	} else {
		/* Hard mode - do a mode switch also. Grab esp0, make a large
		 * iret frame, and change the segsel registers to kernel mode. */
		assert(GET_SEGSEL(cpu, cs) == SEGSEL_USER_CS);
		assert(GET_SEGSEL(cpu, ss) == SEGSEL_USER_DS);

		lsprintf(DEV, "tock! from userspace! (0x%x)\n", eip);

		unsigned int esp0 =
#ifdef PINTOS_KERNEL
			0;
		// FIXME
		assert(false && "TSS/esp0 not implemented for Pintos.");
#else
			READ_MEMORY(cpu, (unsigned)GUEST_ESP0_ADDR);
#endif
		/* 20 is the size of an IRET frame coming from userland. */
		unsigned int new_esp = esp0 - 20;
		SET_CPU_ATTR(cpu, esp, new_esp);
		write_memory(cpu, new_esp + 16, SEGSEL_USER_DS, 4);
		write_memory(cpu, new_esp + 12, esp, 4);
		write_memory(cpu, new_esp +  8, eflags, 4);
		write_memory(cpu, new_esp +  4, SEGSEL_USER_CS, 4);
		write_memory(cpu, new_esp +  0, eip, 4);

		/* Change %cs and %ss. (Other segsels should be saved/restored
		 * in the kernel's handler wrappers.) */
		attr_value_t cs = SIM_make_attr_list(10, INT(SEGSEL_KERNEL_CS),
			INT(1), INT(0), INT(1), INT(1), INT(1),
			INT(11), INT(0), INT(4294967295L), INT(1));
		set_error_t ret = SIM_set_attribute(cpu, "cs", &cs);
		assert(ret == Sim_Set_Ok && "failed set cs");
		SIM_free_attribute(cs);
		attr_value_t ss = SIM_make_attr_list(10, INT(SEGSEL_KERNEL_DS),
			INT(1), INT(3), INT(1), INT(1), INT(1),
			INT(3), INT(0), INT(4294967295L), INT(1));
		ret = SIM_set_attribute(cpu, "ss", &ss);
		assert(ret == Sim_Set_Ok && "failed set ss");
		SIM_free_attribute(ss);

		/* Change CPL. */
		assert(GET_CPU_ATTR(cpu, cpl) == 3);
		SET_CPU_ATTR(cpu, cpl, 0);

	}
	SET_CPU_ATTR(cpu, eip, handler);
#ifdef PINTOS_KERNEL
	SET_CPU_ATTR(cpu, eflags, GET_CPU_ATTR(cpu, eflags) & ~EFL_IF);
#endif
	*new_eip = handler;
	return true;
}

/* i.e., with stallin' */
static void cause_timer_interrupt_soviet_style(conf_object_t *cpu, lang_void *x)
{
	SIM_stall_cycle(cpu, 0);
}

void cause_timer_interrupt(conf_object_t *cpu, conf_object_t *apic, conf_object_t *pic)
{
	lsprintf(DEV, "tick! (0x%x)\n", GET_CPU_ATTR(cpu, eip));

	assert(GET_CPU_ATTR(cpu, waiting_interrupt) == 0);
	SET_CPU_ATTR(cpu, waiting_interrupt, 1);
	SET_ATTR(cpu, waiting_device, object, apic);

	/* hello simics 4.6, nice to meet you too */
	SET_ATTR(apic, interrupt_posted, integer, 1);
	SET_ATTR(apic, ext_int_obj, object, pic);
	attr_value_t s = SIM_get_attribute(apic, "status");
	SIM_attr_list_set_item(&s, 0, SIM_make_attr_list(3, INT(0), INT(1), INT(0)));
	set_error_t ret = SIM_set_attribute(apic, "status", &s);
	assert(ret == Sim_Set_Ok && "failed set apic status");
	SIM_free_attribute(s);

	SET_ATTR(pic, irq_requested, integer, 1);
	attr_value_t r = SIM_make_attr_list(2, INT(1), INT(0));
	ret = SIM_set_attribute(pic, "request", &r);
	assert(ret == Sim_Set_Ok && "failed set pic status");
	SIM_free_attribute(r);

	/* Causes simics to flush whatever pipeline, implicit or not, would
	 * otherwise let more instructions get executed before the interrupt be
	 * taken. */
	SIM_run_unrestricted(cpu, cause_timer_interrupt_soviet_style, NULL);
}

/* Will use 8 bytes of stack when it runs. */
#define CUSTOM_ASSEMBLY_CODES_STACK 8
static const char custom_assembly_codes[] = {
	0x50, /* push %eax */
	0x52, /* push %edx */
	0x66, 0xba, 0x20, 0x00, /* mov $0x20, %dx # INT_ACK_CURRENT */
	0xb0, 0x20, /* mov $0x20, %al # INT_CTL_PORT */
	0xee, /* out %al, (%dx) */
	0x5a, /* pop %edx */
	0x58, /* pop %eax */
	0xcf, /* iret */
};

unsigned int avoid_timer_interrupt_immediately(conf_object_t *cpu)
{
	// XXX: This mechanism is vulnerable to the twilight zone bug that's
	// fixed in delay_instruction; as well as the stack-clobber bug from
	// issue #201. It's just 10000x less likely to trigger because of how
	// infrequently simics timer ticks happen.
	unsigned int buf = GET_CPU_ATTR(cpu, esp) -
		(ARRAY_SIZE(custom_assembly_codes) + CUSTOM_ASSEMBLY_CODES_STACK);

	lsprintf(INFO, "Cuckoo!\n");

	STATIC_ASSERT(ARRAY_SIZE(custom_assembly_codes) % 4 == 0);
	for (int i = 0; i < ARRAY_SIZE(custom_assembly_codes); i++) {
		write_memory(cpu, buf + i, custom_assembly_codes[i], 1);
	}

	SET_CPU_ATTR(cpu, eip, buf);
	return buf;
}

/* keycodes for the keyboard buffer */
static int i8042_key(char c)
{
	static const int i8042_keys[] = {
		['0'] = 18, ['1'] = 19, ['2'] = 20, ['3'] = 21, ['4'] = 22,
		['5'] = 23, ['6'] = 24, ['7'] = 25, ['8'] = 26, ['9'] = 27,
		['a'] = 28, ['b'] = 29, ['c'] = 30, ['d'] = 31, ['e'] = 32,
		['f'] = 33, ['g'] = 34, ['h'] = 35, ['i'] = 36, ['j'] = 37,
		['k'] = 38, ['l'] = 39, ['m'] = 40, ['n'] = 41, ['o'] = 42,
		['p'] = 43, ['q'] = 44, ['r'] = 45, ['s'] = 46, ['t'] = 47,
		['u'] = 48, ['v'] = 49, ['w'] = 50, ['x'] = 51, ['y'] = 52,
		['z'] = 53, ['\''] = 54, [','] = 55, ['.'] = 56, [';'] = 57,
		['='] = 58, ['/'] = 59, ['\\'] = 60, [' '] = 61, ['['] = 62,
		[']'] = 63, ['-'] = 64, ['`'] = 65,             ['\n'] = 67,
	};
	assert(i8042_keys[(int)c] != 0 && "Attempt to type an unsupported key");
	return i8042_keys[(int)c];
}

static bool i8042_shift_key(char *c)
{
	static const char i8042_shift_keys[] = {
		['~'] = '`', ['!'] = '1', ['@'] = '2', ['#'] = '3', ['$'] = '4',
		['%'] = '5', ['^'] = '6', ['&'] = '7', ['*'] = '8', ['('] = '9',
		[')'] = '0', ['_'] = '-', ['+'] = '=', ['Q'] = 'q', ['W'] = 'w',
		['E'] = 'e', ['R'] = 'r', ['T'] = 't', ['Y'] = 'y', ['U'] = 'u',
		['I'] = 'i', ['O'] = 'o', ['P'] = 'p', ['{'] = '[', ['}'] = ']',
		['A'] = 'a', ['S'] = 's', ['D'] = 'd', ['D'] = 'd', ['F'] = 'f',
		['G'] = 'g', ['H'] = 'h', ['J'] = 'j', ['K'] = 'k', ['L'] = 'l',
		[':'] = ';', ['"'] = '\'', ['Z'] = 'z', ['X'] = 'x',
		['C'] = 'c', ['V'] = 'v', ['B'] = 'b', ['N'] = 'n', ['M'] = 'm',
		['<'] = ',', ['>'] = '.', ['?'] = '/',
	};

	if (i8042_shift_keys[(int)*c] != 0) {
		*c = i8042_shift_keys[(int)*c];
		return true;
	}
	return false;
}

void cause_keypress(conf_object_t *kbd, char key)
{
	bool do_shift = i8042_shift_key(&key);

	unsigned int keycode = i8042_key(key);

	attr_value_t i = SIM_make_attr_integer(keycode);
	attr_value_t v = SIM_make_attr_integer(0); /* see i8042 docs */
	/* keycode value for shift found by trial and error :< */
	attr_value_t shift = SIM_make_attr_integer(72);

	set_error_t ret;

	/* press key */
	if (do_shift) {
		ret = SIM_set_attribute_idx(kbd, "key_event", &shift, &v);
		assert(ret == Sim_Set_Ok && "shift press failed!");
	}

	ret = SIM_set_attribute_idx(kbd, "key_event", &i, &v);
	assert(ret == Sim_Set_Ok && "cause_keypress press failed!");

	/* release key */
	v = SIM_make_attr_integer(1);
	ret = SIM_set_attribute_idx(kbd, "key_event", &i, &v);
	assert(ret == Sim_Set_Ok && "cause_keypress release failed!");

	if (do_shift) {
		ret = SIM_set_attribute_idx(kbd, "key_event", &shift, &v);
		assert(ret == Sim_Set_Ok && "cause_keypress release failed!");
	}
}

bool interrupts_enabled(conf_object_t *cpu)
{
	unsigned int eflags = GET_CPU_ATTR(cpu, eflags);
	return (eflags & EFL_IF) != 0;
}

/* I figured this out between 3 and 5 AM on a sunday morning. Could have been
 * playing Netrunner instead. Even just writing the PLDI paper would have been
 * more pleasurable. It was a real "twilight zone" bug. Aaaargh. */
void flush_instruction_cache(lang_void *x)
{
	/* I tried using SIM_flush_I_STC_logical here, and even the supposedly
	 * universal SIM_STC_flush_cache, but the make sucked. h8rs gon h8. */
	SIM_flush_all_caches();
}
