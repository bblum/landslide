/**
 * @file x86-simics.h
 * @brief x86-specific utilities
 * @author Ben Blum
 */

#ifndef __LS_X86_SIMICS_H
#define __LS_X86_SIMICS_H

#ifndef __LS_X86_H
#error "include x86.h, do not include this directly"
#endif

/* reading and writing cpu registers */
#define GET_CPU_ATTR(cpu, name) get_cpu_attr(cpu, #name)

static inline unsigned int get_cpu_attr(conf_object_t *cpu, const char *name) {
	attr_value_t register_attr = SIM_get_attribute(cpu, name);
	if (SIM_attr_is_integer(register_attr)) {
		return SIM_attr_integer(register_attr);
	} else if (SIM_attr_is_boolean(register_attr)) {
		return (int)SIM_attr_boolean(register_attr);
	} else {
		assert(register_attr.kind == Sim_Val_Invalid && "GET_CPU_ATTR failed!");
		// "Try again." WTF, simics??
		register_attr = SIM_get_attribute(cpu, name);
		assert(SIM_attr_is_integer(register_attr));
		return SIM_attr_integer(register_attr);
	}
}

#define SET_ATTR(obj, name, type, val) do {				\
		attr_value_t noob = SIM_make_attr_##type(val);		\
		set_error_t ret = SIM_set_attribute(obj, #name, &noob);	\
		assert(ret == Sim_Set_Ok && "SET_ATTR failed!");	\
		SIM_free_attribute(noob);				\
	} while (0)

#define SET_CPU_ATTR(cpu, name, val) SET_ATTR(cpu, name, integer, val)

#define GET_CR0(cpu) GET_CPU_ATTR((cpu), cr0)
#define GET_CR2(cpu) GET_CPU_ATTR((cpu), cr2)
#define GET_CR3(cpu) GET_CPU_ATTR((cpu), cr3)

#define READ_PHYS_MEMORY(cpu, addr, width) ({				\
	unsigned int __result = SIM_read_phys_memory(cpu, addr, width);	\
	assert(SIM_get_pending_exception() == SimExc_No_Exception &&	\
	       "failed read during VM translation -- kernel VM bug?");	\
	__result; })

#define WRITE_PHYS_MEMORY(cpu, addr, val, width) do {			\
		SIM_write_phys_memory(cpu, addr, val, width);		\
		assert(SIM_get_pending_exception() == SimExc_No_Exception && \
		       "failed write during VM translation -- kernel VM bug?"); \
	} while (0)

#endif
