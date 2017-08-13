/**
 * @file simulator-simics.h
 * @brief simics-specific glue definitions
 * @author Ben Blum
 */

#ifndef __LS_SIMULATOR_SIMICS_H
#define __LS_SIMULATOR_SIMICS_H

#ifndef __LS_SIMULATOR_H
#error "include simulator.h, do not include this directly"
#endif

#include <simics/api.h>

typedef conf_object_t cpu_t;
typedef conf_object_t symtable_t;
typedef conf_object_t keyboard_t;
typedef conf_object_t apic_t;
typedef conf_object_t pic_t;

typedef log_object_t simulator_state_t;
typedef conf_object_t simulator_object_t;

#endif /* __LS_SIMULATOR_SIMICS_H */
